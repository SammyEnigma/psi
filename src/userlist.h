/*
 * userlist.h - high-level roster
 * Copyright (C) 2001-2002  Justin Karneges
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef USERLIST_H
#define USERLIST_H

#include "activity.h"
#include "geolocation.h"
#include "iris/xmpp_liverosteritem.h"
#include "iris/xmpp_resource.h"
#include "maybe.h"
#include "mood.h"

#include <QDateTime>
#include <QList>
#include <QPixmap>
#include <QString>

class AvatarFactory;

namespace XMPP {
class Jid;
}

class UserResource : public XMPP::Resource {
public:
    UserResource() = default;
    UserResource(const XMPP::Resource &);
    ~UserResource() = default;

    void setResource(const XMPP::Resource &);

    const QString &versionString() const;
    const QString &clientVersion() const;
    const QString &clientName() const;
    const QString &clientOS() const;
    void           setClient(const QString &name, const QString &version, const QString &os);

    Maybe<int>     timezoneOffset() const;
    const QString &timezoneOffsetString() const;
    void           setTimezone(Maybe<int> tzo);

    const QString &publicKeyID() const;
    int            pgpVerifyStatus() const;
    QDateTime      sigTimestamp() const;
    void           setPublicKeyID(const QString &);
    void           setPgpVerifyStatus(int);
    void           setSigTimestamp(const QDateTime &);

    void               setTune(const QString &);
    const QString     &tune() const;
    void               setGeoLocation(const GeoLocation &);
    const GeoLocation &geoLocation() const;
    // void setPhysicalLocation(const PhysicalLocation&);
    // const PhysicalLocation& physicalLocation() const;

private:
    QString     v_ver, v_clientName, v_clientVersion, v_clientOS, v_keyID;
    Maybe<int>  v_tzo;
    QString     v_tzoString;
    QString     v_tune;
    GeoLocation v_geoLocation;
    // PhysicalLocation v_physicalLocation;
    int       v_pgpVerifyStatus = 0;
    QDateTime sigts;
};

bool operator<(const UserResource &r1, const UserResource &r2);
bool operator<=(const UserResource &r1, const UserResource &r2);
bool operator==(const UserResource &r1, const UserResource &r2);
bool operator>(const UserResource &r1, const UserResource &r2);
bool operator>=(const UserResource &r1, const UserResource &r2);

class UserResourceList : public QList<UserResource> {
public:
    UserResourceList();
    ~UserResourceList() = default;

    void sort();

    UserResourceList::Iterator find(const QString &);
    UserResourceList::Iterator priority();

    UserResourceList::ConstIterator find(const QString &) const;
    UserResourceList::ConstIterator priority() const;
};

class UserListItem : public XMPP::LiveRosterItem {
public:
    UserListItem(bool self = false);
    ~UserListItem() = default;

    bool            inList() const;
    bool            isTransport() const;
    bool            isConference() const;
    bool            isAvailable() const;
    bool            isHidden() const;
    bool            isAway() const;
    QDateTime       lastAvailable() const;
    int             lastMessageType() const;
    void            setLastMessageType(const int mtype);
    const QString  &presenceError() const;
    bool            isSelf() const;
    QString         makeTip(bool trim = true, bool doLinkify = true) const;
    QString         makeBareTip(bool trim, bool doLinkify) const;
    QString         makeDesc() const;
    bool            isPrivate() const;
    const Mood     &mood() const;
    QStringList     clients() const;
    QString         findClient(const UserResource &ur) const;
    const Activity &activity() const;
    QString         pending() const;
    QString         tooltipPgpData(const UserResource &ur, const QString &imgTag) const;

    void               setJid(const XMPP::Jid &);
    void               setInList(bool);
    void               setLastAvailable(const QDateTime &);
    void               setPresenceError(const QString &);
    void               setPrivate(bool);
    void               setMood(const Mood &);
    void               setActivity(const Activity &);
    void               setTune(const QString &);
    void               setConference(bool);
    void               setPending(int p, int h);
    const QString     &tune() const;
    void               setGeoLocation(const GeoLocation &);
    const GeoLocation &geoLocation() const;
    // void setPhysicalLocation(const PhysicalLocation&);
    // const PhysicalLocation& physicalLocation() const;
    void setAvatarFactory(AvatarFactory *);

    UserResourceList               &userResourceList();
    UserResourceList::Iterator      priority();
    const UserResourceList         &userResourceList() const;
    UserResourceList::ConstIterator priority() const;
    void                            removeAllResources();
    void                            removeResource(const QString &resource);

    bool isSecure() const;
    bool isSecure(const QString &rname) const;
    void setSecure(const QString &rname, bool);

    const QString &publicKeyID() const;
    void           setPublicKeyID(const QString &);

private:
    int              lastmsgtype, v_pending, v_hPending;
    bool             v_inList;
    QDateTime        v_t;
    UserResourceList v_url;
    QString          v_perr;
    bool             v_self, v_isTransport, v_isConference;
    bool             v_private;
    QStringList      secList;
    QString          v_keyID;
    QPixmap          v_avatar;
    Mood             v_mood;
    Activity         v_activity;
    QString          v_tune;
    GeoLocation      v_geoLocation;
    // PhysicalLocation v_physicalLocation;
    AvatarFactory *v_avatarFactory;
};

typedef QListIterator<UserListItem *> UserListIt;

class UserList : public QList<UserListItem *> {
public:
    UserList()  = default;
    ~UserList() = default;

    UserListItem *find(const XMPP::Jid &);
};

#endif // USERLIST_H
