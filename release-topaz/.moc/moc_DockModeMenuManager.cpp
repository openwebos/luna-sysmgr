/****************************************************************************
** Meta object code from reading C++ file 'DockModeMenuManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/dock/DockModeMenuManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockModeMenuManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DockModeMenuManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   21,   20,   20, 0x0a,
      48,   21,   20,   20, 0x0a,
      73,   20,   20,   20, 0x0a,
      89,   20,   20,   20, 0x0a,
     111,   20,   20,   20, 0x0a,
     157,   20,   20,   20, 0x0a,
     210,  202,   20,   20, 0x0a,
     244,  242,   20,   20, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DockModeMenuManager[] = {
    "DockModeMenuManager\0\0open\0"
    "activateAppMenu(bool)\0activateSystemMenu(bool)\0"
    "closeAllMenus()\0slotCloseSystemMenu()\0"
    "slotDockModeAppSelected(DockModeLaunchPoint*)\0"
    "slotDockModeAppChanged(DockModeLaunchPoint*)\0"
    "enabled\0slotDockModeStatusChanged(bool)\0"
    "r\0slotPositiveSpaceChanged(QRect)\0"
};

void DockModeMenuManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DockModeMenuManager *_t = static_cast<DockModeMenuManager *>(_o);
        switch (_id) {
        case 0: _t->activateAppMenu((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->activateSystemMenu((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->closeAllMenus(); break;
        case 3: _t->slotCloseSystemMenu(); break;
        case 4: _t->slotDockModeAppSelected((*reinterpret_cast< DockModeLaunchPoint*(*)>(_a[1]))); break;
        case 5: _t->slotDockModeAppChanged((*reinterpret_cast< DockModeLaunchPoint*(*)>(_a[1]))); break;
        case 6: _t->slotDockModeStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DockModeMenuManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DockModeMenuManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_DockModeMenuManager,
      qt_meta_data_DockModeMenuManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DockModeMenuManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DockModeMenuManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DockModeMenuManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DockModeMenuManager))
        return static_cast<void*>(const_cast< DockModeMenuManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int DockModeMenuManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
