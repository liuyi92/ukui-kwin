/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright (C) 2016 Martin Gräßlin <mgraesslin@kde.org>

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
#include "cursor.h"
#include "effectloader.h"
#include "platform.h"
#include "wayland_server.h"
#include "workspace.h"
#include "scripting/scripting.h"
#include "effect_builtins.h"

#define private public
#include "screenedge.h"
#undef private

#include <KConfigGroup>

Q_DECLARE_METATYPE(KWin::ElectricBorder)

using namespace KWin;

static const QString s_socketName = QStringLiteral("wayland_test_kwin_scripting_screenedge-0");

class ScreenEdgeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testEdge_data();
    void testEdge();
    void testTouchEdge_data();
    void testTouchEdge();
    void testEdgeUnregister();
    void testDeclarativeTouchEdge();

private:
    void triggerConfigReload();
};

void ScreenEdgeTest::initTestCase()
{
    QSignalSpy workspaceCreatedSpy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspaceCreatedSpy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    QVERIFY(waylandServer()->init(s_socketName.toLocal8Bit()));

    // empty config to have defaults
    auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);

    // disable all effects to prevent them grabbing edges
    KConfigGroup plugins(config, QStringLiteral("Plugins"));
    ScriptedEffectLoader loader;
    const auto builtinNames = BuiltInEffects::availableEffectNames() << loader.listOfKnownEffects();
    for (QString name : builtinNames) {
        plugins.writeEntry(name + QStringLiteral("Enabled"), false);
    }

    // disable electric border pushback
    config->group("Windows").writeEntry("ElectricBorderPushbackPixels", 0);
    config->group("TabBox").writeEntry("TouchBorderActivate", int(ElectricNone));

    config->sync();
    kwinApp()->setConfig(config);

    kwinApp()->start();
    QVERIFY(workspaceCreatedSpy.wait());
    QVERIFY(Scripting::self());

    ScreenEdges::self()->setTimeThreshold(0);
    ScreenEdges::self()->setReActivationThreshold(0);
}

void ScreenEdgeTest::init()
{
    KWin::Cursor::setPos(640, 512);
    if (workspace()->showingDesktop()) {
        workspace()->slotToggleShowDesktop();
    }
    QVERIFY(!workspace()->showingDesktop());
}

void ScreenEdgeTest::cleanup()
{
    // try to unload the script
    const QStringList scripts = {QFINDTESTDATA("./scripts/screenedge.js"), QFINDTESTDATA("./scripts/screenedgeunregister.js"), QFINDTESTDATA("./scripts/touchScreenedge.js")};
    for (const QString &script: scripts) {
        if (!script.isEmpty()) {
            if (Scripting::self()->isScriptLoaded(script)) {
                QVERIFY(Scripting::self()->unloadScript(script));
                QTRY_VERIFY(!Scripting::self()->isScriptLoaded(script));
            }
        }
    }
}

void ScreenEdgeTest::testEdge_data()
{
    QTest::addColumn<KWin::ElectricBorder>("edge");
    QTest::addColumn<QPoint>("triggerPos");

    QTest::newRow("Top")      << KWin::ElectricTop << QPoint(512, 0);
    QTest::newRow("TopRight") << KWin::ElectricTopRight << QPoint(1279, 0);
    QTest::newRow("Right")    << KWin::ElectricRight << QPoint(1279, 512);
    QTest::newRow("BottomRight") << KWin::ElectricBottomRight << QPoint(1279, 1023);
    QTest::newRow("Bottom") << KWin::ElectricBottom << QPoint(512, 1023);
    QTest::newRow("BottomLeft") << KWin::ElectricBottomLeft << QPoint(0, 1023);
    QTest::newRow("Left") << KWin::ElectricLeft << QPoint(0, 512);
    QTest::newRow("TopLeft") << KWin::ElectricTopLeft << QPoint(0, 0);

    //repeat a row to show previously unloading and re-registering works
    QTest::newRow("Top")      << KWin::ElectricTop << QPoint(512, 0);
}

void ScreenEdgeTest::testEdge()
{
    const QString scriptToLoad = QFINDTESTDATA("./scripts/screenedge.js");
    QVERIFY(!scriptToLoad.isEmpty());

    // mock the config
    auto config = kwinApp()->config();
    QFETCH(KWin::ElectricBorder, edge);
    config->group(QLatin1String("Script-") + scriptToLoad).writeEntry("Edge", int(edge));
    config->sync();

    QVERIFY(!Scripting::self()->isScriptLoaded(scriptToLoad));
    const int id = Scripting::self()->loadScript(scriptToLoad);
    QVERIFY(id != -1);
    QVERIFY(Scripting::self()->isScriptLoaded(scriptToLoad));
    auto s = Scripting::self()->findScript(scriptToLoad);
    QVERIFY(s);
    QSignalSpy runningChangedSpy(s, &AbstractScript::runningChanged);
    QVERIFY(runningChangedSpy.isValid());
    s->run();
    QVERIFY(runningChangedSpy.wait());
    QCOMPARE(runningChangedSpy.count(), 1);
    QCOMPARE(runningChangedSpy.first().first().toBool(), true);
    // triggering the edge will result in show desktop being triggered
    QSignalSpy showDesktopSpy(workspace(), &Workspace::showingDesktopChanged);
    QVERIFY(showDesktopSpy.isValid());

    // trigger the edge
    QFETCH(QPoint, triggerPos);
    KWin::Cursor::setPos(triggerPos);
    QCOMPARE(showDesktopSpy.count(), 1);
    QVERIFY(workspace()->showingDesktop());
}

