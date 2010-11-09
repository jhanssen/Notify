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
#include <QtNetwork>
#include <arpa/inet.h>
#include "alert.h"

class Handler : public QObject
{
    Q_OBJECT
public:
    enum Type { Handshake, Message };

    Handler(QObject* parent = 0)
        : QObject(parent)
    {
        QIcon icon(":/irssi.png");
        qApp->setWindowIcon(icon);
        tray.setIcon(icon);

        m_socket = new QTcpSocket;
        m_socket->connectToHost("127.0.0.1", 6945);
        m_socket->waitForConnected(10000);
        if (m_socket->state() != QTcpSocket::ConnectedState)
            qApp->quit();
        if (readHandShake())
            connect(m_socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
        else
            qApp->quit();

        QTimer* timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(checkForActive()));
        timer->setInterval(2000);
        timer->start();
    }

private:
    bool readHandShake()
    {
        QByteArray response = readData(m_socket, Handshake);
        if (response.isEmpty())
            return false;
        QBuffer buf(&response);
        if (!buf.open(QBuffer::ReadOnly))
            return false;

        {
            QDataStream stream(&buf);
            int pos;
            stream >> pos;
        }
        buf.close();

        QByteArray data;
        buf.setBuffer(&data);
        if (!buf.open(QBuffer::WriteOnly))
            return false;
        {
            QDataStream stream(&buf);
            stream << -5; // Last five messages
        }
        buf.close();
        sendData(m_socket, Handshake, data);

        readMessage();

        return true;
    }

private slots:
    void readMessage()
    {
        bool alert = false;
        bool more;
        do {
            QByteArray u8msg = readData(m_socket, Message, &more);

            if (!u8msg.isEmpty()) {
                QString msg = QString::fromUtf8(u8msg.data(), u8msg.size());
                tray.showMessage("irssi", msg);

                if (!alert)
                    alert = true;
            }
        } while (more);

        if (alert)
            OSXalert("htpc-v2");
    }

    void checkForActive()
    {
        OSXunalert("screen-4");
    }

private:
    void sendData(QTcpSocket* socket, Type type, const QByteArray &data)
    {
        int sz = htonl(data.size());
        socket->write(reinterpret_cast<char*>(&sz), 4);
        socket->write(data);
    }

    QByteArray readData(QTcpSocket* socket, Type type, bool* more = 0)
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

        if (more)
            *more = (socket->bytesAvailable() > 0);

        return ba;
    }


private:
    QTcpSocket* m_socket;
    QSystemTrayIcon tray;
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Handler handler;

    return app.exec();
}
