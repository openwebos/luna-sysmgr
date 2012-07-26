/****************************************************************************
** Meta object code from reading C++ file 'linearmotiontransform.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/physics/motion/linearmotiontransform.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'linearmotiontransform.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_LinearMotionTransform[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      48,   23,   22,   22, 0x05,
     160,  135,   22,   22, 0x05,
     268,  243,   22,   22, 0x05,
     363,  355,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
     413,  405,   22,   22, 0x0a,
     432,   22,   22,   22, 0x2a,

       0        // eod
};

static const char qt_meta_stringdata_LinearMotionTransform[] = {
    "LinearMotionTransform\0\0t,d,last_t,last_d,reason\0"
    "signalDisplacementTrigger(qreal,qreal,qreal,qreal,LinearMotionTransfor"
    "mTriggers::Type)\0"
    "t,v,last_t,last_v,reason\0"
    "signalVelocityTrigger(qreal,qreal,qreal,qreal,LinearMotionTransformTri"
    "ggers::Type)\0"
    "t,a,last_t,last_a,reason\0"
    "signalAccelerationTrigger(qreal,qreal,qreal,qreal,LinearMotionTransfor"
    "mTriggers::Type)\0"
    "t,d,v,a\0signalRecomputed(qreal,qreal,qreal,qreal)\0"
    "absTime\0slotTimetic(qreal)\0slotTimetic()\0"
};

void LinearMotionTransform::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LinearMotionTransform *_t = static_cast<LinearMotionTransform *>(_o);
        switch (_id) {
        case 0: _t->signalDisplacementTrigger((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2])),(*reinterpret_cast< qreal(*)>(_a[3])),(*reinterpret_cast< qreal(*)>(_a[4])),(*reinterpret_cast< LinearMotionTransformTriggers::Type(*)>(_a[5]))); break;
        case 1: _t->signalVelocityTrigger((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2])),(*reinterpret_cast< qreal(*)>(_a[3])),(*reinterpret_cast< qreal(*)>(_a[4])),(*reinterpret_cast< LinearMotionTransformTriggers::Type(*)>(_a[5]))); break;
        case 2: _t->signalAccelerationTrigger((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2])),(*reinterpret_cast< qreal(*)>(_a[3])),(*reinterpret_cast< qreal(*)>(_a[4])),(*reinterpret_cast< LinearMotionTransformTriggers::Type(*)>(_a[5]))); break;
        case 3: _t->signalRecomputed((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2])),(*reinterpret_cast< qreal(*)>(_a[3])),(*reinterpret_cast< qreal(*)>(_a[4]))); break;
        case 4: _t->slotTimetic((*reinterpret_cast< const qreal(*)>(_a[1]))); break;
        case 5: _t->slotTimetic(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData LinearMotionTransform::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject LinearMotionTransform::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_LinearMotionTransform,
      qt_meta_data_LinearMotionTransform, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &LinearMotionTransform::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *LinearMotionTransform::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *LinearMotionTransform::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_LinearMotionTransform))
        return static_cast<void*>(const_cast< LinearMotionTransform*>(this));
    return QObject::qt_metacast(_clname);
}

int LinearMotionTransform::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void LinearMotionTransform::signalDisplacementTrigger(qreal _t1, qreal _t2, qreal _t3, qreal _t4, LinearMotionTransformTriggers::Type _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LinearMotionTransform::signalVelocityTrigger(qreal _t1, qreal _t2, qreal _t3, qreal _t4, LinearMotionTransformTriggers::Type _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void LinearMotionTransform::signalAccelerationTrigger(qreal _t1, qreal _t2, qreal _t3, qreal _t4, LinearMotionTransformTriggers::Type _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void LinearMotionTransform::signalRecomputed(qreal _t1, qreal _t2, qreal _t3, qreal _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
