/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

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

#include "kwin_wayland_test.h"

#include "platform.h"
#include "screens.h"
#include "scripting/scripting.h"
#include "wayland_server.h"
#include "workspace.h"
#include "xdgshellclient.h"

#include <KPackage/PackageLoader>
#include <KWayland/Client/surface.h>

#include <linux/input.h>

namespace KWin
{

static const QString s_socketName = QStringLiteral("wayland_test_minimizeall-0");
static const QString s_scriptName = QStringLiteral("minimizeall");

class MinimizeAllScriptTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testMinimizeUnminimize();
};

void MinimizeAllScriptTest::initTestCase()
{
    qputenv("XDG_DATA_DIRS", QCoreApplication::applicationDirPath().toUtf8());

    qRegisterMetaType<AbstractClient *>();
    qRegisterMetaType<XdgShellClient *>();

    QSignalSpy workspaceCreatedSpy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspaceCreatedSpy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    QVERIFY(waylandServer()->init(s_socketName.toLocal8Bit()));
    QMetaObject::invokeMethod(kwinApp()->platform(), "setVirtualOutputs", Qt::DirectConnection, Q_ARG(int, 2));

    kwinApp()->start();
    QVERIFY(workspaceCreatedSpy.wait());
    QCOMPARE(screens()->count(), 2);
    QCOMPARE(screens()->geometry(0), QRect(0, 0, 1280, 1024));
    QCOMPARE(screens()->geometry(1), QRect(1280, 0, 1280, 1024));
    waylandServer()->initWorkspace();
}

static QString locateMainScript(const QString &pluginName)
{
    const QList<KPluginMetaData> offers = KPackage::PackageLoader::self()->findPackages(
        QStringLiteral("UKUIKWin/Script"),
        QStringLiteral("UKUIkwin/scripts"),
        [&](const KPluginMetaData &metaData) {
            return metaData.pluginId() == pluginName;
        }
    );
    if (offers.isEmpty()) {
        return QString();
    }
    const KPluginMetaData &metaData = offers.first();
    const QString mainScriptFileName = metaData.value(QStringLiteral("X-Plasma-MainScript"));
    const QFileInfo metaDataFileInfo(metaData.fileName());
    return metaDataFileInfo.path() + QLatin1String("/contents/") + mainScriptFileName;
}

void MinimizeAllScriptTest::init()
{
    QVERIFY(Test::setupWaylandConnection());

    Scripting::self()->loadScript(locateMainScript(s_scriptName), s_scriptName);
    QTRY_VERIFY(Scripting::self()->isScriptLoaded(s_scriptName));

    AbstractScript *script = Scripting::self()->findScript(s_scriptName);
    QVERIFY(script);
    QSignalSpy runningChangedSpy(script, &AbstractScript::runningChanged);
    QVERIFY(runningChangedSpy.isValid());
    script->run();
    QTRY_COMPARE(runningChangedSpy.count(), 1);
}

void MinimizeAllScriptTest::cleanup()
{
    Test::destroyWaylandConnection();

    Scripting::self()->unloadScript(s_scriptName);
    QTRY_VERIFY(!Scripting::self()->isScriptLoaded(s_scriptName));
}

void MinimizeAllScriptTest::testMinimizeUnminimize()
{
    // This test verifies that all windows are minimized when Meta+Shift+D
    // is pressed, and unminimized when the shortcut is pressed once again.

    using namespace KWayland::Client;

    // Create a couple of test clients.
    QScopedPointer<Surface> surface1(Test::createSurface());
    QScopedPointer<XdgShellSurface> shellSurface1(Test::createXdgShellStableSurface(surface1.data()));
    XdgShellClient *client1 = Test::renderAndWaitForShown(surface1.data(), QSize(100, 50), Qt::blue);
    QVERIFY(client1);
    QVERIFY(client1->isActive());
    QVERIFY(client1->isMinimizable());

    QScopedPointer<Surface> surface2(Test::createSurface());
    QScopedPointer<XdgShellSurface> shellSurface2(Test::createXdgShellStableSurface(surface2.data()));
    XdgShellClient *client2 = Test::renderAndWaitForShown(surface2.data(), QSize(100, 50), Qt::red);
    QVERIFY(client2);
    QVERIFY(client2->isActive());
    QVERIFY(client2->isMinimizable());

    // Minimize the windows.
    quint32 timestamp = 1;
    kwinApp()->platform()->keyboardKeyPressed(KEY_LEFTMETA, timestamp++);
    kwinApp()->platform()->keyboardKeyPressed(KEY_LEFTSHIFT, timestamp++);
    kwinApp()->platform()->keyboardKeyPressed(KEY_D, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_D, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_LEFTSHIFT, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_LEFTMETA, timestamp++);

    QTRY_VERIFY(client1->isMinimized());
    QTRY_VERIFY(client2->isMinimized());

    // Unminimize the windows.
    kwinApp()->platform()->keyboardKeyPressed(KEY_LEFTMETA, timestamp++);
    kwinApp()->platform()->keyboardKeyPressed(KEY_LEFTSHIFT, timestamp++);
    kwinApp()->platform()->keyboardKeyPressed(KEY_D, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_D, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_LEFTSHIFT, timestamp++);
    kwinApp()->platform()->keyboardKeyReleased(KEY_LEFTMETA, timestamp++);

    QTRY_VERIFY(!client1->isMinimized());
    QTRY_VERIFY(!client2->isMinimized());

    // Destroy test clients.
    shellSurface2.reset();
    QVERIFY(Test::waitForWindowDestroyed(client2));
    shellSurface1.reset();
    QVERIFY(Test::waitForWindowDestroyed(client1));
}

}

WAYLANDTEST_MAIN(KWin::MinimizeAllScriptTest)
#include "minimizeall_test.moc"
