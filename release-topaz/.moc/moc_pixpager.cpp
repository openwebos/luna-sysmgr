/****************************************************************************
** Meta object code from reading C++ file 'pixpager.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/gfx/pixpager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pixpager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PixPagerPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_PixPagerPage[] = {
    "PixPagerPage\0\0slotPixmapObjectDeleted()\0"
};

void PixPagerPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PixPagerPage *_t = static_cast<PixPagerPage *>(_o);
        switch (_id) {
        case 0: _t->slotPixmapObjectDeleted(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PixPagerPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixPagerPage::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_PixPagerPage,
      qt_meta_data_PixPagerPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixPagerPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixPagerPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixPagerPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixPagerPage))
        return static_cast<void*>(const_cast< PixPagerPage*>(this));
    return QObject::qt_metacast(_clname);
}

int PixPagerPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_PixPagerAtlasPage[] = {

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

static const char qt_meta_stringdata_PixPagerAtlasPage[] = {
    "PixPagerAtlasPage\0"
};

void PixPagerAtlasPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PixPagerAtlasPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixPagerAtlasPage::staticMetaObject = {
    { &PixPagerPage::staticMetaObject, qt_meta_stringdata_PixPagerAtlasPage,
      qt_meta_data_PixPagerAtlasPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixPagerAtlasPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixPagerAtlasPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixPagerAtlasPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixPagerAtlasPage))
        return static_cast<void*>(const_cast< PixPagerAtlasPage*>(this));
    return PixPagerPage::qt_metacast(_clname);
}

int PixPagerAtlasPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PixPagerPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_PixPager[] = {

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

static const char qt_meta_stringdata_PixPager[] = {
    "PixPager\0"
};

void PixPager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PixPager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixPager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_PixPager,
      qt_meta_data_PixPager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixPager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixPager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixPager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixPager))
        return static_cast<void*>(const_cast< PixPager*>(this));
    return QObject::qt_metacast(_clname);
}

int PixPager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
