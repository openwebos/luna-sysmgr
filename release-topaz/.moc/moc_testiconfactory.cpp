/****************************************************************************
** Meta object code from reading C++ file 'testiconfactory.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/test/testiconfactory.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'testiconfactory.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DimensionsUITest__IconFactory[] = {

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
      41,   31,   30,   30, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DimensionsUITest__IconFactory[] = {
    "DimensionsUITest::IconFactory\0\0p_qobject\0"
    "slotPmoDestroyed(QObject*)\0"
};

void DimensionsUITest::IconFactory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IconFactory *_t = static_cast<IconFactory *>(_o);
        switch (_id) {
        case 0: _t->slotPmoDestroyed((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DimensionsUITest::IconFactory::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DimensionsUITest::IconFactory::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DimensionsUITest__IconFactory,
      qt_meta_data_DimensionsUITest__IconFactory, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DimensionsUITest::IconFactory::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DimensionsUITest::IconFactory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DimensionsUITest::IconFactory::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DimensionsUITest__IconFactory))
        return static_cast<void*>(const_cast< IconFactory*>(this));
    return QObject::qt_metacast(_clname);
}

int DimensionsUITest::IconFactory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
QT_END_MOC_NAMESPACE
