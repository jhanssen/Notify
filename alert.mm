#include <NSApplication.h>
#include <NSAppleScript.h>
#include <NSDictionary.h>
#include <NSRange.h>
#include <NSString.h>

NSAppleScript* check = 0;

static bool appIsAtFront(const char* name)
{
    if (check == 0)
        check = [[NSAppleScript alloc] initWithSource:
            @"tell application \"System Events\"\n"
            @"  set frontApp to name of first application process whose frontmost is true\n"
            @"end tell\n"
            @"tell application frontApp\n"
            @"  if the (count of windows) is not 0 then\n"
            @"    get name of front window\n"
            @"  end if\n"
            @"end tell\n"
        ];

    NSString* find = [NSString stringWithCString:name];

    NSDictionary* error = [[[NSDictionary alloc] init] autorelease];
    NSAppleEventDescriptor* desc = [[check executeAndReturnError:&error] autorelease];

    NSString* value = [desc stringValue];
    NSRange range = [value rangeOfString:find];

    return (range.location != NSNotFound);
}

void OSXunalert(const char* app)
{
    if (appIsAtFront(app))
        [[NSApplication sharedApplication] cancelUserAttentionRequest:NSCriticalRequest];
}

void OSXalert(const char* app)
{
    if (!appIsAtFront(app))
        [[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
}
