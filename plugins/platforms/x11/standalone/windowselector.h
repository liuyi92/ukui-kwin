/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>
Copyright (C) 2012 Martin Gräßlin <mgraesslin@kde.org>

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

#ifndef KWIN_WINDOWSELECTOR_H
#define KWIN_WINDOWSELECTOR_H

#include "x11eventfilter.h"

#include <xcb/xcb.h>

#include <functional>

class QPoint;

namespace KWin
{
class Toplevel;

class WindowSelector : public X11EventFilter
{
public:

    WindowSelector();
    ~WindowSelector() override;

    void start(std::function<void(KWin::Toplevel*)> callback, const QByteArray &cursorName);
    void start(std::function<void (const QPoint &)> callback);
    bool isActive() const {
        return m_active;
    }
    void processEvent(xcb_generic_event_t *event);

    bool event(xcb_generic_event_t *event) override;

private:
    xcb_cursor_t createCursor(const QByteArray &cursorName);
    void release();
    void selectWindowUnderPointer();
    void handleKeyPress(xcb_keycode_t keycode, uint16_t state);
    void handleButtonRelease(xcb_button_t button, xcb_window_t window);
    void selectWindowId(xcb_window_t window_to_kill);
    bool activate(const QByteArray &cursorName = QByteArray());
    void cancelCallback();
    bool m_active;
    std::function<void(KWin::Toplevel*)> m_callback;
    std::function<void(const QPoint &)> m_pointSelectionFallback;
};

} // namespace

#endif
