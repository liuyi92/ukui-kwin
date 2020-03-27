/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Rivo Laks <rivolaks@hot.ee>
Copyright (C) 2010 Jorge Mata <matamax123@gmail.com>

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

#include <QAction>
#include <config-ukui-kwin.h>
#include <kwineffects_interface.h>

#include <KLocalizedString>
#include <KActionCollection>
#include <KAboutData>
#include <KGlobalAccel>
#include <KPluginFactory>

#include <QVBoxLayout>
#include <QLabel>

#include "trackmouse_config.h"

// KConfigSkeleton
#include "trackmouseconfig.h"

K_PLUGIN_FACTORY_WITH_JSON(TrackMouseEffectConfigFactory,
                           "trackmouse_config.json",
                           registerPlugin<KWin::TrackMouseEffectConfig>();)

namespace KWin
{

static const QString s_toggleTrackMouseActionName = QStringLiteral("TrackMouse");

TrackMouseEffectConfigForm::TrackMouseEffectConfigForm(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

TrackMouseEffectConfig::TrackMouseEffectConfig(QWidget* parent, const QVariantList& args) :
    KCModule(KAboutData::pluginData(QStringLiteral("trackmouse")), parent, args)
{
    TrackMouseConfig::instance(UKUI_KWIN_CONFIG);
    m_ui = new TrackMouseEffectConfigForm(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_ui);

    addConfig(TrackMouseConfig::self(), m_ui);

    m_actionCollection = new KActionCollection(this, QStringLiteral("kwin"));
    m_actionCollection->setComponentDisplayName(i18n("KWin"));
    m_actionCollection->setConfigGroup(QStringLiteral("TrackMouse"));
    m_actionCollection->setConfigGlobal(true);

    QAction *a = m_actionCollection->addAction(s_toggleTrackMouseActionName);
    a->setText(i18n("Track mouse"));
    a->setProperty("isConfigurationAction", true);

    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>());
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>());

    connect(m_ui->shortcut, &KKeySequenceWidget::keySequenceChanged,
            this, &TrackMouseEffectConfig::shortcutChanged);

    load();
}

TrackMouseEffectConfig::~TrackMouseEffectConfig()
{
}

void TrackMouseEffectConfig::load()
{
    KCModule::load();

    if (QAction *a = m_actionCollection->action(s_toggleTrackMouseActionName)) {
        auto shortcuts = KGlobalAccel::self()->shortcut(a);
        if (!shortcuts.isEmpty()) {
            m_ui->shortcut->setKeySequence(shortcuts.first());
        }
    }
}

void TrackMouseEffectConfig::save()
{
    KCModule::save();
    m_actionCollection->writeSettings();
    OrgUkuiKwinEffectsInterface interface(QStringLiteral("org.ukui.KWin"),
                                         QStringLiteral("/Effects"),
                                         QDBusConnection::sessionBus());
    interface.reconfigureEffect(QStringLiteral("trackmouse"));
}

void TrackMouseEffectConfig::defaults()
{
    KCModule::defaults();
    m_ui->shortcut->clearKeySequence();
}

void TrackMouseEffectConfig::shortcutChanged(const QKeySequence &seq)
{
    if (QAction *a = m_actionCollection->action(QStringLiteral("TrackMouse"))) {
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << seq, KGlobalAccel::NoAutoloading);
    }
    emit changed(true);
}

} // namespace

#include "trackmouse_config.moc"
