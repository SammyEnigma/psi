/*
 * multifiletransfermodel.cpp - model for file transfers
 * Copyright (C) 2019 Sergey Ilinykh
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef MULTIFILETRANSFERMODEL_H
#define MULTIFILETRANSFERMODEL_H

#include <QAbstractListModel>
#include <QSet>
#include <QTimer>

class MultiFileTransferItem;

class MultiFileTransferModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Direction {
        Incoming,
        Outgoing
    };

    enum State {
        AddTemplate,
        Pending,
        Active,
        Failed,
        Done
    };

    enum {
        FullSizeRole = Qt::UserRole,
        CurrentSizeRole,
        SpeedRole,
        DescriptionRole,
        DirectionRole,
        StateRole,
        TimeRemainingRole,
        ErrorStringRole,

        // requests
        RejectFileRole,  // reject trasnfer of specific file
        DeleteFileRole,  // for finished-incoming files only (remove from disk)
        OpenDirRole,
        OpenFileRole
    };

    MultiFileTransferModel(QObject *parent);
    ~MultiFileTransferModel();

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const override;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

    MultiFileTransferItem *addTransfer(Direction direction, const QString &displayName, quint64 fullSize);
private:
    QList<MultiFileTransferItem*> transfers;
    QSet<MultiFileTransferItem*> updatedTransfers;
    QTimer updateTimer;
};

#endif // MULTIFILETRANSFERMODEL_H