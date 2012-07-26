/****************************************************************************
** Meta object code from reading C++ file 'pixbutton.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/buttons/pixbutton.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pixbutton.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PixButtonExtraHitArea[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   23,   22,   22, 0x05,
      45,   22,   22,   22, 0x25,

 // slots: signature, parameters, type, tag, flags
      57,   22,   22,   22, 0x0a,
      72,   22,   22,   22, 0x0a,
      93,   89,   22,   22, 0x0a,
     129,  121,   22,   22, 0x0a,
     164,  162,   22,   22, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_PixButtonExtraHitArea[] = {
    "PixButtonExtraHitArea\0\0pt\0signalHit(QPointF)\0"
    "signalHit()\0slotActivate()\0slotDeactivate()\0"
    "w,h\0slotResize(quint32,quint32)\0newGeom\0"
    "slotOwnerGeometryChanged(QRectF)\0p\0"
    "slotOwnerDestroyed(QObject*)\0"
};

void PixButtonExtraHitArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PixButtonExtraHitArea *_t = static_cast<PixButtonExtraHitArea *>(_o);
        switch (_id) {
        case 0: _t->signalHit((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 1: _t->signalHit(); break;
        case 2: _t->slotActivate(); break;
        case 3: _t->slotDeactivate(); break;
        case 4: _t->slotResize((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint32(*)>(_a[2]))); break;
        case 5: _t->slotOwnerGeometryChanged((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 6: _t->slotOwnerDestroyed((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PixButtonExtraHitArea::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixButtonExtraHitArea::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_PixButtonExtraHitArea,
      qt_meta_data_PixButtonExtraHitArea, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixButtonExtraHitArea::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixButtonExtraHitArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixButtonExtraHitArea::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixButtonExtraHitArea))
        return static_cast<void*>(const_cast< PixButtonExtraHitArea*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int PixButtonExtraHitArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void PixButtonExtraHitArea::signalHit(const QPointF _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_PixButton[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,   10,   10,   10, 0x05,
      32,   10,   10,   10, 0x05,
      48,   10,   10,   10, 0x05,
      64,   10,   10,   10, 0x05,
      84,   10,   10,   10, 0x05,
      99,   10,   10,   10, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_PixButton[] = {
    "PixButton\0\0signalFirstContact()\0"
    "signalContact()\0signalRelease()\0"
    "signalLastRelease()\0signalCancel()\0"
    "signalActivated()\0"
};

void PixButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PixButton *_t = static_cast<PixButton *>(_o);
        switch (_id) {
        case 0: _t->signalFirstContact(); break;
        case 1: _t->signalContact(); break;
        case 2: _t->signalRelease(); break;
        case 3: _t->signalLastRelease(); break;
        case 4: _t->signalCancel(); break;
        case 5: _t->signalActivated(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PixButton::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixButton::staticMetaObject = {
    { &ThingPaintable::staticMetaObject, qt_meta_stringdata_PixButton,
      qt_meta_data_PixButton, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixButton::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixButton::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixButton))
        return static_cast<void*>(const_cast< PixButton*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< PixButton*>(this));
    return ThingPaintable::qt_metacast(_clname);
}

int PixButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ThingPaintable::qt_metacall(_c, _id, _a);
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
void PixButton::signalFirstContact()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void PixButton::signalContact()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void PixButton::signalRelease()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void PixButton::signalLastRelease()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void PixButton::signalCancel()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void PixButton::signalActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}
QT_END_MOC_NAMESPACE
