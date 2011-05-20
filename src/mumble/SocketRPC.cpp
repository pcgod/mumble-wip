/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "SocketRPC.h"
#include "Global.h"
#include "MainWindow.h"
#include "ServerHandler.h"
#include "Channel.h"
#include "ClientUser.h"

#if QT_VERSION < 0x040600
#include "QXmlStreamReaderCompat.h"
#endif

SocketRPCClient::SocketRPCClient(QLocalSocket *s, QObject *p) : QObject(p), qlsSocket(s), qbBuffer(NULL) {
	qlsSocket->setParent(this);

	connect(qlsSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(qlsSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(error(QLocalSocket::LocalSocketError)));
	connect(qlsSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));

	qxsrReader.setDevice(qlsSocket);
	qxswWriter.setAutoFormatting(true);

	qbBuffer = new QBuffer(&qbaOutput, this);
	qbBuffer->open(QIODevice::WriteOnly);
	qxswWriter.setDevice(qbBuffer);
}

void SocketRPCClient::disconnected() {
	deleteLater();
}

void SocketRPCClient::error(QLocalSocket::LocalSocketError) {
}

void SocketRPCClient::readyRead() {
	forever {
		switch (qxsrReader.readNext()) {
			case QXmlStreamReader::Invalid: {
					if (qxsrReader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
						qWarning() << "Malformed" << qxsrReader.error();
						qlsSocket->abort();
					}
					return;
				}
				break;
			case QXmlStreamReader::EndDocument: {
					qxswWriter.writeCurrentToken(qxsrReader);

					processXml();

					qxsrReader.clear();
					qxsrReader.setDevice(qlsSocket);

					qxswWriter.setDevice(NULL);
					delete qbBuffer;
					qbaOutput = QByteArray();
					qbBuffer = new QBuffer(&qbaOutput, this);
					qbBuffer->open(QIODevice::WriteOnly);
					qxswWriter.setDevice(qbBuffer);
				}
				break;
			default:
				qxswWriter.writeCurrentToken(qxsrReader);
				break;
		}
	}
}

void SocketRPCClient::processXml() {
#if QT_VERSION < 0x040600
	QXmlStreamReaderCompat xml(qbaOutput);
#else
	QXmlStreamReader xml(qbaOutput);
#endif

	if (!xml.readNextStartElement())
		return;

	bool ack = false;
	QMap<QString, QVariant> qmRequest;
	QMap<QString, QVariant> qmReply;
	QMap<QString, QVariant>::const_iterator iter;

	QXmlStreamAttributes attributes = xml.attributes();
	foreach (QXmlStreamAttribute attr, attributes) {
		qmRequest.insert(attr.name().toString(), attr.value().toString());
	}

	while (xml.readNextStartElement()) {
		qmRequest.insert(xml.name().toString(), xml.readElementText());
	}

	iter = qmRequest.find(QLatin1String("reqid"));
	if (iter != qmRequest.constEnd())
		qmReply.insert(iter.key(), iter.value());

	if (xml.name() == QLatin1String("focus")) {
		g.mw->show();
		g.mw->raise();
		g.mw->activateWindow();

		ack = true;
	} else if (xml.name() == QLatin1String("self")) {
		iter = qmRequest.find(QLatin1String("mute"));
		if (iter != qmRequest.constEnd()) {
			bool set = iter.value().toBool();
			if (set != g.s.bMute) {
				g.mw->qaAudioMute->setChecked(! set);
				g.mw->qaAudioMute->trigger();
			}
		}
		iter = qmRequest.find(QLatin1String("deaf"));
		if (iter != qmRequest.constEnd()) {
			bool set = iter.value().toBool();
			if (set != g.s.bDeaf) {
				g.mw->qaAudioDeaf->setChecked(! set);
				g.mw->qaAudioDeaf->trigger();
			}
		}

		ack = true;
	} else if (xml.name() == QLatin1String("url")) {
		ServerHandlerPtr sh = g.getCurrentServerHandler();
		if (sh && sh->isRunning() && g.uiSession) {
			QString host, user, pw;
			unsigned short port;
			QUrl u;

			sh->getConnectionInfo(host, port, user, pw);
			u.setScheme(QLatin1String("mumble"));
			u.setHost(host);
			u.setPort(port);
			u.setUserName(user);
			u.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
			QStringList path;
			Channel *c = ClientUser::get(g.uiSession)->cChannel;
			while (c->cParent) {
				path.prepend(c->qsName);
				c = c->cParent;
			}
			u.setPath(path.join(QLatin1String("/")));
			qmReply.insert(QLatin1String("href"), u);
		}

		iter = qmRequest.find(QLatin1String("href"));
		if (iter != qmRequest.constEnd()) {
			QUrl u = iter.value().toUrl();
			if (u.isValid() && u.scheme() == QLatin1String("mumble")) {
				OpenURLEvent *oue = new OpenURLEvent(u);
				qApp->postEvent(g.mw, oue);
				ack = true;
			}
		} else {
			ack = true;
		}
	}

	QString reply;
	QXmlStreamWriter stream(&reply);

	stream.writeStartDocument();
	stream.writeStartElement(QLatin1String("reply"));

	qmReply.insert(QLatin1String("succeeded"), ack);

	for (iter = qmReply.constBegin(); iter != qmReply.constEnd(); ++iter) {
		stream.writeTextElement(iter.key(), iter.value().toString());
	}

	stream.writeEndElement();
	stream.writeEndDocument();

	qlsSocket->write(reply.toUtf8());
}

