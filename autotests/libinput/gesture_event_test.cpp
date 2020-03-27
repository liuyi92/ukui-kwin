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
#include "mock_libinput.h"
#include "../../libinput/device.h"
#include "../../libinput/events.h"

#include <QtTest>

#include <linux/input.h>

Q_DECLARE_METATYPE(libinput_event_type)

using namespace KWin::LibInput;

class TestLibinputGestureEvent : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void testType_data();
    void testType();

    void testStart_data();
    void testStart();

    void testSwipeUpdate();
    void testPinchUpdate();

    void testEnd_data();
    void testEnd();

private:
    libinput_device *m_nativeDevice = nullptr;
    Device *m_device = nullptr;
};

void TestLibinputGestureEvent::init()
{
    m_nativeDevice = new libinput_device;
    m_nativeDevice->pointer = true;
    m_nativeDevice->gestureSupported = true;
    m_nativeDevice->deviceSize = QSizeF(12.5, 13.8);
    m_device = new Device(m_nativeDevice);
}

void TestLibinputGestureEvent::cleanup()
{
    delete m_device;
    m_device = nullptr;

    delete m_nativeDevice;
    m_nativeDevice = nullptr;
}

void TestLibinputGestureEvent::testType_data()
{
    QTest::addColumn<libinput_event_type>("type");

    QTest::newRow("pinch-start") << LIBINPUT_EVENT_GESTURE_PINCH_BEGIN;
    QTest::newRow("pinch-update") << LIBINPUT_EVENT_GESTURE_PINCH_UPDATE;
    QTest::newRow("pinch-end") << LIBINPUT_EVENT_GESTURE_PINCH_END;
    QTest::newRow("swipe-start") << LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN;
    QTest::newRow("swipe-update") << LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE;
    QTest::newRow("swipe-end") << LIBINPUT_EVENT_GESTURE_SWIPE_END;
}

void TestLibinputGestureEvent::testType()
{
    // this test verifies the initialization of a PointerEvent and the parent Event class
    libinput_event_gesture *gestureEvent = new libinput_event_gesture;
    QFETCH(libinput_event_type, type);
    gestureEvent->type = type;
    gestureEvent->device = m_nativeDevice;

    QScopedPointer<Event> event(Event::create(gestureEvent));
    // API of event
    QCOMPARE(event->type(), type);
    QCOMPARE(event->device(), m_device);
    QCOMPARE(event->nativeDevice(), m_nativeDevice);
    QCOMPARE((libinput_event*)(*event.data()), gestureEvent);
    // verify it's a pointer event
    QVERIFY(dynamic_cast<GestureEvent*>(event.data()));
    QCOMPARE((libinput_event_gesture*)(*dynamic_cast<GestureEvent*>(event.data())), gestureEvent);
}

void TestLibinputGestureEvent::testStart_data()
{
    QTest::addColumn<libinput_event_type>("type");

    QTest::newRow("pinch") << LIBINPUT_EVENT_GESTURE_PINCH_BEGIN;
    QTest::newRow("swipe") << LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN;
}

void TestLibinputGestureEvent::testStart()
{
    libinput_event_gesture *gestureEvent = new libinput_event_gesture;
    gestureEvent->device = m_nativeDevice;
    QFETCH(libinput_event_type, type);
    gestureEvent->type = type;
    gestureEvent->fingerCount = 3;
    gestureEvent->time = 100u;

    QScopedPointer<Event> event(Event::create(gestureEvent));
    auto ge = dynamic_cast<GestureEvent*>(event.data());
    QVERIFY(ge);
    QCOMPARE(ge->fingerCount(), gestureEvent->fingerCount);
    QVERIFY(!ge->isCancelled());
    QCOMPARE(ge->time(), gestureEvent->time);
    QCOMPARE(ge->delta(), QSizeF(0, 0));
    if (ge->type() == LIBINPUT_EVENT_GESTURE_PINCH_BEGIN) {
        auto pe = dynamic_cast<PinchGestureEvent*>(event.data());
        QCOMPARE(pe->scale(), 1.0);
        QCOMPARE(pe->angleDelta(), 0.0);
    }
}

