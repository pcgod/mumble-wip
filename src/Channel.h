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

#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "murmur_pch.h"

class User;
class Group;
class ChanACL;
class ClientUser;

class Channel : public QObject {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(Channel)

		int iId;
		int iPosition;
		bool bInheritACL;
		bool bTemporary;

		QSet<Channel *> qsUnseen;
		Channel *cParent;
		QString qsName;
		QString qsDesc;
		QByteArray qbaDescHash;
		QList<Channel *> qlChannels;
		QList<User *> qlUsers;
		QHash<QString, Group *> qhGroups;
		QList<ChanACL *> qlACL;

		QSet<Channel *> qsPermLinks;

#ifdef MUMBLE
		unsigned int uiPermissions;

		static QHash<int, Channel *> c_qhChannels;
		static QReadWriteLock c_qrwlChannels;

	public:
		static Channel *get(int);
		static Channel *add(int, const QString &);
		static void remove(Channel *);
		static void resetPermissions();

		void addClientUser(ClientUser *p);

		unsigned int permissions() const { return uiPermissions; }
		void set_permissions(unsigned int permissions) { uiPermissions = permissions; }
#endif

	public:
		Channel(int id, const QString &name, QObject *p = NULL);
		~Channel();

		int id() const { return iId; }
		const QString &name() const { return qsName; }
		const QString &description() const { return qsDesc; }
		const QByteArray &description_hash() const { return qbaDescHash; }
		int position() const { return iPosition; }
		Channel *parent() const { return cParent; }
		bool temporary() const { return bTemporary; }
		bool inherit_acl() const { return bInheritACL; }
		const QList<Channel *> &channels() const { return qlChannels; }
		const QList<User *> &users() const { return qlUsers; }
		const QHash<QString, Group *> &groups() const { return qhGroups; }
		const QList<ChanACL *> &acls() const { return qlACL; }
		const QSet<Channel *> &links() const { return qsPermLinks; }

		void set_name(const QString &name) { qsName = name; }
		void set_position(int position) { iPosition = position; }
		void set_description(const QString &description) { qsDesc = description; }
		void set_description_hash(const QByteArray &description_hash) { qbaDescHash = description_hash; }
		void set_temporary(bool temporary) { bTemporary = temporary; }
		void set_inherit_acl(bool inherit_acl) { bInheritACL = inherit_acl; }

		void addAcl(ChanACL *a);
		void removeAcl(ChanACL *a) { qlACL.removeAll(a); }
		void clearAcls() { qlACL.clear(); }

		void addChannel(Channel *c);
		void removeChannel(Channel *c);
		void addUser(User *p);
		void removeUser(User *p);

		void addGroup(Group *group, const QString &name);
		Group *findGroup(const QString &name) { return qhGroups.value(name); }
		void clearGroups() { qhGroups.clear(); }

		bool isLinked(Channel *c) const;
		void link(Channel *c);
		void unlink(Channel *c = NULL);
		int linkCount() const { return qsPermLinks.count(); }
		int userCount() const { return qlUsers.count(); }

		const QSet<Channel *> allLinks();
		const QSet<Channel *> allChildren();

		operator const QString() const;

		static bool lessThan(const Channel *, const Channel *);
};

#endif
