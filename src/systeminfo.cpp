/*
 * Copyright (C) 2007-2008  Psi Development Team
 * Licensed under the GNU General Public License.
 * See the COPYING file for more information.
 */

#include "systeminfo.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QTextStream>
#if defined(Q_OS_UNIX)
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#endif
#if defined(Q_OS_WIN)
#include <windows.h>
#endif
#if defined(Q_OS_HAIKU)
#include <AppFileInfo.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <sys/utsname.h>
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
#include <QOperatingSystemVersion>
#if defined(Q_OS_WIN)
#include <versionhelpers.h>
#endif
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)
#error "Minimal supported version of Qt in this file is 5.9.0"
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
static QString lsbRelease(const QStringList &args)
{
    QStringList path = QString(qgetenv("PATH")).split(':');
    QString     found;

    for (const QString &dirname : std::as_const(path)) {
        QDir      dir(dirname);
        QFileInfo cand(dir.filePath("lsb_release"));
        if (cand.isExecutable()) {
            found = cand.absoluteFilePath();
            break;
        }
    }

    if (found.isEmpty()) {
        return QString();
    }

    QProcess process;
    process.start(found, args, QIODevice::ReadOnly);

    if (!process.waitForStarted())
        return QString(); // process failed to start

    QTextStream stream(&process);
    QString     ret;

    while (process.waitForReadyRead())
        ret += stream.readAll();

    ret = ret.trimmed();
    if (ret.startsWith('"') && ret.endsWith('"') && ret.size() > 1) {
        ret = ret.mid(1, ret.size() - 2);
    }
    process.close();
    return ret;
}

static QString unixHeuristicDetect()
{
    struct utsname u;
    uname(&u);
    auto ret = QString::asprintf("%s", u.sysname);

    // get description about os
    enum LinuxName {
        LinuxNone = 0,

        LinuxMandrake,
        LinuxDebian,
        LinuxRedHat,
        LinuxGentoo,
        LinuxExherbo,
        LinuxSlackware,
        LinuxSuSE,
        LinuxConectiva,
        LinuxCaldera,
        LinuxLFS,

        LinuxASP, // Russian Linux distros
        LinuxALT,
        LinuxRFRemix,

        LinuxPLD, // Polish Linux distros
        LinuxAurox,
        LinuxArch
    };

    enum OsFlags { OsUseName = 0, OsUseFile, OsAppendFile };

    struct OsInfo {
        LinuxName id;
        OsFlags   flags;
        QString   file;
        QString   name;
    } osInfo[] = { { LinuxMandrake, OsUseFile, "/etc/mandrake-release", "Mandrake Linux" },
                   { LinuxDebian, OsAppendFile, "/etc/debian_version", "Debian GNU/Linux" },
                   { LinuxGentoo, OsUseFile, "/etc/gentoo-release", "Gentoo Linux" },
                   { LinuxExherbo, OsUseName, "/etc/exherbo-release", "Exherbo Linux" },
                   { LinuxArch, OsUseName, "/etc/arch-release", "Arch Linux" },
                   { LinuxSlackware, OsAppendFile, "/etc/slackware-version", "Slackware Linux" },
                   { LinuxPLD, OsUseFile, "/etc/pld-release", "PLD Linux" },
                   { LinuxAurox, OsUseName, "/etc/aurox-release", "Aurox Linux" },
                   { LinuxArch, OsUseFile, "/etc/arch-release", "Arch Linux" },
                   { LinuxLFS, OsAppendFile, "/etc/lfs-release", "LFS Linux" },
                   { LinuxRFRemix, OsUseFile, "/etc/rfremix-release", "RFRemix Linux" },

                   // untested
                   { LinuxSuSE, OsUseFile, "/etc/SuSE-release", "SuSE Linux" },
                   { LinuxConectiva, OsUseFile, "/etc/conectiva-release", "Conectiva Linux" },
                   { LinuxCaldera, OsUseFile, "/etc/.installed", "Caldera Linux" },

                   // many distros use the /etc/redhat-release for compatibility, so RedHat will be the last :)
                   { LinuxRedHat, OsUseFile, "/etc/redhat-release", "RedHat Linux" },

                   { LinuxNone, OsUseName, "", "" } };

    for (int i = 0; osInfo[i].id != LinuxNone; i++) {
        QFileInfo fi(osInfo[i].file);
        if (fi.exists()) {
            char buffer[128];

            QFile f(osInfo[i].file);
            f.open(QIODevice::ReadOnly);
            f.readLine(buffer, 128);
            QString desc = QString::fromUtf8(buffer);

            desc = desc.trimmed();

            switch (osInfo[i].flags) {
            case OsUseFile:
                ret = desc;
                break;
            case OsUseName:
                ret = osInfo[i].name;
                break;
            case OsAppendFile:
                ret = osInfo[i].name + " (" + desc + ")";
                break;
            }

            break;
        }
    }
    return ret;
}
#endif

SystemInfo::SystemInfo() : QObject(QCoreApplication::instance())
{
    // Initialize
    os_str_      = "Unknown";
    os_name_str_ = os_str_;

    // Detect
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_HAIKU)
    // attempt to get LSB version before trying the distro-specific approach
    os_str_ = lsbRelease(QStringList() << "--description" << "--short");

    if (os_str_.isEmpty()) {
        os_str_ = unixHeuristicDetect();
    }
    os_name_str_ = os_str_;

    os_version_str_ = lsbRelease(QStringList() << "--release" << "--short");

    if (os_version_str_.isEmpty()) {
        os_version_str_ = QSysInfo::productVersion();
    }
    if (os_version_str_ == QLatin1String("unknown")) {
        os_version_str_.clear();
    }

    if (!os_version_str_.isEmpty() && os_name_str_.contains(os_version_str_)) {
        os_version_str_.clear();
    }

    if (os_version_str_.isEmpty()) {
        os_str_ = os_name_str_;
    } else {
        os_str_ = os_name_str_ + " " + os_version_str_;
    }

