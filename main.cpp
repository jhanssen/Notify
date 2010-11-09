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
    argv[0] = (char*)malloc(7);
    memcpy(argv[0], "Notify\0", 7);
    QApplication app(argc, argv);

    Handler handler;

    int ret = app.exec();
    free(argv[0]);
    return ret;
}
