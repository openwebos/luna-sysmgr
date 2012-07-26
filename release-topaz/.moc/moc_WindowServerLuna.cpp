/****************************************************************************
** Meta object code from reading C++ file 'WindowServerLuna.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/WindowServerLuna.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WindowServerLuna.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_WindowServerLuna[] = {

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
      53,   18,   17,   17, 0x05,
     100,   17,   17,   17, 0x05,
     133,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
     176,  167,   17,   17, 0x08,
     216,  210,   17,   17, 0x08,
     251,  244,   17,   17, 0x08,
     285,   17,   17,   17, 0x08,
     321,  313,   17,   17, 0x08,
     356,  346,   17,   17, 0x08,
     380,   17,   17,   17, 0x08,
     402,   17,   17,   17, 0x08,
     437,  428,   17,   17, 0x08,
     466,   17,   17,   17, 0x08,
     505,   17,   17,   17, 0x08,
     527,   17,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_WindowServerLuna[] = {
    "WindowServerLuna\0\0wallpaper,fullScreen,rotationAngle\0"
    "signalWallpaperImageChanged(QPixmap*,bool,int)\0"
    "signalDockModeAnimationStarted()\0"
    "signalDockModeAnimationComplete()\0"
    "filePath\0slotWallPaperChanged(const char*)\0"
    "state\0slotDisplayStateChange(int)\0"
    "locked\0slotScreenLockStatusChanged(bool)\0"
    "slotDockAnimationFinished()\0enabled\0"
    "slotDockModeEnable(bool)\0connected\0"
    "slotPuckConnected(bool)\0slotFullEraseDevice()\0"
    "slotShowFullEraseWindow()\0critical\0"
    "slotMemoryStateChanged(bool)\0"
    "slotAppLaunchPreventedUnderLowMemory()\0"
    "slotBrickModeFailed()\0slotFirstCardRun()\0"
};

void WindowServerLuna::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        WindowServerLuna *_t = static_cast<WindowServerLuna *>(_o);
        switch (_id) {
        case 0: _t->signalWallpaperImageChanged((*reinterpret_cast< QPixmap*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->signalDockModeAnimationStarted(); break;
        case 2: _t->signalDockModeAnimationComplete(); break;
        case 3: _t->slotWallPaperChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 4: _t->slotDisplayStateChange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->slotScreenLockStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->slotDockAnimationFinished(); break;
        case 7: _t->slotDockModeEnable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->slotPuckConnected((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->slotFullEraseDevice(); break;
        case 10: _t->slotShowFullEraseWindow(); break;
        case 11: _t->slotMemoryStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->slotAppLaunchPreventedUnderLowMemory(); break;
        case 13: _t->slotBrickModeFailed(); break;
        case 14: _t->slotFirstCardRun(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData WindowServerLuna::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject WindowServerLuna::staticMetaObject = {
    { &WindowServer::staticMetaObject, qt_meta_stringdata_WindowServerLuna,
      qt_meta_data_WindowServerLuna, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &WindowServerLuna::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *WindowServerLuna::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *WindowServerLuna::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WindowServerLuna))
        return static_cast<void*>(const_cast< WindowServerLuna*>(this));
    return WindowServer::qt_metacast(_clname);
}

int WindowServerLuna::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowServer::qt_metacall(_c, _id, _a);
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
void WindowServerLuna::signalWallpaperImageChanged(QPixmap * _t1, bool _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void WindowServerLuna::signalDockModeAnimationStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void WindowServerLuna::signalDockModeAnimationComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
