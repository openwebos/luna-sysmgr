/****************************************************************************
** Meta object code from reading C++ file 'StatusBarServicesConnector.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarServicesConnector.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarServicesConnector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarServicesConnector[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      30,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      26,       // signalCount

 // signals: signature, parameters, type, tag, flags
      28,   27,   27,   27, 0x05,
      63,   53,   27,   27, 0x05,
     115,  104,   27,   27, 0x05,
     155,  146,   27,   27, 0x05,
     193,  188,   27,   27, 0x05,
     242,  231,   27,   27, 0x05,
     292,  231,   27,   27, 0x05,
     354,  346,   27,   27, 0x05,
     382,  346,   27,   27, 0x05,
     410,  346,   27,   27, 0x05,
     446,  346,   27,   27, 0x05,
     478,  346,   27,   27, 0x05,
     506,  231,   27,   27, 0x05,
     554,  231,   27,   27, 0x05,
     614,  231,   27,   27, 0x05,
     664,   27,   27,   27, 0x05,
     734,  690,   27,   27, 0x05,
     809,  792,   27,   27, 0x05,
     871,   27,   27,   27, 0x05,
     908,  897,   27,   27, 0x05,
     978,  955,   27,   27, 0x05,
    1051, 1028,   27,   27, 0x05,
    1121, 1111,   27,   27, 0x05,
    1177, 1164,   27,   27, 0x05,
    1248, 1231,   27,   27, 0x05,
    1300, 1294,   27,   27, 0x05,

 // slots: signature, parameters, type, tag, flags
    1345,  346,   27,   27, 0x08,
    1375,   27,   27,   27, 0x08,
    1405,   27,   27,   27, 0x08,
    1430,   27,   27,   27, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarServicesConnector[] = {
    "StatusBarServicesConnector\0\0"
    "signalPhoneTypeUpdated()\0connected\0"
    "signalPowerdConnectionStateChanged(bool)\0"
    "percentage\0signalBatteryLevelUpdated(int)\0"
    "charging\0signalChargingStateUpdated(bool)\0"
    "text\0signalCarrierTextChanged(const char*)\0"
    "show,index\0signalRssiIndexChanged(bool,StatusBar::IndexRSSI)\0"
    "signalRssi1xIndexChanged(bool,StatusBar::IndexRSSI1x)\0"
    "enabled\0signalTTYStateChanged(bool)\0"
    "signalHACStateChanged(bool)\0"
    "signalCallForwardStateChanged(bool)\0"
    "signalRoamingStateChanged(bool)\0"
    "signalVpnStateChanged(bool)\0"
    "signalWanIndexChanged(bool,StatusBar::IndexWAN)\0"
    "signalBluetoothIndexChanged(bool,StatusBar::IndexBluetooth)\0"
    "signalWifiIndexChanged(bool,StatusBar::IndexWiFi)\0"
    "signalSystemTimeChanged()\0"
    "wifiOn,wifiConnected,wifiSSID,wifiConnState\0"
    "signalWifiStateChanged(bool,bool,std::string,std::string)\0"
    "numNetworks,list\0"
    "signalWifiAvailableNetworksListUpdate(int,t_wifiAccessPoint*)\0"
    "signalBluetoothTurnedOn()\0radioState\0"
    "signalBluetoothPowerStateChanged(t_radioState)\0"
    "btConnected,deviceName\0"
    "signalBluetoothConnStateChanged(bool,std::string)\0"
    "numTrustedDevices,list\0"
    "signalBluetoothTrustedDevicesUpdate(int,t_bluetoothDevice*)\0"
    "available\0signalBluetoothParedDevicesAvailable(bool)\0"
    "deviceStatus\0"
    "signalBluetoothUpdateDeviceStatus(t_bluetoothDevice*)\0"
    "numProfiles,list\0"
    "signalVpnProfileListUpdate(int,t_vpnProfile*)\0"
    "state\0signalAirplaneModeState(t_airplaneModeState)\0"
    "slotAirplaneModeChanged(bool)\0"
    "slotRoamingIndicatorChanged()\0"
    "slotEnterBrickMode(bool)\0slotExitBrickMode()\0"
};

void StatusBarServicesConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarServicesConnector *_t = static_cast<StatusBarServicesConnector *>(_o);
        switch (_id) {
        case 0: _t->signalPhoneTypeUpdated(); break;
        case 1: _t->signalPowerdConnectionStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->signalBatteryLevelUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->signalChargingStateUpdated((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->signalCarrierTextChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 5: _t->signalRssiIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexRSSI(*)>(_a[2]))); break;
        case 6: _t->signalRssi1xIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexRSSI1x(*)>(_a[2]))); break;
        case 7: _t->signalTTYStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->signalHACStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->signalCallForwardStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->signalRoamingStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->signalVpnStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->signalWanIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexWAN(*)>(_a[2]))); break;
        case 13: _t->signalBluetoothIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexBluetooth(*)>(_a[2]))); break;
        case 14: _t->signalWifiIndexChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< StatusBar::IndexWiFi(*)>(_a[2]))); break;
        case 15: _t->signalSystemTimeChanged(); break;
        case 16: _t->signalWifiStateChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< std::string(*)>(_a[3])),(*reinterpret_cast< std::string(*)>(_a[4]))); break;
        case 17: _t->signalWifiAvailableNetworksListUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_wifiAccessPoint*(*)>(_a[2]))); break;
        case 18: _t->signalBluetoothTurnedOn(); break;
        case 19: _t->signalBluetoothPowerStateChanged((*reinterpret_cast< t_radioState(*)>(_a[1]))); break;
        case 20: _t->signalBluetoothConnStateChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< std::string(*)>(_a[2]))); break;
        case 21: _t->signalBluetoothTrustedDevicesUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_bluetoothDevice*(*)>(_a[2]))); break;
        case 22: _t->signalBluetoothParedDevicesAvailable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 23: _t->signalBluetoothUpdateDeviceStatus((*reinterpret_cast< t_bluetoothDevice*(*)>(_a[1]))); break;
        case 24: _t->signalVpnProfileListUpdate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< t_vpnProfile*(*)>(_a[2]))); break;
        case 25: _t->signalAirplaneModeState((*reinterpret_cast< t_airplaneModeState(*)>(_a[1]))); break;
        case 26: _t->slotAirplaneModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 27: _t->slotRoamingIndicatorChanged(); break;
        case 28: _t->slotEnterBrickMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 29: _t->slotExitBrickMode(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBarServicesConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarServicesConnector::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_StatusBarServicesConnector,
      qt_meta_data_StatusBarServicesConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarServicesConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarServicesConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarServicesConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarServicesConnector))
        return static_cast<void*>(const_cast< StatusBarServicesConnector*>(this));
    return QObject::qt_metacast(_clname);
}

int StatusBarServicesConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 30)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 30;
    }
    return _id;
}

// SIGNAL 0
void StatusBarServicesConnector::signalPhoneTypeUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void StatusBarServicesConnector::signalPowerdConnectionStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void StatusBarServicesConnector::signalBatteryLevelUpdated(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void StatusBarServicesConnector::signalChargingStateUpdated(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void StatusBarServicesConnector::signalCarrierTextChanged(const char * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void StatusBarServicesConnector::signalRssiIndexChanged(bool _t1, StatusBar::IndexRSSI _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void StatusBarServicesConnector::signalRssi1xIndexChanged(bool _t1, StatusBar::IndexRSSI1x _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void StatusBarServicesConnector::signalTTYStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void StatusBarServicesConnector::signalHACStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void StatusBarServicesConnector::signalCallForwardStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void StatusBarServicesConnector::signalRoamingStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void StatusBarServicesConnector::signalVpnStateChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void StatusBarServicesConnector::signalWanIndexChanged(bool _t1, StatusBar::IndexWAN _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void StatusBarServicesConnector::signalBluetoothIndexChanged(bool _t1, StatusBar::IndexBluetooth _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void StatusBarServicesConnector::signalWifiIndexChanged(bool _t1, StatusBar::IndexWiFi _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void StatusBarServicesConnector::signalSystemTimeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, 0);
}

// SIGNAL 16
void StatusBarServicesConnector::signalWifiStateChanged(bool _t1, bool _t2, std::string _t3, std::string _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void StatusBarServicesConnector::signalWifiAvailableNetworksListUpdate(int _t1, t_wifiAccessPoint * _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void StatusBarServicesConnector::signalBluetoothTurnedOn()
{
    QMetaObject::activate(this, &staticMetaObject, 18, 0);
}

// SIGNAL 19
void StatusBarServicesConnector::signalBluetoothPowerStateChanged(t_radioState _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void StatusBarServicesConnector::signalBluetoothConnStateChanged(bool _t1, std::string _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 21
void StatusBarServicesConnector::signalBluetoothTrustedDevicesUpdate(int _t1, t_bluetoothDevice * _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}

// SIGNAL 22
void StatusBarServicesConnector::signalBluetoothParedDevicesAvailable(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 23
void StatusBarServicesConnector::signalBluetoothUpdateDeviceStatus(t_bluetoothDevice * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 23, _a);
}

// SIGNAL 24
void StatusBarServicesConnector::signalVpnProfileListUpdate(int _t1, t_vpnProfile * _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 24, _a);
}

// SIGNAL 25
void StatusBarServicesConnector::signalAirplaneModeState(t_airplaneModeState _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 25, _a);
}
QT_END_MOC_NAMESPACE
