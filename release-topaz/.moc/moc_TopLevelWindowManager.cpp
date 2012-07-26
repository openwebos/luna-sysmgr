/****************************************************************************
** Meta object code from reading C++ file 'TopLevelWindowManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/lockscreen/TopLevelWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TopLevelWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_TopLevelWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      30,   23,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
      80,   66,   22,   22, 0x08,
     105,   22,   22,   22, 0x08,
     125,   22,   22,   22, 0x08,
     164,  158,   22,   22, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_TopLevelWindowManager[] = {
    "TopLevelWindowManager\0\0locked\0"
    "signalScreenLockStatusChanged(bool)\0"
    "mediaSyncMode\0slotEnterBrickMode(bool)\0"
    "slotExitBrickMode()\0"
    "slotBrickSurfAnimationFinished()\0state\0"
    "slotScreenLocked(int)\0"
};

void TopLevelWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TopLevelWindowManager *_t = static_cast<TopLevelWindowManager *>(_o);
        switch (_id) {
        case 0: _t->signalScreenLockStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->slotEnterBrickMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->slotExitBrickMode(); break;
        case 3: _t->slotBrickSurfAnimationFinished(); break;
        case 4: _t->slotScreenLocked((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData TopLevelWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject TopLevelWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_TopLevelWindowManager,
      qt_meta_data_TopLevelWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &TopLevelWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *TopLevelWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *TopLevelWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TopLevelWindowManager))
        return static_cast<void*>(const_cast< TopLevelWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int TopLevelWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void TopLevelWindowManager::signalScreenLockStatusChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