void ScreenEdgeTest::testTouchEdge_data()
{
    QTest::addColumn<KWin::ElectricBorder>("edge");
    QTest::addColumn<QPoint>("triggerPos");
    QTest::addColumn<QPoint>("motionPos");

    QTest::newRow("Top")      << KWin::ElectricTop << QPoint(50, 0) << QPoint(50, 500);
    QTest::newRow("Right")    << KWin::ElectricRight << QPoint(1279, 50) << QPoint(500, 50);
    QTest::newRow("Bottom") << KWin::ElectricBottom << QPoint(512, 1023) << QPoint(512, 500);
    QTest::newRow("Left") << KWin::ElectricLeft << QPoint(0, 50) << QPoint(500, 50);

    //repeat a row to show previously unloading and re-registering works
    QTest::newRow("Top")      << KWin::ElectricTop << QPoint(512, 0) << QPoint(512, 500);
}

void ScreenEdgeTest::testTouchEdge()
{
    const QString scriptToLoad = QFINDTESTDATA("./scripts/touchScreenedge.js");
    QVERIFY(!scriptToLoad.isEmpty());

    // mock the config
    auto config = kwinApp()->config();
    QFETCH(KWin::ElectricBorder, edge);
    config->group(QLatin1String("Script-") + scriptToLoad).writeEntry("Edge", int(edge));
    config->sync();

    QVERIFY(!Scripting::self()->isScriptLoaded(scriptToLoad));
    const int id = Scripting::self()->loadScript(scriptToLoad);
    QVERIFY(id != -1);
    QVERIFY(Scripting::self()->isScriptLoaded(scriptToLoad));
    auto s = Scripting::self()->findScript(scriptToLoad);
    QVERIFY(s);
    QSignalSpy runningChangedSpy(s, &AbstractScript::runningChanged);
    QVERIFY(runningChangedSpy.isValid());
    s->run();
    QVERIFY(runningChangedSpy.wait());
    QCOMPARE(runningChangedSpy.count(), 1);
    QCOMPARE(runningChangedSpy.first().first().toBool(), true);
    // triggering the edge will result in show desktop being triggered
    QSignalSpy showDesktopSpy(workspace(), &Workspace::showingDesktopChanged);
    QVERIFY(showDesktopSpy.isValid());

    // trigger the edge
    QFETCH(QPoint, triggerPos);
    quint32 timestamp = 0;
    kwinApp()->platform()->touchDown(0, triggerPos, timestamp++);
    QFETCH(QPoint, motionPos);
    kwinApp()->platform()->touchMotion(0, motionPos, timestamp++);
    kwinApp()->platform()->touchUp(0, timestamp++);
    QVERIFY(showDesktopSpy.wait());
    QCOMPARE(showDesktopSpy.count(), 1);
    QVERIFY(workspace()->showingDesktop());
}

void ScreenEdgeTest::triggerConfigReload() {
    workspace()->slotReconfigure();
}

void ScreenEdgeTest::testEdgeUnregister()
{
    const QString scriptToLoad = QFINDTESTDATA("./scripts/screenedgeunregister.js");
    QVERIFY(!scriptToLoad.isEmpty());

    Scripting::self()->loadScript(scriptToLoad);
    auto s = Scripting::self()->findScript(scriptToLoad);
    auto configGroup = s->config();
    configGroup.writeEntry("Edge", int(KWin::ElectricLeft));
    configGroup.sync();
    const QPoint triggerPos = QPoint(0, 512);

    QSignalSpy runningChangedSpy(s, &AbstractScript::runningChanged);
    s->run();
    QVERIFY(runningChangedSpy.wait());

    QSignalSpy showDesktopSpy(workspace(), &Workspace::showingDesktopChanged);
    QVERIFY(showDesktopSpy.isValid());

    //trigger the edge
    KWin::Cursor::setPos(triggerPos);
    QCOMPARE(showDesktopSpy.count(), 1);

    //reset
    KWin::Cursor::setPos(500,500);
    workspace()->slotToggleShowDesktop();
    showDesktopSpy.clear();

    //trigger again, to show that retriggering works
    KWin::Cursor::setPos(triggerPos);
    QCOMPARE(showDesktopSpy.count(), 1);

    //reset
    KWin::Cursor::setPos(500,500);
    workspace()->slotToggleShowDesktop();
    showDesktopSpy.clear();

    //make the script unregister the edge
    configGroup.writeEntry("mode", "unregister");
    triggerConfigReload();
    KWin::Cursor::setPos(triggerPos);
    QCOMPARE(showDesktopSpy.count(), 0); //not triggered

    //force the script to unregister a non-registered edge to prove it doesn't explode
    triggerConfigReload();
}

void ScreenEdgeTest::testDeclarativeTouchEdge()
{
    const QString scriptToLoad = QFINDTESTDATA("./scripts/screenedgetouch.qml");
    QVERIFY(!scriptToLoad.isEmpty());
    QVERIFY(Scripting::self()->loadDeclarativeScript(scriptToLoad) != -1);
    QVERIFY(Scripting::self()->isScriptLoaded(scriptToLoad));

    auto s = Scripting::self()->findScript(scriptToLoad);
    QSignalSpy runningChangedSpy(s, &AbstractScript::runningChanged);
    s->run();
    QTRY_COMPARE(runningChangedSpy.count(), 1);

    QSignalSpy showDesktopSpy(workspace(), &Workspace::showingDesktopChanged);
    QVERIFY(showDesktopSpy.isValid());

    // Trigger the edge through touch
    quint32 timestamp = 0;
    kwinApp()->platform()->touchDown(0, QPointF(0, 50), timestamp++);
    kwinApp()->platform()->touchMotion(0, QPointF(500, 50), timestamp++);
    kwinApp()->platform()->touchUp(0, timestamp++);

    QVERIFY(showDesktopSpy.wait());
}

WAYLANDTEST_MAIN(ScreenEdgeTest)
#include "screenedge_test.moc"
