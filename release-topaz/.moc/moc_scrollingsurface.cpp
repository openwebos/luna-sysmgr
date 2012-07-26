/****************************************************************************
** Meta object code from reading C++ file 'scrollingsurface.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/gfx/scrollingsurface.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrollingsurface.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScrollingSurface[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   18,   17,   17, 0x0a,
      71,   56,   17,   17, 0x0a,
     108,   56,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScrollingSurface[] = {
    "ScrollingSurface\0\0newGeom\0"
    "slotSourceGeomChanged(QRectF)\0"
    "newContentSize\0slotSourceContentSizeChanged(QSizeF)\0"
    "slotSourceContentSizeChanged(QSize)\0"
};

void ScrollingSurface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScrollingSurface *_t = static_cast<ScrollingSurface *>(_o);
        switch (_id) {
        case 0: _t->slotSourceGeomChanged((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 1: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSizeF(*)>(_a[1]))); break;
        case 2: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScrollingSurface::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScrollingSurface::staticMetaObject = {
    { &ScrollableObject::staticMetaObject, qt_meta_stringdata_ScrollingSurface,
      qt_meta_data_ScrollingSurface, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScrollingSurface::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScrollingSurface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScrollingSurface::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollingSurface))
        return static_cast<void*>(const_cast< ScrollingSurface*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< ScrollingSurface*>(this));
    return ScrollableObject::qt_metacast(_clname);
}

int ScrollingSurface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ScrollableObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
