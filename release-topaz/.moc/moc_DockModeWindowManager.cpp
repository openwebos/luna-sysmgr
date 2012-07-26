/****************************************************************************
** Meta object code from reading C++ file 'DockModeWindowManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/dock/DockModeWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockModeWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DockModeWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      31,   23,   22,   22, 0x05,
      76,   65,   22,   22, 0x05,
     106,  103,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
     153,   22,   22,   22, 0x08,
     172,   22,   22,   22, 0x08,
     208,   22,   22,   22, 0x08,
     247,  232,   22,   22, 0x08,
     287,  283,   22,   22, 0x08,
     333,  103,   22,   22, 0x08,
     384,  103,   22,   22, 0x08,
     436,   22,   22,   22, 0x08,
     467,   22,   22,   22, 0x08,
     509,  499,   22,   22, 0x08,
     538,  533,   22,   22, 0x08,
     581,  570,   22,   22, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DockModeWindowManager[] = {
    "DockModeWindowManager\0\0enabled\0"
    "signalDockModeStatusChanged(bool)\0"
    "forceClose\0signalCloseDashboard(bool)\0"
    "lp\0signalDockModeAppChanged(DockModeLaunchPoint*)\0"
    "slotBootFinished()\0"
    "slotWindowChangeAnimationFinished()\0"
    "slotVisibilityChanged()\0allowExpensive\0"
    "slotLowMemoryActionsRequested(bool)\0"
    "dlp\0slotDockModeAppSelected(DockModeLaunchPoint*)\0"
    "slotDockModeLaunchPointEnabled(const LaunchPoint*)\0"
    "slotDockModeLaunchPointDisabled(const LaunchPoint*)\0"
    "slotDockModeAnimationStarted()\0"
    "slotDockModeAnimationComplete()\0"
    "connected\0slotPuckConnected(bool)\0"
    "rect\0slotPositiveSpaceChanged(QRect)\0"
    "lp,reasons\0"
    "slotLaunchPointRemoved(const LaunchPoint*,QBitArray)\0"
};

void DockModeWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DockModeWindowManager *_t = static_cast<DockModeWindowManager *>(_o);
        switch (_id) {
        case 0: _t->signalDockModeStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->signalCloseDashboard((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->signalDockModeAppChanged((*reinterpret_cast< DockModeLaunchPoint*(*)>(_a[1]))); break;
        case 3: _t->slotBootFinished(); break;
        case 4: _t->slotWindowChangeAnimationFinished(); break;
        case 5: _t->slotVisibilityChanged(); break;
        case 6: _t->slotLowMemoryActionsRequested((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->slotDockModeAppSelected((*reinterpret_cast< DockModeLaunchPoint*(*)>(_a[1]))); break;
        case 8: _t->slotDockModeLaunchPointEnabled((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 9: _t->slotDockModeLaunchPointDisabled((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 10: _t->slotDockModeAnimationStarted(); break;
        case 11: _t->slotDockModeAnimationComplete(); break;
        case 12: _t->slotPuckConnected((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 14: _t->slotLaunchPointRemoved((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DockModeWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DockModeWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_DockModeWindowManager,
      qt_meta_data_DockModeWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DockModeWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DockModeWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DockModeWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DockModeWindowManager))
        return static_cast<void*>(const_cast< DockModeWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int DockModeWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void DockModeWindowManager::signalDockModeStatusChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DockModeWindowManager::signalCloseDashboard(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DockModeWindowManager::signalDockModeAppChanged(DockModeLaunchPoint * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
