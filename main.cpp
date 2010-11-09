/*
Copyright (c) 2010 Jan Erik Hanssen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
fnotify for irssi is required for this to work

http://www.leemhuis.info/files/fnotify/fnotify
*/

#include <QtGui>
#include "alert.h"

class Handler : public QObject
{
    Q_OBJECT
public:
    Handler(QObject* parent = 0)
        : QObject(parent), pos(0)
    {
        QIcon icon(":/irssi.png");
        qApp->setWindowIcon(icon);
        tray.setIcon(icon);

        tray.show();
        watcher.addPath(QDir::homePath() + "/.irssi/fnotify");
        connect(&watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleMessage(QString)));

        QTimer* timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(checkForActive()));
        timer->setInterval(2000);
        timer->start();
    }

private slots:
    void handleMessage(const QString& fn = QString())
    {
        static QString last;
        QString filename = fn;
        if (!filename.isEmpty())
            last = filename;
        else
            filename = last;
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            QTimer::singleShot(100, this, SLOT(handleMessage()));
            return;
        }
        QString line;
        bool print;
        QByteArray ba = file.readAll();
        QList<QByteArray> list = ba.split('\n');
        while (list.back() == QByteArray())
            list.pop_back();
        for (int i = 0; i < list.size(); ++i) {
            print = (pos <= i);
            line = list.at(i);
            if (print)
                tray.showMessage("irssi", line);
        }
        pos = list.size();

        OSXalert("screen-4");
    }

    void checkForActive()
    {
        OSXunalert("screen-4");
    }

private:
    QSystemTrayIcon tray;
    QFileSystemWatcher watcher;
    int pos;
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Handler handler;

    return app.exec();
}
