/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2017, 2018 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/


#ifndef SLIDE_CONFIG_H
#define SLIDE_CONFIG_H

#include <KCModule>
#include "ui_slide_config.h"

namespace KWin
{

class SlideEffectConfig : public KCModule
{
    Q_OBJECT

public:
    explicit SlideEffectConfig(QWidget *parent = nullptr, const QVariantList &args = QVariantList());
    ~SlideEffectConfig() override;

    void save() override;

private:
    ::Ui::SlideEffectConfig m_ui;
};

} // namespace KWin

#endif
