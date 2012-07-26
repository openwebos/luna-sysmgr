/****************************************************************************
** Meta object code from reading C++ file 'SystemService.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/SystemService.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SystemService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SystemService[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x05,
      51,   41,   14,   14, 0x05,
      78,   14,   14,   14, 0x05,
     100,   14,   14,   14, 0x05,
     124,   14,   14,   14, 0x05,
     160,   14,   14,   14, 0x05,
     183,   14,   14,   14, 0x05,
     206,   14,   14,   14, 0x05,
     237,   14,   14,   14, 0x05,
     285,   14,   14,   14, 0x05,
     312,   14,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
     342,   14,   14,   14, 0x08,
     369,  361,   14,   14, 0x08,
     394,   14,   14,   14, 0x08,
     417,   14,   14,   14, 0x08,
     442,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_SystemService[] = {
    "SystemService\0\0signalIncomingPhoneCall()\0"
    "mediaSync\0signalEnterBrickMode(bool)\0"
    "signalExitBrickMode()\0signalBrickModeFailed()\0"
    "signalMediaPartitionAvailable(bool)\0"
    "signalDeviceUnlocked()\0signalCancelPinEntry()\0"
    "signalTouchToShareCanTap(bool)\0"
    "signalTouchToShareAppUrlTransfered(std::string)\0"
    "signalDismissModalDialog()\0"
    "signalStopModalDismissTimer()\0"
    "postBootFinished()\0enabled\0"
    "postDockModeStatus(bool)\0"
    "slotModalWindowAdded()\0slotModalWindowRemoved()\0"
    "slotModalDialogTimerFired()\0"
};

void SystemService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SystemService *_t = static_cast<SystemService *>(_o);
        switch (_id) {
        case 0: _t->signalIncomingPhoneCall(); break;
        case 1: _t->signalEnterBrickMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->signalExitBrickMode(); break;
        case 3: _t->signalBrickModeFailed(); break;
        case 4: _t->signalMediaPartitionAvailable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->signalDeviceUnlocked(); break;
        case 6: _t->signalCancelPinEntry(); break;
        case 7: _t->signalTouchToShareCanTap((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->signalTouchToShareAppUrlTransfered((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 9: _t->signalDismissModalDialog(); break;
        case 10: _t->signalStopModalDismissTimer(); break;
        case 11: _t->postBootFinished(); break;
        case 12: _t->postDockModeStatus((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->slotModalWindowAdded(); break;
        case 14: _t->slotModalWindowRemoved(); break;
        case 15: _t->slotModalDialogTimerFired(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SystemService::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SystemService::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SystemService,
      qt_meta_data_SystemService, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SystemService::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SystemService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SystemService::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SystemService))
        return static_cast<void*>(const_cast< SystemService*>(this));
    return QObject::qt_metacast(_clname);
}

int SystemService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void SystemService::signalIncomingPhoneCall()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void SystemService::signalEnterBrickMode(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SystemService::signalExitBrickMode()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void SystemService::signalBrickModeFailed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void SystemService::signalMediaPartitionAvailable(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SystemService::signalDeviceUnlocked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void SystemService::signalCancelPinEntry()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void SystemService::signalTouchToShareCanTap(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void SystemService::signalTouchToShareAppUrlTransfered(const std::string & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void SystemService::signalDismissModalDialog()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void SystemService::signalStopModalDismissTimer()
{
    QMetaObject::activate(this, &staticMetaObject, 10, 0);
}
QT_END_MOC_NAMESPACE
