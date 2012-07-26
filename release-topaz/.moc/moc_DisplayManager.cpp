/****************************************************************************
** Meta object code from reading C++ file 'DisplayManager.h'
**
** Created: Mon Jul 23 16:14:41 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/DisplayManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DisplayManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DisplayManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   16,   15,   15, 0x05,
      62,   52,   15,   15, 0x05,
     107,   88,   15,   15, 0x05,
     149,  138,   15,   15, 0x05,

 // slots: signature, parameters, type, tag, flags
     195,  188,   15,   15, 0x08,
     219,  188,   15,   15, 0x08,
     240,   15,   15,   15, 0x08,
     259,   15,   15,   15, 0x08,
     273,   15,   15,   15, 0x08,
     294,  287,   15,   15, 0x08,
     335,  328,   15,   15, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DisplayManager[] = {
    "DisplayManager\0\0state\0"
    "signalDisplayStateChange(int)\0connected\0"
    "signalPuckConnected(bool)\0state,displayEvent\0"
    "signalLockStateChange(int,int)\0"
    "brightness\0signalDisplayMaxBrightnessChanged(int)\0"
    "enable\0slotEmergencyMode(bool)\0"
    "slotAlsEnabled(bool)\0slotBootFinished()\0"
    "slotShowIME()\0slotHideIME()\0active\0"
    "slotBluetoothKeyboardActive(bool)\0"
    "change\0slotAirplaneModeChanged(bool)\0"
};

void DisplayManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DisplayManager *_t = static_cast<DisplayManager *>(_o);
        switch (_id) {
        case 0: _t->signalDisplayStateChange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->signalPuckConnected((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->signalLockStateChange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->signalDisplayMaxBrightnessChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->slotEmergencyMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->slotAlsEnabled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->slotBootFinished(); break;
        case 7: _t->slotShowIME(); break;
        case 8: _t->slotHideIME(); break;
        case 9: _t->slotBluetoothKeyboardActive((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->slotAirplaneModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DisplayManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DisplayManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DisplayManager,
      qt_meta_data_DisplayManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DisplayManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DisplayManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DisplayManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DisplayManager))
        return static_cast<void*>(const_cast< DisplayManager*>(this));
    return QObject::qt_metacast(_clname);
}

int DisplayManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void DisplayManager::signalDisplayStateChange(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DisplayManager::signalPuckConnected(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DisplayManager::signalLockStateChange(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DisplayManager::signalDisplayMaxBrightnessChanged(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
