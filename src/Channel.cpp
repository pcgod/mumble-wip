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
QHash<int, Channel *> Channel::channel_list_;
QReadWriteLock Channel::channel_lock_;
#endif

Channel::Channel(int id, const QString &name, QObject *p) : QObject(p),
	id_(id),
	position_(0),
	name_(name),
	inherit_acl_(true),
	temporary_(false),
	parent_(qobject_cast<Channel *>(p))
#ifdef MUMBLE
	, permissions_(0)
#endif
{
	if (parent_)
		parent_->addChannel(this);
}

Channel::~Channel() {
	if (parent_)
		parent_->removeChannel(this);

	foreach(Channel *c, channels_)
		delete c;

	foreach(ChanACL *acl, acls_)
		delete acl;
	foreach(Group *g, groups_)
		delete g;
	foreach(Channel *l, links_)
		unlink(l);

	Q_ASSERT(channels_.count() == 0);
	Q_ASSERT(children().count() == 0);
}

int Channel::id() const {
	return id_;
}

const QString &Channel::name() const {
	return name_;
}

const QString &Channel::description() const {
	return description_;
}

const QByteArray &Channel::description_hash() const {
	return description_hash_;
}

int Channel::position() const {
	return position_;
}

Channel *Channel::parent() const {
	return parent_;
}

bool Channel::temporary() const {
	return temporary_;
}

bool Channel::inherit_acl() const {
	return inherit_acl_;
}

const QList<Channel *> &Channel::channels() const {
	return channels_;
}

const QList<User *> &Channel::users() const {
	return users_;
}

const QHash<QString, Group *> &Channel::groups() const {
	return groups_;
}

const QList<ChanACL *> &Channel::acls() const {
	return acls_;
}

const QSet<Channel *> &Channel::links() const {
	return links_;
}

void Channel::set_name(const QString &name) {
	name_ = name;
}

void Channel::set_position(int position) {
	position_ = position;
}

void Channel::set_description(const QString &description) {
	description_ = description;
}

void Channel::set_description_hash(const QByteArray &description_hash) {
	description_hash_ = description_hash;
}

void Channel::set_temporary(bool temporary) {
	temporary_ = temporary;
}

void Channel::set_inherit_acl(bool inherit_acl) {
	inherit_acl_ = inherit_acl;
}

#ifdef MUMBLE
void Channel::resetPermissions() {
	foreach(Channel *c, channel_list_)
		c->set_permissions(0);
}

Channel *Channel::get(int id) {
	QReadLocker lock(&channel_lock_);
	return channel_list_.value(id);
}

Channel *Channel::add(int id, const QString &name) {
	QWriteLocker lock(&channel_lock_);

	if (channel_list_.contains(id))
		return NULL;

	Channel *c = new Channel(id, name, NULL);
	channel_list_.insert(id, c);
	return c;
}

void Channel::remove(Channel *c) {
	QWriteLocker lock(&channel_lock_);
	channel_list_.remove(c->id_);
}

unsigned int Channel::permissions() const {
	return permissions_;
}

void Channel::set_permissions(unsigned int permissions) {
	permissions_ = permissions;
}
#endif

bool Channel::lessThan(const Channel *first, const Channel *second) {
	if ((first->position_ != second->position_) && (first->parent_ == second->parent_))
		return first->position_ < second->position_;
	else
		return QString::localeAwareCompare(first->name_, second->name_) < 0;
}

bool Channel::isLinked(Channel *l) const {
	return ((l == this) || links_.contains(l));
}

void Channel::link(Channel *l) {
	if (links_.contains(l))
		return;
	links_.insert(l);
	l->links_.insert(this);
}

void Channel::unlink(Channel *l) {
	if (l) {
		links_.remove(l);
		l->links_.remove(this);
	} else {
		foreach(Channel *c, links_)
			unlink(c);
	}
}

const QSet<Channel *> Channel::allLinks() {
	QSet<Channel *> seen;
	seen.insert(this);
	if (links_.isEmpty())
		return seen;

	Channel *l, *lnk;
	QStack<Channel *> stack;
	stack.push(this);

	while (! stack.isEmpty()) {
		lnk = stack.pop();
		foreach(l, lnk->links_) {
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
	if (! channels_.isEmpty()) {
		QStack<Channel *> stack;
		stack.push(this);

		while (! stack.isEmpty()) {
			Channel *c = stack.pop();
			foreach(Channel *chld, c->channels_) {
				seen.insert(chld);
				if (! chld->channels_.isEmpty())
					stack.append(chld);
			}
		}
	}
	return seen;
}

void Channel::addAcl(ChanACL *a) {
	acls_ << a;
}

void Channel::removeAcl(ChanACL *a) {
	acls_.removeAll(a);
}

void Channel::clearAcls() {
	acls_.clear();
}

void Channel::addGroup(Group *group, const QString &name) {
	groups_[name] = group;
}

Group *Channel::findGroup(const QString &name) {
	return groups_.value(name);
}

void Channel::clearGroups() {
	groups_.clear();
}

void Channel::addChannel(Channel *c) {
	c->parent_ = this;
	c->setParent(this);
	channels_ << c;
}

void Channel::removeChannel(Channel *c) {
	c->parent_ = NULL;
	c->setParent(NULL);
	channels_.removeAll(c);
}

void Channel::addUser(User *p) {
	if (p->cChannel)
		p->cChannel->removeUser(p);
	p->cChannel = this;
	users_ << p;
}

void Channel::removeUser(User *p) {
	users_.removeAll(p);
}

int Channel::linkCount() const {
	return links_.count();
}

int Channel::userCount() const {
	return users_.count();
}

Channel::operator const QString() const {
	return QString::fromLatin1("%1[%2:%3%4]").arg(name_,
	        QString::number(id_),
	        QString::number(parent_ ? parent_->id_ : -1),
	        temporary_ ? QLatin1String("*") : QLatin1String(""));
}
