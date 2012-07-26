/****************************************************************************
** Meta object code from reading C++ file 'pixmapfilmstripobject.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/gfx/pixmapobject/pixmapfilmstripobject.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pixmapfilmstripobject.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PixmapFilmstripObject[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       2,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
      30,   22, 0x03095003,
      41,   22, 0x03095001,

       0        // eod
};

static const char qt_meta_stringdata_PixmapFilmstripObject[] = {
    "PixmapFilmstripObject\0quint32\0frameindex\0"
    "totalframes\0"
};

void PixmapFilmstripObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PixmapFilmstripObject::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PixmapFilmstripObject::staticMetaObject = {
    { &PixmapObject::staticMetaObject, qt_meta_stringdata_PixmapFilmstripObject,
      qt_meta_data_PixmapFilmstripObject, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PixmapFilmstripObject::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PixmapFilmstripObject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PixmapFilmstripObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PixmapFilmstripObject))
        return static_cast<void*>(const_cast< PixmapFilmstripObject*>(this));
    return PixmapObject::qt_metacast(_clname);
}

int PixmapFilmstripObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PixmapObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< quint32*>(_v) = frameIndex(); break;
        case 1: *reinterpret_cast< quint32*>(_v) = totalFrames(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setFrameIndex(*reinterpret_cast< quint32*>(_v)); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
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
QT_END_MOC_NAMESPACE
