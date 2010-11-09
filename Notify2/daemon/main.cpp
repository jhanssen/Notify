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

#include <QtCore>
#include <QtNetwork>
#include <arpa/inet.h>

class ReadMessageEvent : public QEvent
{
public:
    ReadMessageEvent(int pos, QTcpSocket* socket)
        : QEvent(static_cast<QEvent::Type>(QEvent::User + 15)), m_pos(pos), m_socket(socket)
    {
    }

    int m_pos;
    QTcpSocket* m_socket;
};

class Handler : public QObject
{
    Q_OBJECT
public:
    enum Type { Handshake, Message };

    Handler(const QString &fn, QObject* parent = 0)
        : QObject(parent), m_filename(fn), m_pos(0)
    {
        m_watcher.addPath(m_filename);
        connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleMessage()));

        connect(&m_server, SIGNAL(newConnection()),
                this, SLOT(newTcpConnection()));
        m_server.listen(QHostAddress::Any, 6945);
    }

protected:
    bool event(QEvent* event)
    {
        if (event->type() == QEvent::User + 15) {
            ReadMessageEvent* rme = static_cast<ReadMessageEvent*>(event);
            QFile file(m_filename);
            if (!file.open(QFile::ReadOnly)) {
                QCoreApplication::postEvent(this, new ReadMessageEvent(rme->m_pos, rme->m_socket));
                return true;
            }
            int pos = rme->m_pos;
            if (pos < 0)
                pos = qMin(0, m_pos + pos);
            readMessages(&file, pos, (QList<QTcpSocket*>() << rme->m_socket));

            return true;
        }
        return false;
    }

public slots:
    void handleMessage()
    {
        QFile file(m_filename);
        if (!file.open(QFile::ReadOnly)) {
            QTimer::singleShot(100, this, SLOT(handleMessage()));
            return;
        }
        m_pos = readMessages(&file, m_pos);
    }

private slots:
    void newTcpConnection()
    {
        QTcpSocket* socket = m_server.nextPendingConnection();
        if (readHandShake(socket)) {
            connect(socket, SIGNAL(disconnected()), this, SLOT(removeSocket()));
            m_sockets << socket;
        } else {
            delete socket;
	}
    }

    void removeSocket()
    {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket)
            return;

        m_sockets.removeAll(socket);
        socket->deleteLater();;
    }

private:
    bool readHandShake(QTcpSocket* socket)
    {
        QByteArray data;
        QBuffer buf(&data);
        if (!buf.open(QBuffer::WriteOnly))
            return false;
        {
            QDataStream stream(&buf);
            stream << m_pos;
        }
        buf.close();
        sendData(socket, Handshake, data);

        QByteArray response = readData(socket, Handshake);
        if (response.isEmpty()) {
            return false;
	}
        buf.setBuffer(&response);
        if (!buf.open(QBuffer::ReadOnly))
            return false;

        {
            QDataStream stream(&buf);
            int pos;
            stream >> pos;

            QCoreApplication::postEvent(this, new ReadMessageEvent(pos, socket));
        }

        QByteArray empty;
        sendData(socket, Message, empty);

        return true;
    }

    int readMessages(QFile* file, int start,
		     const QList<QTcpSocket*> sockets = QList<QTcpSocket*>())
    {
        QList<QTcpSocket*> socketList = sockets;
        if (socketList.isEmpty())
            socketList = m_sockets;

        QString line;
        bool print;
        QByteArray ba = file->readAll();
        QList<QByteArray> list = ba.split('\n');
        while (list.back() == QByteArray())
            list.pop_back();
        for (int i = 0; i < list.size(); ++i) {
            print = (start <= i);
            line = list.at(i);
            if (print) {
                QByteArray u8line = line.toUtf8();
                foreach(QTcpSocket* socket, socketList) {
                    sendData(socket, Message, u8line);
                }
            }
        }
        return list.size();
    }

    void sendData(QTcpSocket* socket, Type type, const QByteArray &data)
    {
        int sz = htonl(data.size());
        socket->write(reinterpret_cast<char*>(&sz), 4);
        socket->write(data);
    }

    QByteArray readData(QTcpSocket* socket, Type type)
    {
        QByteArray intdata;
        int rem = 4;
        while (rem > 0) {
            intdata += socket->read(rem);
            rem = 4 - intdata.size();
            if (rem > 0)
                socket->waitForReadyRead();
        }
        int sz;
        memcpy(&sz, intdata.data(), 4);
        sz = ntohl(sz);

        rem = sz;
        QByteArray ba;
        while (rem > 0) {
            ba += socket->read(rem);
            rem = sz - ba.size();
            if (rem > 0)
                socket->waitForReadyRead();
        }

        return ba;
    }

private:
    QFileSystemWatcher m_watcher;
    QString m_filename;
    int m_pos;

    QTcpServer m_server;
    QList<QTcpSocket*> m_sockets;
};

#include "main.moc"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    Handler handler(QDir::homePath() + "/.irssi/fnotify");
    handler.handleMessage();

    return app.exec();
}
