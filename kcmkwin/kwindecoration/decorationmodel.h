/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KDECORATION_DECORATION_MODEL_H
#define KDECORATION_DECORATION_MODEL_H

#include "utils.h"

#include <QAbstractListModel>

namespace KDecoration2
{

namespace Configuration
{

class DecorationsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum DecorationRole {
        PluginNameRole = Qt::UserRole + 1,
        ThemeNameRole,
        ConfigurationRole,
        RecommendedBorderSizeRole,
    };

public:
    explicit DecorationsModel(QObject *parent = nullptr);
    ~DecorationsModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash< int, QByteArray > roleNames() const override;

    QModelIndex findDecoration(const QString &pluginName, const QString &themeName = QString()) const;

    QStringList knsProviders() const {
        return m_knsProviders;
    }

public Q_SLOTS:
    void init();

private:
    struct Data {
        QString pluginName;
        QString themeName;
        QString visibleName;
        bool configuration = false;
        KDecoration2::BorderSize recommendedBorderSize = KDecoration2::BorderSize::Normal;
    };
    std::vector<Data> m_plugins;
    QStringList m_knsProviders;
};

}
}

#endif
