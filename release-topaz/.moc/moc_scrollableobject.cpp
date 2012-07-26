/****************************************************************************
** Meta object code from reading C++ file 'scrollableobject.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/gfx/scrollableobject.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrollableobject.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScrollableObject[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       1,   29, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   18,   17,   17, 0x0a,
      71,   56,   17,   17, 0x0a,
     108,   56,   17,   17, 0x0a,

 // properties: name, type, flags
     151,  144, 0x02095003,

       0        // eod
};

static const char qt_meta_stringdata_ScrollableObject[] = {
    "ScrollableObject\0\0newGeom\0"
    "slotSourceGeomChanged(QRectF)\0"
    "newContentSize\0slotSourceContentSizeChanged(QSizeF)\0"
    "slotSourceContentSizeChanged(QSize)\0"
    "qint32\0scroll\0"
};

void ScrollableObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScrollableObject *_t = static_cast<ScrollableObject *>(_o);
        switch (_id) {
        case 0: _t->slotSourceGeomChanged((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 1: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSizeF(*)>(_a[1]))); break;
        case 2: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScrollableObject::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScrollableObject::staticMetaObject = {
    { &ThingPaintable::staticMetaObject, qt_meta_stringdata_ScrollableObject,
      qt_meta_data_ScrollableObject, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScrollableObject::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScrollableObject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScrollableObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollableObject))
        return static_cast<void*>(const_cast< ScrollableObject*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< ScrollableObject*>(this));
    return ThingPaintable::qt_metacast(_clname);
}

int ScrollableObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ThingPaintable::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< qint32*>(_v) = scrollValue(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setScrollValue(*reinterpret_cast< qint32*>(_v)); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
