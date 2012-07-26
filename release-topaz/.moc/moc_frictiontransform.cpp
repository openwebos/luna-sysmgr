/****************************************************************************
** Meta object code from reading C++ file 'frictiontransform.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/physics/motion/frictiontransform.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'frictiontransform.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_FrictionTransform[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      27,   19,   18,   18, 0x0a,
      46,   18,   18,   18, 0x2a,

       0        // eod
};

static const char qt_meta_stringdata_FrictionTransform[] = {
    "FrictionTransform\0\0absTime\0"
    "slotTimetic(qreal)\0slotTimetic()\0"
};

void FrictionTransform::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        FrictionTransform *_t = static_cast<FrictionTransform *>(_o);
        switch (_id) {
        case 0: _t->slotTimetic((*reinterpret_cast< const qreal(*)>(_a[1]))); break;
        case 1: _t->slotTimetic(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData FrictionTransform::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject FrictionTransform::staticMetaObject = {
    { &LinearMotionTransform::staticMetaObject, qt_meta_stringdata_FrictionTransform,
      qt_meta_data_FrictionTransform, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &FrictionTransform::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *FrictionTransform::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *FrictionTransform::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FrictionTransform))
        return static_cast<void*>(const_cast< FrictionTransform*>(this));
    return LinearMotionTransform::qt_metacast(_clname);
}

int FrictionTransform::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LinearMotionTransform::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
