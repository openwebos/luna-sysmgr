/****************************************************************************
** Meta object code from reading C++ file 'thingpaintable.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/thingpaintable.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'thingpaintable.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ThingPaintable[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       2,   34, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      18,   16,   15,   15, 0x05,
      55,   15,   15,   15, 0x05,
      79,   15,   15,   15, 0x05,
     102,   15,   15,   15, 0x05,

 // properties: name, type, flags
     134,  126, 0x0a495007,
     150,  142, 0x1a495003,

 // properties: notify_signal_id
       2,
       3,

       0        // eod
};

static const char qt_meta_stringdata_ThingPaintable[] = {
    "ThingPaintable\0\0,\0"
    "signalGeometryChanged(QRectF,QRectF)\0"
    "signalGeometryChanged()\0signalUiStateChanged()\0"
    "signalPositionChanged()\0QString\0uistate\0"
    "QPointF\0animatePosition\0"
};

void ThingPaintable::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ThingPaintable *_t = static_cast<ThingPaintable *>(_o);
        switch (_id) {
        case 0: _t->signalGeometryChanged((*reinterpret_cast< const QRectF(*)>(_a[1])),(*reinterpret_cast< const QRectF(*)>(_a[2]))); break;
        case 1: _t->signalGeometryChanged(); break;
        case 2: _t->signalUiStateChanged(); break;
        case 3: _t->signalPositionChanged(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ThingPaintable::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ThingPaintable::staticMetaObject = {
    { &Thing::staticMetaObject, qt_meta_stringdata_ThingPaintable,
      qt_meta_data_ThingPaintable, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ThingPaintable::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ThingPaintable::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ThingPaintable::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ThingPaintable))
        return static_cast<void*>(const_cast< ThingPaintable*>(this));
    return Thing::qt_metacast(_clname);
}

int ThingPaintable::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Thing::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = readUiState(); break;
        case 1: *reinterpret_cast< QPointF*>(_v) = pos(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: writeUiState(*reinterpret_cast< QString*>(_v)); break;
        case 1: setPos(*reinterpret_cast< QPointF*>(_v)); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        switch (_id) {
        case 0: resetUiState(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ThingPaintable::signalGeometryChanged(const QRectF & _t1, const QRectF & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ThingPaintable::signalGeometryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void ThingPaintable::signalUiStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void ThingPaintable::signalPositionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
