/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

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
#ifndef KWIN_MOCK_WORKSPACE_H
#define KWIN_MOCK_WORKSPACE_H

#include <QObject>
#include <kwinglobals.h>

namespace KWin
{

class AbstractClient;
class X11Client;
class X11EventFilter;

class MockWorkspace;
typedef MockWorkspace Workspace;

class MockWorkspace : public QObject
{
    Q_OBJECT
public:
    explicit MockWorkspace(QObject *parent = nullptr);
    ~MockWorkspace() override;
    AbstractClient *activeClient() const;
    AbstractClient *moveResizeClient() const;
    void setShowingDesktop(bool showing);
    bool showingDesktop() const;
    QRect clientArea(clientAreaOption, int screen, int desktop) const;

    void setActiveClient(AbstractClient *c);
    void setMoveResizeClient(AbstractClient *c);

    void registerEventFilter(X11EventFilter *filter);
    void unregisterEventFilter(X11EventFilter *filter);

    bool compositing() const {
        return false;
    }

    static Workspace *self();

Q_SIGNALS:
    void clientRemoved(KWin::X11Client *);

private:
    AbstractClient *m_activeClient;
    AbstractClient *m_moveResizeClient;
    bool m_showingDesktop;
    static Workspace *s_self;
};

inline
Workspace *MockWorkspace::self()
{
    return s_self;
}

inline Workspace *workspace()
{
    return Workspace::self();
}

}

#endif
