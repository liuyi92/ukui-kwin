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
#include "clipboard.h"

#include "databridge.h"
#include "selection_source.h"
#include "transfer.h"
#include "xwayland.h"

#include "x11client.h"
#include "wayland_server.h"
#include "workspace.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/datadevice.h>
#include <KWayland/Client/datasource.h>

#include <KWayland/Server/datadevice_interface.h>
#include <KWayland/Server/datasource_interface.h>
#include <KWayland/Server/seat_interface.h>

#include <xcb/xcb_event.h>
#include <xcb/xfixes.h>

#include <xwayland_logging.h>

namespace KWin
{
namespace Xwl
{

Clipboard::Clipboard(xcb_atom_t atom, QObject *parent)
    : Selection(atom, parent)
{
    xcb_connection_t *xcbConn = kwinApp()->x11Connection();

    const uint32_t clipboardValues[] = { XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                   XCB_EVENT_MASK_PROPERTY_CHANGE };
    xcb_create_window(xcbConn,
                      XCB_COPY_FROM_PARENT,
                      window(),
                      kwinApp()->x11RootWindow(),
                      0, 0,
                      10, 10,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      Xwayland::self()->xcbScreen()->root_visual,
                      XCB_CW_EVENT_MASK,
                      clipboardValues);
    registerXfixes();
    xcb_flush(xcbConn);

    connect(waylandServer()->seat(), &KWayland::Server::SeatInterface::selectionChanged,
            this, &Clipboard::wlSelectionChanged);
}

void Clipboard::wlSelectionChanged(KWayland::Server::DataDeviceInterface *ddi)
{
    if (ddi && ddi != DataBridge::self()->dataDeviceIface()) {
        // Wayland native client provides new selection
        if (!m_checkConnection) {
            m_checkConnection = connect(workspace(), &Workspace::clientActivated,
                                        this, [this](AbstractClient *ac) {
                                            Q_UNUSED(ac);
                                            checkWlSource();
                                        });
        }
        // remove previous source so checkWlSource() can create a new one
        setWlSource(nullptr);
    }
    checkWlSource();
}

void Clipboard::checkWlSource()
{
    auto ddi = waylandServer()->seat()->selection();
    auto removeSource = [this] {
        if (wlSource()) {
            setWlSource(nullptr);
            ownSelection(false);
        }
    };

    // Wayland source gets created when:
    // - the Wl selection exists,
    // - its source is not Xwayland,
    // - a client is active,
    // - this client is an Xwayland one.
    //
    // Otherwise the Wayland source gets destroyed to shield
    // against snooping X clients.

    if (!ddi || DataBridge::self()->dataDeviceIface() == ddi) {
        // Xwayland source or no source
        disconnect(m_checkConnection);
        m_checkConnection = QMetaObject::Connection();
        removeSource();
        return;
    }
    if (!workspace()->activeClient() || !workspace()->activeClient()->inherits("KWin::X11Client")) {
        // no active client or active client is Wayland native
        removeSource();
        return;
    }
    // Xwayland client is active and we need a Wayland source
    if (wlSource()) {
        // source already exists, nothing more to do
        return;
    }
    auto *wls = new WlSource(this, ddi);
    setWlSource(wls);
    auto *dsi = ddi->selection();
    if (dsi) {
        wls->setDataSourceIface(dsi);
    }
    connect(ddi, &KWayland::Server::DataDeviceInterface::selectionChanged,
            wls, &WlSource::setDataSourceIface);
    ownSelection(true);
}

void Clipboard::doHandleXfixesNotify(xcb_xfixes_selection_notify_event_t *event)
{
    createX11Source(nullptr);

    const AbstractClient *client = workspace()->activeClient();
    if (!qobject_cast<const X11Client *>(client)) {
        // clipboard is only allowed to be acquired when Xwayland has focus
        // TODO: can we make this stronger (window id comparison)?
        return;
    }

    createX11Source(event);

    if (X11Source *source = x11Source()) {
        source->getTargets();
    }
}

void Clipboard::x11OffersChanged(const QStringList &added, const QStringList &removed)
{
    X11Source *source = x11Source();
    if (!source) {
        return;
    }

    const Mimes offers = source->offers();

    if (!offers.isEmpty()) {
        if (!source->dataSource() || !removed.isEmpty()) {
            // create new Wl DataSource if there is none or when types
            // were removed (Wl Data Sources can only add types)
            KWayland::Client::DataDeviceManager *dataDeviceManager =
                waylandServer()->internalDataDeviceManager();
            KWayland::Client::DataSource *dataSource =
                dataDeviceManager->createDataSource(source);

            // also offers directly the currently available types
            source->setDataSource(dataSource);
            DataBridge::self()->dataDevice()->setSelection(0, dataSource);
            waylandServer()->seat()->setSelection(DataBridge::self()->dataDeviceIface());
        } else if (auto *dataSource = source->dataSource()) {
            for (const QString &mime : added) {
                dataSource->offer(mime);
            }
        }
    } else {
        waylandServer()->seat()->setSelection(nullptr);
    }

    waylandServer()->internalClientConection()->flush();
    waylandServer()->dispatch();
}

} // namespace Xwl
} // namespace KWin
