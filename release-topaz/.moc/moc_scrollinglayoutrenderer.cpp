/****************************************************************************
** Meta object code from reading C++ file 'scrollinglayoutrenderer.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/gfx/scrollinglayoutrenderer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrollinglayoutrenderer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScrollingLayoutRenderer[] = {

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
      33,   25,   24,   24, 0x09,
      78,   63,   24,   24, 0x09,
     115,   63,   24,   24, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_ScrollingLayoutRenderer[] = {
    "ScrollingLayoutRenderer\0\0newGeom\0"
    "slotSourceGeomChanged(QRectF)\0"
    "newContentSize\0slotSourceContentSizeChanged(QSizeF)\0"
    "slotSourceContentSizeChanged(QSize)\0"
};

void ScrollingLayoutRenderer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScrollingLayoutRenderer *_t = static_cast<ScrollingLayoutRenderer *>(_o);
        switch (_id) {
        case 0: _t->slotSourceGeomChanged((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 1: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSizeF(*)>(_a[1]))); break;
        case 2: _t->slotSourceContentSizeChanged((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScrollingLayoutRenderer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScrollingLayoutRenderer::staticMetaObject = {
    { &ScrollableObject::staticMetaObject, qt_meta_stringdata_ScrollingLayoutRenderer,
      qt_meta_data_ScrollingLayoutRenderer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScrollingLayoutRenderer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScrollingLayoutRenderer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScrollingLayoutRenderer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollingLayoutRenderer))
        return static_cast<void*>(const_cast< ScrollingLayoutRenderer*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< ScrollingLayoutRenderer*>(this));
    return ScrollableObject::qt_metacast(_clname);
}

int ScrollingLayoutRenderer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
