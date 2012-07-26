/****************************************************************************
** Meta object code from reading C++ file 'MetaKeyManager.h'
**
** Created: Mon Jul 23 16:14:41 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/MetaKeyManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MetaKeyManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MetaKeyManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      16,   15,   15,   15, 0x05,
      29,   15,   15,   15, 0x05,
      41,   15,   15,   15, 0x05,
      55,   15,   15,   15, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_MetaKeyManager[] = {
    "MetaKeyManager\0\0signalCopy()\0signalCut()\0"
    "signalPaste()\0signalSelectAll()\0"
};

void MetaKeyManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MetaKeyManager *_t = static_cast<MetaKeyManager *>(_o);
        switch (_id) {
        case 0: _t->signalCopy(); break;
        case 1: _t->signalCut(); break;
        case 2: _t->signalPaste(); break;
        case 3: _t->signalSelectAll(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData MetaKeyManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MetaKeyManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_MetaKeyManager,
      qt_meta_data_MetaKeyManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MetaKeyManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MetaKeyManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MetaKeyManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MetaKeyManager))
        return static_cast<void*>(const_cast< MetaKeyManager*>(this));
    return QObject::qt_metacast(_clname);
}

int MetaKeyManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MetaKeyManager::signalCopy()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void MetaKeyManager::signalCut()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void MetaKeyManager::signalPaste()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void MetaKeyManager::signalSelectAll()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
