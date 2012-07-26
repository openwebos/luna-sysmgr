/****************************************************************************
** Meta object code from reading C++ file 'WindowServer.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/WindowServer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WindowServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Runtime[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       2,   59, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x05,
      30,    8,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
      51,    8,    8,    8, 0x0a,

 // methods: signature, parameters, type, tag, flags
      99,   94,   86,    8, 0x02,
     127,    8,   86,    8, 0x02,
     145,    8,   86,    8, 0x02,
     165,    8,   86,    8, 0x02,
     188,    8,  184,    8, 0x02,
     207,    8,  202,    8, 0x02,

 // properties: name, type, flags
     225,  184, 0x02495001,
     237,  202, 0x01495001,

 // properties: notify_signal_id
       0,
       1,

       0        // eod
};

static const char qt_meta_stringdata_Runtime[] = {
    "Runtime\0\0orientationChanged()\0"
    "clockFormatChanged()\0"
    "slotTimeFormatChanged(const char*)\0"
    "QString\0text\0getLocalizedString(QString)\0"
    "getLocalizedDay()\0getLocalizedMonth()\0"
    "getLocalizedAMPM()\0int\0orientation()\0"
    "bool\0twelveHourClock()\0orientation\0"
    "twelveHourClock\0"
};

void Runtime::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Runtime *_t = static_cast<Runtime *>(_o);
        switch (_id) {
        case 0: _t->orientationChanged(); break;
        case 1: _t->clockFormatChanged(); break;
        case 2: _t->slotTimeFormatChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 3: { QString _r = _t->getLocalizedString((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 4: { QString _r = _t->getLocalizedDay();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 5: { QString _r = _t->getLocalizedMonth();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 6: { QString _r = _t->getLocalizedAMPM();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 7: { int _r = _t->orientation();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 8: { bool _r = _t->twelveHourClock();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Runtime::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Runtime::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Runtime,
      qt_meta_data_Runtime, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Runtime::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Runtime::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Runtime::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Runtime))
        return static_cast<void*>(const_cast< Runtime*>(this));
    return QObject::qt_metacast(_clname);
}

int Runtime::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = orientation(); break;
        case 1: *reinterpret_cast< bool*>(_v) = twelveHourClock(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void Runtime::orientationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void Runtime::clockFormatChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
static const uint qt_meta_data_WindowServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x05,
      43,   13,   13,   13, 0x05,
      73,   13,   13,   13, 0x05,
      96,   13,   13,   13, 0x05,
     114,   13,   13,   13, 0x05,

 // slots: signature, parameters, type, tag, flags
     148,   13,   13,   13, 0x08,
     192,  179,   13,   13, 0x08,
     247,   13,   13,   13, 0x08,
     280,   13,   13,   13, 0x08,
     313,  307,   13,   13, 0x08,
     344,   13,   13,   13, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_WindowServer[] = {
    "WindowServer\0\0signalWindowUpdated(Window*)\0"
    "signalAboutToTakeScreenShot()\0"
    "signalTookScreenShot()\0signalUiRotated()\0"
    "signalTouchesReleasedFromScreen()\0"
    "slotResizePendingTimerTicked()\0"
    "rotationLock\0"
    "slotRotationLockChanged(OrientationEvent::Orientation)\0"
    "slotProgressAnimationCompleted()\0"
    "slotRotationAnimFinished()\0value\0"
    "rotationValueChanged(QVariant)\0"
    "slotDeferredNewOrientation()\0"
};

void WindowServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        WindowServer *_t = static_cast<WindowServer *>(_o);
        switch (_id) {
        case 0: _t->signalWindowUpdated((*reinterpret_cast< Window*(*)>(_a[1]))); break;
        case 1: _t->signalAboutToTakeScreenShot(); break;
        case 2: _t->signalTookScreenShot(); break;
        case 3: _t->signalUiRotated(); break;
        case 4: _t->signalTouchesReleasedFromScreen(); break;
        case 5: _t->slotResizePendingTimerTicked(); break;
        case 6: _t->slotRotationLockChanged((*reinterpret_cast< OrientationEvent::Orientation(*)>(_a[1]))); break;
        case 7: _t->slotProgressAnimationCompleted(); break;
        case 8: _t->slotRotationAnimFinished(); break;
        case 9: _t->rotationValueChanged((*reinterpret_cast< const QVariant(*)>(_a[1]))); break;
        case 10: _t->slotDeferredNewOrientation(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData WindowServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject WindowServer::staticMetaObject = {
    { &QGraphicsView::staticMetaObject, qt_meta_stringdata_WindowServer,
      qt_meta_data_WindowServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &WindowServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *WindowServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *WindowServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WindowServer))
        return static_cast<void*>(const_cast< WindowServer*>(this));
    return QGraphicsView::qt_metacast(_clname);
}

int WindowServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void WindowServer::signalWindowUpdated(Window * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void WindowServer::signalAboutToTakeScreenShot()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void WindowServer::signalTookScreenShot()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void WindowServer::signalUiRotated()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void WindowServer::signalTouchesReleasedFromScreen()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
