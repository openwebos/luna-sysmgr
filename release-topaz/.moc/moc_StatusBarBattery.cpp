/****************************************************************************
** Meta object code from reading C++ file 'StatusBarBattery.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarBattery.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarBattery.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarBattery[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      28,   18,   17,   17, 0x08,
      78,   67,   17,   17, 0x08,
     116,  107,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarBattery[] = {
    "StatusBarBattery\0\0connected\0"
    "slotPowerdConnectionStateChanged(bool)\0"
    "percentage\0slotBatteryLevelUpdated(int)\0"
    "charging\0slotChargingStateUpdated(bool)\0"
};

void StatusBarBattery::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarBattery *_t = static_cast<StatusBarBattery *>(_o);
        switch (_id) {
        case 0: _t->slotPowerdConnectionStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->slotBatteryLevelUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->slotChargingStateUpdated((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBarBattery::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarBattery::staticMetaObject = {
    { &StatusBarItem::staticMetaObject, qt_meta_stringdata_StatusBarBattery,
      qt_meta_data_StatusBarBattery, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarBattery::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarBattery::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarBattery::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarBattery))
        return static_cast<void*>(const_cast< StatusBarBattery*>(this));
    return StatusBarItem::qt_metacast(_clname);
}

int StatusBarBattery::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
