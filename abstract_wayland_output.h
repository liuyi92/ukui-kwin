/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright 2019 Roman Gilg <subdiff@gmail.com>

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
#ifndef KWIN_ABSTRACT_WAYLAND_OUTPUT_H
#define KWIN_ABSTRACT_WAYLAND_OUTPUT_H

#include "abstract_output.h"
#include <utils.h>
#include <ukui-kwin_export.h>

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QVector>

#include <KWayland/Server/output_interface.h>
#include <KWayland/Server/outputdevice_interface.h>

namespace KWayland
{
namespace Server
{
class OutputInterface;
class OutputDeviceInterface;
class OutputChangeSet;
class OutputManagementInterface;
class XdgOutputInterface;
}
}

namespace KWin
{

/**
 * Generic output representation in a Wayland session
 */
class UKUI_KWIN_EXPORT AbstractWaylandOutput : public AbstractOutput
{
    Q_OBJECT
public:
    enum class Transform {
        Normal,
        Rotated90,
        Rotated180,
        Rotated270,
        Flipped,
        Flipped90,
        Flipped180,
        Flipped270
    };

    explicit AbstractWaylandOutput(QObject *parent = nullptr);
    ~AbstractWaylandOutput() override;

    QString name() const override;
    QByteArray uuid() const override;

    QSize pixelSize() const;
    qreal scale() const override;

    /**
     * The geometry of this output in global compositor co-ordinates (i.e scaled)
     */
    QRect geometry() const override;
    QSize physicalSize() const override;

    /**
     * Current refresh rate in 1/ms.
     */
    int refreshRate() const override;

    bool isInternal() const override {
        return m_internal;
    }

    void setGlobalPos(const QPoint &pos);
    void setScale(qreal scale);

    void applyChanges(const KWayland::Server::OutputChangeSet *changeSet) override;

    QPointer<KWayland::Server::OutputInterface> waylandOutput() const {
        return m_waylandOutput;
    }

    bool isEnabled() const;
    /**
     * Enable or disable the output.
     *
     * This differs from updateDpms as it also removes the wl_output.
     * The default is on.
     */
    void setEnabled(bool enable) override;

Q_SIGNALS:
    void modeChanged();

protected:
    void initInterfaces(const QString &model, const QString &manufacturer,
                        const QByteArray &uuid, const QSize &physicalSize,
                        const QVector<KWayland::Server::OutputDeviceInterface::Mode> &modes);

    QPointer<KWayland::Server::XdgOutputInterface> xdgOutput() const {
        return m_xdgOutput;
    }

    QPointer<KWayland::Server::OutputDeviceInterface> waylandOutputDevice() const {
        return m_waylandOutputDevice;
    }

    QPoint globalPos() const;

    bool internal() const {
        return m_internal;
    }
    void setInternal(bool set) {
        m_internal = set;
    }
    void setDpmsSupported(bool set) {
        m_supportsDpms = set;
    }

    virtual void updateEnablement(bool enable) {
        Q_UNUSED(enable);
    }
    virtual void updateDpms(KWayland::Server::OutputInterface::DpmsMode mode) {
        Q_UNUSED(mode);
    }
    virtual void updateMode(int modeIndex) {
        Q_UNUSED(modeIndex);
    }
    virtual void updateTransform(Transform transform) {
        Q_UNUSED(transform);
    }

    void setWaylandMode(const QSize &size, int refreshRate);
    void setTransform(Transform transform);

    QSize orientateSize(const QSize &size) const;

    /**
     * Returns the orientation of this output.
     *
     * - Flipped along the vertical axis is landscape + inv. portrait.
     * - Rotated 90° and flipped along the horizontal axis is portrait + inv. landscape
     * - Rotated 180° and flipped along the vertical axis is inv. landscape + inv. portrait
     * - Rotated 270° and flipped along the horizontal axis is inv. portrait + inv. landscape +
     *   portrait
     */
    Transform transform() const;

private:
    void createWaylandOutput();
    void createXdgOutput();

    void setTransform(KWayland::Server::OutputDeviceInterface::Transform transform);

    QPointer<KWayland::Server::OutputInterface> m_waylandOutput;
    QPointer<KWayland::Server::XdgOutputInterface> m_xdgOutput;
    QPointer<KWayland::Server::OutputDeviceInterface> m_waylandOutputDevice;

    KWayland::Server::OutputInterface::DpmsMode m_dpms = KWayland::Server::OutputInterface::DpmsMode::On;

    bool m_internal = false;
    bool m_supportsDpms = false;
};

}

#endif // KWIN_OUTPUT_H
