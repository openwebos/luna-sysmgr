/****************************************************************************
** Meta object code from reading C++ file 'SystemMenu.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/SystemMenu.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SystemMenu.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SystemMenu[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      39,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      30,   11,   11,   11, 0x08,
      52,   11,   11,   11, 0x08,
      73,   11,   11,   11, 0x08,
      94,   11,   11,   11, 0x08,
     119,   11,   11,   11, 0x08,
     189,  144,   11,   11, 0x08,
     246,   11,   11,   11, 0x08,
     272,   11,   11,   11, 0x08,
     298,   11,   11,   11, 0x08,
     328,   11,   11,   11, 0x08,
     364,  358,   11,   11, 0x08,
     397,   11,   11,   11, 0x08,
     417,   11,   11,   11, 0x08,
     437,   11,   11,   11, 0x08,
     482,  461,   11,   11, 0x08,
     530,   11,   11,   11, 0x08,
     567,  558,   11,   11, 0x08,
     607,  599,   11,   11, 0x08,
     650,  637,   11,   11, 0x08,
     712,  705,   11,   11, 0x08,
     750,  739,   11,   11, 0x08,
     797,  787,   11,   11, 0x08,
     847,  836,   11,   11, 0x08,
     920,  876,   11,   11, 0x08,
     993,  976,   11,   11, 0x08,
    1053,   11,   11,   11, 0x08,
    1088, 1077,   11,   11, 0x08,
    1156, 1133,   11,   11, 0x08,
    1227, 1204,   11,   11, 0x08,
    1295, 1285,   11,   11, 0x08,
    1349, 1336,   11,   11, 0x08,
    1418, 1401,   11,   11, 0x08,
    1470, 1462,   11,   11, 0x08,
    1502, 1496,   11,   11, 0x08,
    1556, 1545,   11,   11, 0x08,
    1594,   11,   11,   11, 0x08,
    1623, 1618,   11,   11, 0x08,
    1662,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_SystemMenu[] = {
    "SystemMenu\0\0signalCloseMenu()\0"
    "slotCloseSystemMenu()\0slotWifiMenuOpened()\0"
    "slotWifiMenuClosed()\0slotWifiOnOffTriggered()\0"
    "slotWifiPrefsTriggered()\0"
    "index,name,profileId,securityType,connStatus\0"
    "slotWifiNetworkSelected(int,QString,int,QString,QString)\0"
    "slotBluetoothMenuOpened()\0"
    "slotBluetoothMenuClosed()\0"
    "slotBluetoothOnOffTriggered()\0"
    "slotBluetoothPrefsTriggered()\0index\0"
    "slotBluetoothDeviceSelected(int)\0"
    "slotVpnMenuOpened()\0slotVpnMenuClosed()\0"
    "slotVpnPrefsTriggered()\0name,status,profInfo\0"
    "slotVpnNetworkSelected(QString,QString,QString)\0"
    "slotAirplaneModeTriggered()\0isLocked\0"
    "slotRotationLockTriggered(bool)\0isMuted\0"
    "slotMuteToggleTriggered(bool)\0"
    "rotationLock\0"
    "slotRotationLockChanged(OrientationEvent::Orientation)\0"
    "muteOn\0slotMuteSoundChanged(bool)\0"
    "brightness\0slotDisplayMaxBrightnessChanged(int)\0"
    "connected\0slotPowerdConnectionStateChanged(bool)\0"
    "percentage\0slotBatteryLevelUpdated(int)\0"
    "wifiOn,wifiConnected,wifiSSID,wifiConnState\0"
    "slotWifiStateChanged(bool,bool,std::string,std::string)\0"
    "numNetworks,list\0"
    "slotWifiAvailableNetworksListUpdate(int,t_wifiAccessPoint*)\0"
    "slotBluetoothTurnedOn()\0radioState\0"
    "slotBluetoothPowerStateChanged(t_radioState)\0"
    "btConnected,deviceName\0"
    "slotBluetoothConnStateChanged(bool,std::string)\0"
    "numTrustedDevices,list\0"
    "slotBluetoothTrustedDevicesUpdate(int,t_bluetoothDevice*)\0"
    "available\0slotBluetoothParedDevicesAvailable(bool)\0"
    "deviceStatus\0"
    "slotBluetoothUpdateDeviceStatus(t_bluetoothDevice*)\0"
    "numProfiles,list\0"
    "slotVpnProfileListUpdate(int,t_vpnProfile*)\0"
    "enabled\0slotVpnStateChanged(bool)\0"
    "state\0slotAirplaneModeState(t_airplaneModeState)\0"
    "value,save\0slotMenuBrightnessChanged(qreal,bool)\0"
    "slotSystemTimeChanged()\0rect\0"
    "slotPositiveSpaceChangeFinished(QRect)\0"
    "tick()\0"
};

void SystemMenu::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SystemMenu *_t = static_cast<SystemMenu *>(_o);
        switch (_id) {
        case 0: _t->signalCloseMenu(); break;
        case 1: _t->slotCloseSystemMenu(); break;
        case 2: _t->slotWifiMenuOpened(); break;
        case 3: _t->slotWifiMenuClosed(); break;
        case 4: _t->slotWifiOnOffTriggered(); break;
        case 5: _t->slotWifiPrefsTriggered(); break;
        case 6: _t->slotWifiNetworkSelected((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< QString(*)>(_a[5]))); break;
        case 7: _t->slotBluetoothMenuOpened(); break;
        case 8: _t->slotBluetoothMenuClosed(); break;
        case 9: _t->slotBluetoothOnOffTriggered(); break;
        case 10: _t->slotBluetoothPrefsTriggered(); break;
        case 11: _t->slotBluetoothDeviceSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->slotVpnMenuOpened(); break;
        case 13: _t->slotVpnMenuClosed(); break;
        case 14: _t->slotVpnPrefsTriggered(); break;
        case 15: _t->slotVpnNetworkSelected((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3]))); break;
        case 16: _t->slotAirplaneModeTriggered(); break;
        case 17: _t->slotRotationLockTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->slotMuteToggleTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 19: _t->slotRotationLockChanged((*reinterpret_cast< OrientationEvent::Orientation(*)>(_a[1]))); break;
        case 20: _t->slotMuteSoundChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 21: _t->slotDisplayMaxBrightnessChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 22: _t->slotPowerdConnectionStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 23: _t->slotBatteryLevelUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 24: _t->slotWifiStateChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< std::string(*)>(_a[3])),(*reinterpret_cast< std::string(*)>(_a[4]))); break;
        case 25: _t->slotWifiAvailableNetworksListUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_wifiAccessPoint*(*)>(_a[2]))); break;
        case 26: _t->slotBluetoothTurnedOn(); break;
        case 27: _t->slotBluetoothPowerStateChanged((*reinterpret_cast< t_radioState(*)>(_a[1]))); break;
        case 28: _t->slotBluetoothConnStateChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< std::string(*)>(_a[2]))); break;
        case 29: _t->slotBluetoothTrustedDevicesUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_bluetoothDevice*(*)>(_a[2]))); break;
        case 30: _t->slotBluetoothParedDevicesAvailable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 31: _t->slotBluetoothUpdateDeviceStatus((*reinterpret_cast< t_bluetoothDevice*(*)>(_a[1]))); break;
        case 32: _t->slotVpnProfileListUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_vpnProfile*(*)>(_a[2]))); break;
        case 33: _t->slotVpnStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 34: _t->slotAirplaneModeState((*reinterpret_cast< t_airplaneModeState(*)>(_a[1]))); break;
        case 35: _t->slotMenuBrightnessChanged((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 36: _t->slotSystemTimeChanged(); break;
        case 37: _t->slotPositiveSpaceChangeFinished((*reinterpret_cast< QRect(*)>(_a[1]))); break;
        case 38: _t->tick(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SystemMenu::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SystemMenu::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_SystemMenu,
      qt_meta_data_SystemMenu, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SystemMenu::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SystemMenu::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SystemMenu::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SystemMenu))
        return static_cast<void*>(const_cast< SystemMenu*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int SystemMenu::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 39)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 39;
    }
    return _id;
}

// SIGNAL 0
void SystemMenu::signalCloseMenu()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_MenuHandler[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   13,   12,   12, 0x05,
      73,   55,   12,   12, 0x05,
     104,   12,   12,   12, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_MenuHandler[] = {
    "MenuHandler\0\0batteryLevel\0"
    "batteryLevelUpdated(QString)\0"
    "wifiOn,wifiString\0wifiStateChanged(bool,QString)\0"
    "wifiListUpdated()\0"
};

void MenuHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MenuHandler *_t = static_cast<MenuHandler *>(_o);
        switch (_id) {
        case 0: _t->batteryLevelUpdated((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->wifiStateChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 2: _t->wifiListUpdated(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MenuHandler::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MenuHandler::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_MenuHandler,
      qt_meta_data_MenuHandler, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MenuHandler::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MenuHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MenuHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MenuHandler))
        return static_cast<void*>(const_cast< MenuHandler*>(this));
    return QObject::qt_metacast(_clname);
}

int MenuHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void MenuHandler::batteryLevelUpdated(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MenuHandler::wifiStateChanged(bool _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MenuHandler::wifiListUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
static const uint qt_meta_data_AnimatedSpinner[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       3,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
      20,   16, 0x02095103,
      34,   29, 0x01095103,
      37,   16, 0x02095103,

       0        // eod
};

static const char qt_meta_stringdata_AnimatedSpinner[] = {
    "AnimatedSpinner\0int\0duration\0bool\0on\0"
    "currentFrame\0"
};

void AnimatedSpinner::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData AnimatedSpinner::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject AnimatedSpinner::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_AnimatedSpinner,
      qt_meta_data_AnimatedSpinner, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &AnimatedSpinner::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *AnimatedSpinner::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *AnimatedSpinner::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AnimatedSpinner))
        return static_cast<void*>(const_cast< AnimatedSpinner*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int AnimatedSpinner::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = duration(); break;
        case 1: *reinterpret_cast< bool*>(_v) = on(); break;
        case 2: *reinterpret_cast< int*>(_v) = currentFrame(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setDuration(*reinterpret_cast< int*>(_v)); break;
        case 1: setOn(*reinterpret_cast< bool*>(_v)); break;
        case 2: setCurrentFrame(*reinterpret_cast< int*>(_v)); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
