/****************************************************************************
** Meta object code from reading C++ file 'MenuWindowManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/MenuWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MenuWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MenuWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      19,   18,   18,   18, 0x0a,
      33,   31,   18,   18, 0x08,
      72,   65,   18,   18, 0x08,
     105,   18,   18,   18, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MenuWindowManager[] = {
    "MenuWindowManager\0\0closeMenu()\0r\0"
    "slotPositiveSpaceChanged(QRect)\0opened\0"
    "slotSystemMenuStateChanged(bool)\0"
    "slotCloseSystemMenu()\0"
};

void MenuWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MenuWindowManager *_t = static_cast<MenuWindowManager *>(_o);
        switch (_id) {
        case 0: _t->closeMenu(); break;
        case 1: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 2: _t->slotSystemMenuStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->slotCloseSystemMenu(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MenuWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MenuWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_MenuWindowManager,
      qt_meta_data_MenuWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MenuWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MenuWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MenuWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MenuWindowManager))
        return static_cast<void*>(const_cast< MenuWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int MenuWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
