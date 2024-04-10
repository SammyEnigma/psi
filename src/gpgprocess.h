/*
 * gpgprocess.h - QProcess wrapper makes it easy to handle gpg
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QProcess>

class GpgProcess : public QProcess {
    Q_OBJECT

public:
    GpgProcess(QObject *parent = nullptr);

    void start(const QStringList &arguments, OpenMode mode = ReadWrite);
    void start(OpenMode mode = ReadWrite);

    bool success() const;
    bool info(QString &message);

private:
    QString        findBin() const;
    static QString m_bin;
};