#elif defined(Q_OS_MAC)
    os_str_.clear();
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 5)
    const auto current = QOperatingSystemVersion::current();
    os_name_str_       = current.name();
    os_version_str_    = QString::number(current.majorVersion());
    if (current.minorVersion() >= 0)
        os_version_str_ += "." + QString::number(current.minorVersion());
    if (current.microVersion() > 0)
        os_version_str_ += "." + QString::number(current.microVersion());

    if (current > QOperatingSystemVersion::MacOSCatalina) {
        // Unknown name
    } else if (current >= QOperatingSystemVersion::MacOSCatalina) {
        os_version_str_ += " (Catalina)";
    } else if (current >= QOperatingSystemVersion::MacOSMojave) {
        os_version_str_ += " (Mojave)";
    } else if (current >= QOperatingSystemVersion::MacOSHighSierra) {
        os_version_str_ += " (High Sierra)";
    } else if (current >= QOperatingSystemVersion::MacOSSierra) {
        os_version_str_ += " (Sierra)";
    } else if (current >= QOperatingSystemVersion::OSXElCapitan) {
        os_version_str_ += " (El Capitan)";
    }
#else
    QSysInfo::MacVersion v = QSysInfo::MacintoshVersion;
    switch (v) {
    case 0x0011: // QSysInfo::MV_10_15 should not be used for compatibility reasons
        os_name_str_    = "macOS";
        os_version_str_ = "10.15 (Catalina)";
        break;
    case 0x0010: // QSysInfo::MV_10_14 should not be used for compatibility reasons
        os_name_str_    = "macOS";
        os_version_str_ = "10.14 (Mojave)";
        break;
    case 0x000F: // QSysInfo::MV_10_13 should not be used for compatibility reasons
        os_name_str_    = "macOS";
        os_version_str_ = "10.13 (High Sierra)";
        break;
    case 0x000E: // QSysInfo::MV_10_12 should not be used for compatibility reasons
        os_name_str_    = "macOS";
        os_version_str_ = "10.12 (Sierra)";
        break;
    case 0x000D: // QSysInfo::MV_10_11 should not be used for compatibility reasons
        os_name_str_    = "Mac OS X";
        os_version_str_ = "10.11 (El Capitan)";
        break;
    case 0x000C: // QSysInfo::MV_10_10 should not be used for compatibility reasons
        os_name_str_    = "Mac OS X";
        os_version_str_ = "10.10 (Yosemite)";
        break;
    case 0x000B: // QSysInfo::MV_10_9 should not be used for compatibility reasons
        os_name_str_    = "Mac OS X";
        os_version_str_ = "10.9 (Mavericks)";
        break;
    default:
        os_version_str_ = QSysInfo::productVersion();
        os_name_str_    = QSysInfo::productType();
        os_str_         = QSysInfo::prettyProductName();
    }
#endif
    if (os_str_.isEmpty()) {
        os_str_ = os_name_str_ + " " + os_version_str_;
    }
#endif

#if defined(Q_OS_WIN)
    os_name_str_ = "Windows";
    os_str_      = os_name_str_;
    auto current = QOperatingSystemVersion::current();
    if (IsWindowsServer()) {
        os_name_str_ = "Windows Server";
        os_str_      = os_name_str_;
        switch (current.majorVersion()) {
        case 6:
            switch (current.minorVersion()) {
            case 0:
                os_version_str_ = "2008";
                break;
            case 1:
                os_version_str_ = "2008 R2";
                break;
            case 2:
                os_version_str_ = "2012";
                break;
            case 3:
                os_version_str_ = "2012 R2";
                break;
            }
            break;
        case 10:
            if (current.microVersion() >= 20348)
                os_version_str_ = "2022";
            else if (current.microVersion() >= 17763)
                os_version_str_ = "2019";
            else
                os_version_str_ = "2016";
            break;
        }
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        if (current >= QOperatingSystemVersion::Windows11) {
            os_version_str_ = "11";
        } else
#endif
            if (current >= QOperatingSystemVersion::Windows10) {
            if (current.microVersion() >= 22000) // Hack to detect Win11
                os_version_str_ = "11";
            else
                os_version_str_ = "10";
        } else if (current >= QOperatingSystemVersion::Windows8_1) {
            os_version_str_ = "8.1";
        } else if (current >= QOperatingSystemVersion::Windows8) {
            os_version_str_ = "8";
        } else if (current >= QOperatingSystemVersion::Windows7) {
            os_version_str_ = "7";
        }
    }
    if (!os_version_str_.isEmpty()) {
        os_str_ += (" " + os_version_str_);
    }
#endif

#if defined(Q_OS_HAIKU)
    os_name_str_    = "Haiku";
    os_str_         = os_name_str_;
    os_version_str_ = "";

    utsname uname_info;
    if (uname(&uname_info) == 0) {
        os_str_         = QLatin1String(uname_info.sysname);
        os_version_str_ = (QLatin1String(uname_info.machine) + " ");
        os_version_str_ += QLatin1String(uname_info.version);
        os_str_ += (" " + os_version_str_);
    }
#endif
}

SystemInfo *SystemInfo::instance()
{
    if (!instance_)
        instance_ = new SystemInfo();
    return instance_;
}

SystemInfo *SystemInfo::instance_ = nullptr;
