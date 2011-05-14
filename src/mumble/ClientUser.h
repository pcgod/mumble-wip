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

#ifndef _CLIENTUSER_H
#define _CLIENTUSER_H

#include "User.h"
#include "Timer.h"
#include "Settings.h"

class ClientUser;

typedef boost::shared_ptr<ClientUser> ClientUserPtr;

class ClientUser : public QObject, public User, public boost::enable_shared_from_this<ClientUser> {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(ClientUser)
	public:
		struct JitterRecord {
			int iSequence;
			int iFrames;
			quint64 uiElapsed;
		};

		Settings::TalkState tsState;
		Timer tLastTalkStateChange;
		bool bLocalMute;

		float fPowerMin, fPowerMax;
		float fAverageAvailable;

#ifdef REPORT_JITTER
		QMutex qmTiming;
		Timer tTiming;
		QList<JitterRecord> qlTiming;
#endif
		int iFrames;
		int iSequence;

		QByteArray qbaTextureFormat;
		QString qsFriendName;

		QString getFlagsString() const;
		ClientUser(QObject *p = NULL);

		/*! Determines whether a user is active or not
		 * A user is active when it is currently speaking or when the user has
		 * spoken within Settings::iActiveTime amount of seconds.
		 */
		bool isActive();

		static QHash<unsigned int, ClientUserPtr> c_qmUsers;
		static QReadWriteLock c_qrwlUsers;

		static QList<ClientUserPtr> c_qlTalking;
		static QReadWriteLock c_qrwlTalking;
		static QList<ClientUserPtr> getTalking();
		static QList<ClientUserPtr> getActive();

		static void sortUsersOverlay(QList<ClientUserPtr> &list);

		static ClientUserPtr get(unsigned int uiSession);
		static ClientUserPtr getByHash(const QString &hash);
		static bool isValid(unsigned int uiSession);
		static ClientUserPtr add(unsigned int uiSession, QObject *p = NULL);
		static ClientUserPtr match(ClientUserPtr p, bool matchname = false);
		static void remove(unsigned int uiSession);
		static void remove(const ClientUserPtr p);
		static bool lessThan(const ClientUserPtr first, const ClientUserPtr second);
	protected:
		static bool lessThanOverlay(const ClientUserPtr first, const ClientUserPtr second);
	public slots:
		void setTalking(Settings::TalkState ts);
		void setMute(bool mute);
		void setDeaf(bool deaf);
		void setSuppress(bool suppress);
		void setLocalMute(bool mute);
		void setSelfMute(bool mute);
		void setSelfDeaf(bool deaf);
		void setPrioritySpeaker(bool priority);
		void setRecording(bool recording);
	signals:
		void talkingChanged();
		void muteDeafChanged();
};

uint qHash(const ClientUserPtr &key);

QDataStream &operator<<(QDataStream &, const ClientUser::JitterRecord &);

#endif
