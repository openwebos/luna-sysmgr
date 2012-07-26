/****************************************************************************
** Meta object code from reading C++ file 'iconlayout.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/icon_layouts/iconlayout.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'iconlayout.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_IconCell[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      17,   10,    9,    9, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IconCell[] = {
    "IconCell\0\0p_icon\0signalChangedIcon(const IconBase*)\0"
};

void IconCell::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IconCell *_t = static_cast<IconCell *>(_o);
        switch (_id) {
        case 0: _t->signalChangedIcon((*reinterpret_cast< const IconBase*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IconCell::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IconCell::staticMetaObject = {
    { &LayoutItem::staticMetaObject, qt_meta_stringdata_IconCell,
      qt_meta_data_IconCell, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IconCell::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IconCell::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IconCell::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IconCell))
        return static_cast<void*>(const_cast< IconCell*>(this));
    return LayoutItem::qt_metacast(_clname);
}

int IconCell::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LayoutItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void IconCell::signalChangedIcon(const IconBase * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IconRow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_IconRow[] = {
    "IconRow\0"
};

void IconRow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData IconRow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IconRow::staticMetaObject = {
    { &LayoutItem::staticMetaObject, qt_meta_stringdata_IconRow,
      qt_meta_data_IconRow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IconRow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IconRow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IconRow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IconRow))
        return static_cast<void*>(const_cast< IconRow*>(this));
    return LayoutItem::qt_metacast(_clname);
}

int IconRow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LayoutItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_IconLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_IconLayout[] = {
    "IconLayout\0"
};

void IconLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData IconLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IconLayout::staticMetaObject = {
    { &LayoutItem::staticMetaObject, qt_meta_stringdata_IconLayout,
      qt_meta_data_IconLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IconLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IconLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IconLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IconLayout))
        return static_cast<void*>(const_cast< IconLayout*>(this));
    return LayoutItem::qt_metacast(_clname);
}

int IconLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LayoutItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