void TestLibinputGestureEvent::testSwipeUpdate()
{
    libinput_event_gesture *gestureEvent = new libinput_event_gesture;
    gestureEvent->device = m_nativeDevice;
    gestureEvent->type = LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE;
    gestureEvent->fingerCount = 2;
    gestureEvent->time = 200u;
    gestureEvent->delta = QSizeF(2, 3);

    QScopedPointer<Event> event(Event::create(gestureEvent));
    auto se = dynamic_cast<SwipeGestureEvent*>(event.data());
    QVERIFY(se);
    QCOMPARE(se->fingerCount(), gestureEvent->fingerCount);
    QVERIFY(!se->isCancelled());
    QCOMPARE(se->time(), gestureEvent->time);
    QCOMPARE(se->delta(), QSizeF(2, 3));
}

void TestLibinputGestureEvent::testPinchUpdate()
{
    libinput_event_gesture *gestureEvent = new libinput_event_gesture;
    gestureEvent->device = m_nativeDevice;
    gestureEvent->type = LIBINPUT_EVENT_GESTURE_PINCH_UPDATE;
    gestureEvent->fingerCount = 4;
    gestureEvent->time = 600u;
    gestureEvent->delta = QSizeF(5, 4);
    gestureEvent->scale = 2;
    gestureEvent->angleDelta = -30;

    QScopedPointer<Event> event(Event::create(gestureEvent));
    auto pe = dynamic_cast<PinchGestureEvent*>(event.data());
    QVERIFY(pe);
    QCOMPARE(pe->fingerCount(), gestureEvent->fingerCount);
    QVERIFY(!pe->isCancelled());
    QCOMPARE(pe->time(), gestureEvent->time);
    QCOMPARE(pe->delta(), QSizeF(5, 4));
    QCOMPARE(pe->scale(), gestureEvent->scale);
    QCOMPARE(pe->angleDelta(), gestureEvent->angleDelta);
}

void TestLibinputGestureEvent::testEnd_data()
{
    QTest::addColumn<libinput_event_type>("type");
    QTest::addColumn<bool>("cancelled");

    QTest::newRow("pinch/not cancelled") << LIBINPUT_EVENT_GESTURE_PINCH_END << false;
    QTest::newRow("pinch/cancelled") << LIBINPUT_EVENT_GESTURE_PINCH_END << true;
    QTest::newRow("swipe/not cancelled") << LIBINPUT_EVENT_GESTURE_SWIPE_END << false;
    QTest::newRow("swipe/cancelled") << LIBINPUT_EVENT_GESTURE_SWIPE_END << true;
}

void TestLibinputGestureEvent::testEnd()
{
    libinput_event_gesture *gestureEvent = new libinput_event_gesture;
    gestureEvent->device = m_nativeDevice;
    QFETCH(libinput_event_type, type);
    gestureEvent->type = type;
    gestureEvent->fingerCount = 4;
    QFETCH(bool, cancelled);
    gestureEvent->cancelled = cancelled;
    gestureEvent->time = 300u;
    gestureEvent->scale = 3;

    QScopedPointer<Event> event(Event::create(gestureEvent));
    auto ge = dynamic_cast<GestureEvent*>(event.data());
    QVERIFY(ge);
    QCOMPARE(ge->fingerCount(), gestureEvent->fingerCount);
    QCOMPARE(ge->isCancelled(), cancelled);
    QCOMPARE(ge->time(), gestureEvent->time);
    QCOMPARE(ge->delta(), QSizeF(0, 0));
    if (ge->type() == LIBINPUT_EVENT_GESTURE_PINCH_END) {
        auto pe = dynamic_cast<PinchGestureEvent*>(event.data());
        QCOMPARE(pe->scale(), gestureEvent->scale);
        QCOMPARE(pe->angleDelta(), 0.0);
    }
}

QTEST_GUILESS_MAIN(TestLibinputGestureEvent)
#include "gesture_event_test.moc"