SocketRPC::SocketRPC(const QString &basename, QObject *p) : QObject(p) {
	qlsServer = new QLocalServer(this);

	QString pipepath;

#ifdef Q_OS_WIN
	pipepath = basename;
#else
	pipepath = QDir::home().absoluteFilePath(QLatin1String(".") + basename + QLatin1String("Socket"));
	{
		QFile f(pipepath);
		if (f.exists()) {
			qWarning() << "SocketRPC: Removing old socket on" << pipepath;
			f.remove();
		}
	}
#endif

	if (! qlsServer->listen(pipepath)) {
		qWarning() << "SocketRPC: Listen failed";
		delete qlsServer;
		qlsServer = NULL;
	} else {
		connect(qlsServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
	}
}

void SocketRPC::newConnection() {
	while (1) {
		QLocalSocket *qls = qlsServer->nextPendingConnection();
		if (! qls)
			break;
		new SocketRPCClient(qls, this);
	}
}

bool SocketRPC::send(const QString &basename, const QString &request, const QMap<QString, QVariant> &param) {
	QString pipepath;

#ifdef Q_OS_WIN
	pipepath = basename;
#else
	pipepath = QDir::home().absoluteFilePath(QLatin1String(".") + basename + QLatin1String("Socket"));
#endif

	QLocalSocket qls;
	qls.connectToServer(pipepath);
	if (! qls.waitForConnected(1000)) {
		return false;
	}

	QString requestxml;
	QXmlStreamWriter stream(&requestxml);

	stream.writeStartDocument();
	stream.writeStartElement(request);

	for (QMap<QString, QVariant>::const_iterator iter = param.constBegin(); iter != param.constEnd(); ++iter) {
		stream.writeTextElement(iter.key(), iter.value().toString());
	}

	stream.writeEndElement();
	stream.writeEndDocument();

	qls.write(requestxml.toUtf8());
	qls.flush();

	if (! qls.waitForReadyRead(2000)) {
		return false;
	}

	QByteArray qba = qls.readAll();

	QString success;
#if QT_VERSION < 0x040600
	QXmlStreamReaderCompat xml(qba);
#else
	QXmlStreamReader xml(qba);
#endif

	bool found = false;
	while (xml.readNext()) {
		if (found && xml.isEndElement())
			break;
		if (found && xml.isStartElement())
			if (xml.name() == QLatin1String("succeeded"))
				success = xml.readElementText();
			else
				xml.skipCurrentElement();
		if (!found && xml.isStartElement() && xml.name() == QLatin1String("reply"))
			found = true;
	}

	if (success.isNull())
		return false;

	return QVariant(success).toBool();
}
