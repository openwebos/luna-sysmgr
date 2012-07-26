/****************************************************************************
** Meta object code from reading C++ file 'StatusBar.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBar.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBar[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   11,   10,   10, 0x05,
      70,   63,   10,   10, 0x05,
     105,   63,   10,   10, 0x05,

 // slots: signature, parameters, type, tag, flags
     149,  142,   10,   10, 0x08,
     184,   10,   10,   10, 0x08,
     212,  207,   10,   10, 0x08,
     259,  248,   10,   10, 0x08,
     307,  248,   10,   10, 0x08,
     367,  359,   10,   10, 0x08,
     393,  359,   10,   10, 0x08,
     419,  359,   10,   10, 0x08,
     453,  359,   10,   10, 0x08,
     483,  359,   10,   10, 0x08,
     509,  248,   10,   10, 0x08,
     555,  248,   10,   10, 0x08,
     613,  248,   10,   10, 0x08,
     674,  661,   10,   10, 0x08,
     736,  729,   10,   10, 0x08,
     763,   10,   10,   10, 0x08,
     802,  794,   10,   10, 0x08,
     847,   10,   10,   10, 0x08,
     883,  876,   10,   10, 0x08,
     916,  876,   10,   10, 0x08,
     947,  876,   10,   10, 0x08,
     975,   10,   10,   10, 0x08,
     997,   10,   10,   10, 0x08,
    1027, 1021,   10,   10, 0x08,
    1071,  359,   10,   10, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBar[] = {
    "StatusBar\0\0rightOffset\0"
    "signalDashboardAreaRightEdgeOffset(int)\0"
    "opened\0signalSystemMenuStateChanged(bool)\0"
    "signalDockModeMenuStateChanged(bool)\0"
    "format\0slotTimeFormatChanged(const char*)\0"
    "slotPhoneTypeUpdated()\0text\0"
    "slotCarrierTextChanged(const char*)\0"
    "show,index\0slotRssiIndexChanged(bool,StatusBar::IndexRSSI)\0"
    "slotRssi1xIndexChanged(bool,StatusBar::IndexRSSI1x)\0"
    "enabled\0slotTTYStateChanged(bool)\0"
    "slotHACStateChanged(bool)\0"
    "slotCallForwardStateChanged(bool)\0"
    "slotRoamingStateChanged(bool)\0"
    "slotVpnStateChanged(bool)\0"
    "slotWanIndexChanged(bool,StatusBar::IndexWAN)\0"
    "slotBluetoothIndexChanged(bool,StatusBar::IndexBluetooth)\0"
    "slotWifiIndexChanged(bool,StatusBar::IndexWiFi)\0"
    "rotationLock\0"
    "slotRotationLockChanged(OrientationEvent::Orientation)\0"
    "muteOn\0slotMuteSoundChanged(bool)\0"
    "slotChildBoundingRectChanged()\0visible\0"
    "slotNotificationArealVisibilityChanged(bool)\0"
    "slotBannerMessageActivated()\0active\0"
    "slotNotificationMenuAction(bool)\0"
    "slotSystemMenuMenuAction(bool)\0"
    "slotAppMenuMenuAction(bool)\0"
    "slotBannerActivated()\0slotBannerDeactivated()\0"
    "group\0slotMenuGroupActivated(StatusBarItemGroup*)\0"
    "slotDockModeStatusChanged(bool)\0"
};

void StatusBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBar *_t = static_cast<StatusBar *>(_o);
        switch (_id) {
        case 0: _t->signalDashboardAreaRightEdgeOffset((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->signalSystemMenuStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->signalDockModeMenuStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->slotTimeFormatChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 4: _t->slotPhoneTypeUpdated(); break;
        case 5: _t->slotCarrierTextChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 6: _t->slotRssiIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexRSSI(*)>(_a[2]))); break;
        case 7: _t->slotRssi1xIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexRSSI1x(*)>(_a[2]))); break;
        case 8: _t->slotTTYStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->slotHACStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->slotCallForwardStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->slotRoamingStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->slotVpnStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->slotWanIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexWAN(*)>(_a[2]))); break;
        case 14: _t->slotBluetoothIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexBluetooth(*)>(_a[2]))); break;
        case 15: _t->slotWifiIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexWiFi(*)>(_a[2]))); break;
        case 16: _t->slotRotationLockChanged((*reinterpret_cast< OrientationEvent::Orientation(*)>(_a[1]))); break;
        case 17: _t->slotMuteSoundChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->slotChildBoundingRectChanged(); break;
        case 19: _t->slotNotificationArealVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 20: _t->slotBannerMessageActivated(); break;
        case 21: _t->slotNotificationMenuAction((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 22: _t->slotSystemMenuMenuAction((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 23: _t->slotAppMenuMenuAction((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 24: _t->slotBannerActivated(); break;
        case 25: _t->slotBannerDeactivated(); break;
        case 26: _t->slotMenuGroupActivated((*reinterpret_cast< StatusBarItemGroup*(*)>(_a[1]))); break;
        case 27: _t->slotDockModeStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBar::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBar::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_StatusBar,
      qt_meta_data_StatusBar, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBar::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBar))
        return static_cast<void*>(const_cast< StatusBar*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int StatusBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    }
    return _id;
}

// SIGNAL 0
void StatusBar::signalDashboardAreaRightEdgeOffset(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StatusBar::signalSystemMenuStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void StatusBar::signalDockModeMenuStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
