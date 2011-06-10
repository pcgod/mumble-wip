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

#include "Channel.h"
#include "User.h"
#include "Group.h"
#include "ACL.h"

#ifdef MUMBLE
QHash<int, Channel *> Channel::c_qhChannels;
QReadWriteLock Channel::c_qrwlChannels;
#endif

Channel::Channel(int id, const QString &name, QObject *p) : QObject(p) {
	iId = id;
	iPosition = 0;
	qsName = name;
	bInheritACL = true;
	bTemporary = false;
	cParent = qobject_cast<Channel *>(p);
	if (cParent)
		cParent->addChannel(this);
#ifdef MUMBLE
	uiPermissions = 0;
#endif
}

Channel::~Channel() {
	if (cParent)
		cParent->removeChannel(this);

	foreach(Channel *c, qlChannels)
		delete c;

	foreach(ChanACL *acl, qlACL)
		delete acl;
	foreach(Group *g, qhGroups)
		delete g;
	foreach(Channel *l, qsPermLinks)
		unlink(l);

	Q_ASSERT(qlChannels.count() == 0);
	Q_ASSERT(children().count() == 0);
}

int Channel::id() const {
	return iId;
}

const QString &Channel::name() const {
	return qsName;
}

const QString &Channel::description() const {
	return qsDesc;
}

const QByteArray &Channel::description_hash() const {
	return qbaDescHash;
}

int Channel::position() const {
	return iPosition;
}

Channel *Channel::parent() const {
	return cParent;
}

bool Channel::temporary() const {
	return bTemporary;
}

bool Channel::inherit_acl() const {
	return bInheritACL;
}

const QList<Channel *> &Channel::channels() const {
	return qlChannels;
}

const QList<User *> &Channel::users() const {
	return qlUsers;
}

const QHash<QString, Group *> &Channel::groups() const {
	return qhGroups;
}

const QList<ChanACL *> &Channel::acls() const {
	return qlACL;
}

const QSet<Channel *> &Channel::links() const {
	return qsPermLinks;
}

void Channel::set_name(const QString &name) {
	qsName = name;
}

void Channel::set_position(int position) {
	iPosition = position;
}

void Channel::set_description(const QString &description) {
	qsDesc = description;
}

void Channel::set_description_hash(const QByteArray &description_hash) {
	qbaDescHash = description_hash;
}

void Channel::set_temporary(bool temporary) {
	bTemporary = temporary;
}

void Channel::set_inherit_acl(bool inherit_acl) {
	bInheritACL = inherit_acl;
}

#ifdef MUMBLE
void Channel::resetPermissions() {
	foreach(Channel *c, c_qhChannels)
		c->set_permissions(0);
}

Channel *Channel::get(int id) {
	QReadLocker lock(&c_qrwlChannels);
	return c_qhChannels.value(id);
}

Channel *Channel::add(int id, const QString &name) {
	QWriteLocker lock(&c_qrwlChannels);

	if (c_qhChannels.contains(id))
		return NULL;

	Channel *c = new Channel(id, name, NULL);
	c_qhChannels.insert(id, c);
	return c;
}

void Channel::remove(Channel *c) {
	QWriteLocker lock(&c_qrwlChannels);
	c_qhChannels.remove(c->iId);
}

unsigned int Channel::permissions() const {
	return uiPermissions;
}

void Channel::set_permissions(unsigned int permissions) {
	uiPermissions = permissions;
}
#endif

bool Channel::lessThan(const Channel *first, const Channel *second) {
	if ((first->iPosition != second->iPosition) && (first->cParent == second->cParent))
		return first->iPosition < second->iPosition;
	else
		return QString::localeAwareCompare(first->qsName, second->qsName) < 0;
}

bool Channel::isLinked(Channel *l) const {
	return ((l == this) || qsPermLinks.contains(l));
}

void Channel::link(Channel *l) {
	if (qsPermLinks.contains(l))
		return;
	qsPermLinks.insert(l);
	l->qsPermLinks.insert(this);
}

void Channel::unlink(Channel *l) {
	if (l) {
		qsPermLinks.remove(l);
		l->qsPermLinks.remove(this);
	} else {
		foreach(Channel *c, qsPermLinks)
			unlink(c);
	}
}

const QSet<Channel *> Channel::allLinks() {
	QSet<Channel *> seen;
	seen.insert(this);
	if (qsPermLinks.isEmpty())
		return seen;

	Channel *l, *lnk;
	QStack<Channel *> stack;
	stack.push(this);

	while (! stack.isEmpty()) {
		lnk = stack.pop();
		foreach(l, lnk->qsPermLinks) {
			if (! seen.contains(l)) {
				seen.insert(l);
				stack.push(l);
			}
		}
	}
	return seen;
}

const QSet<Channel *> Channel::allChildren() {
	QSet<Channel *> seen;
	if (! qlChannels.isEmpty()) {
		QStack<Channel *> stack;
		stack.push(this);

		while (! stack.isEmpty()) {
			Channel *c = stack.pop();
			foreach(Channel *chld, c->qlChannels) {
				seen.insert(chld);
				if (! chld->qlChannels.isEmpty())
					stack.append(chld);
			}
		}
	}
	return seen;
}

void Channel::addAcl(ChanACL *a) {
	qlACL << a;
}

void Channel::removeAcl(ChanACL *a) {
	qlACL.removeAll(a);
}

void Channel::clearAcls() {
	qlACL.clear();
}

void Channel::addGroup(Group *group, const QString &name) {
	qhGroups[name] = group;
}

Group *Channel::findGroup(const QString &name) {
	return qhGroups.value(name);
}

void Channel::clearGroups() {
	qhGroups.clear();
}

void Channel::addChannel(Channel *c) {
	c->cParent = this;
	c->setParent(this);
	qlChannels << c;
}

void Channel::removeChannel(Channel *c) {
	c->cParent = NULL;
	c->setParent(NULL);
	qlChannels.removeAll(c);
}

void Channel::addUser(User *p) {
	if (p->cChannel)
		p->cChannel->removeUser(p);
	p->cChannel = this;
	qlUsers << p;
}

void Channel::removeUser(User *p) {
	qlUsers.removeAll(p);
}

int Channel::linkCount() const {
	return qsPermLinks.count();
}

int Channel::userCount() const {
	return qlUsers.count();
}

Channel::operator const QString() const {
	return QString::fromLatin1("%1[%2:%3%4]").arg(qsName,
	        QString::number(iId),
	        QString::number(cParent ? cParent->iId : -1),
	        bTemporary ? QLatin1String("*") : QLatin1String(""));
}
