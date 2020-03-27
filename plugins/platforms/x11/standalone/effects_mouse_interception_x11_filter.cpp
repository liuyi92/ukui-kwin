/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2017 Martin Flöser <mgraesslin@kde.org>

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
#include "effects_mouse_interception_x11_filter.h"
#include "effects.h"
#include "utils.h"

#include <QMouseEvent>

namespace KWin
{

EffectsMouseInterceptionX11Filter::EffectsMouseInterceptionX11Filter(xcb_window_t window, EffectsHandlerImpl *effects)
    : X11EventFilter(QVector<int>{XCB_BUTTON_PRESS, XCB_BUTTON_RELEASE, XCB_MOTION_NOTIFY})
    , m_effects(effects)
    , m_window(window)
{
}

bool EffectsMouseInterceptionX11Filter::event(xcb_generic_event_t *event)
{
    const uint8_t eventType = event->response_type & ~0x80;
    if (eventType == XCB_BUTTON_PRESS || eventType == XCB_BUTTON_RELEASE) {
        auto *me = reinterpret_cast<xcb_button_press_event_t*>(event);
        if (m_window == me->event) {
            const bool isWheel = me->detail >= 4 && me->detail <= 7;
            if (isWheel) {
                if (eventType != XCB_BUTTON_PRESS) {
                    return false;
                }
                QPoint angleDelta;
                switch (me->detail) {
                case 4:
                    angleDelta.setY(120);
                    break;
                case 5:
                    angleDelta.setY(-120);
                    break;
                case 6:
                    angleDelta.setX(120);
                    break;
                case 7:
                    angleDelta.setX(-120);
                    break;
                }

                const Qt::MouseButtons buttons = x11ToQtMouseButtons(me->state);
                const Qt::KeyboardModifiers modifiers = x11ToQtKeyboardModifiers(me->state);

                if (modifiers & Qt::AltModifier) {
                    int x = angleDelta.x();
                    int y = angleDelta.y();

                    angleDelta.setX(y);
                    angleDelta.setY(x);
                    // After Qt > 5.14 simplify to
                    // angleDelta = angleDelta.transposed();
                }

                if (angleDelta.y()) {
                    QWheelEvent ev(QPoint(me->event_x, me->event_y), angleDelta.y(), buttons, modifiers, Qt::Vertical);
                    return m_effects->checkInputWindowEvent(&ev);
                } else if (angleDelta.x()) {
                    QWheelEvent ev(QPoint(me->event_x, me->event_y), angleDelta.x(), buttons, modifiers, Qt::Horizontal);
                    return m_effects->checkInputWindowEvent(&ev);
                }
            }
            const Qt::MouseButton button = x11ToQtMouseButton(me->detail);
            Qt::MouseButtons buttons = x11ToQtMouseButtons(me->state);
            const QEvent::Type type = (eventType == XCB_BUTTON_PRESS) ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
            if (type == QEvent::MouseButtonPress) {
                buttons |= button;
            } else {
                buttons &= ~button;
            }
            QMouseEvent ev(type, QPoint(me->event_x, me->event_y), QPoint(me->root_x, me->root_y),
                           button, buttons, x11ToQtKeyboardModifiers(me->state));
            return m_effects->checkInputWindowEvent(&ev);
        }
    } else if (eventType == XCB_MOTION_NOTIFY) {
        const auto *me = reinterpret_cast<xcb_motion_notify_event_t*>(event);
        if (m_window == me->event) {
            QMouseEvent ev(QEvent::MouseMove, QPoint(me->event_x, me->event_y), QPoint(me->root_x, me->root_y),
                           Qt::NoButton, x11ToQtMouseButtons(me->state), x11ToQtKeyboardModifiers(me->state));
            return m_effects->checkInputWindowEvent(&ev);
        }
    }
    return false;
}

}
