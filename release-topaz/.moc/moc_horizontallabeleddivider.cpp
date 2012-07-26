/****************************************************************************
** Meta object code from reading C++ file 'horizontallabeleddivider.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/static/horizontallabeleddivider.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'horizontallabeleddivider.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_HorizontalLabeledDivider[] = {

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
      26,   25,   25,   25, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_HorizontalLabeledDivider[] = {
    "HorizontalLabeledDivider\0\0"
    "slotPixmapObjectInvalidated()\0"
};

void HorizontalLabeledDivider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        HorizontalLabeledDivider *_t = static_cast<HorizontalLabeledDivider *>(_o);
        switch (_id) {
        case 0: _t->slotPixmapObjectInvalidated(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData HorizontalLabeledDivider::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject HorizontalLabeledDivider::staticMetaObject = {
    { &HorizontalDivider::staticMetaObject, qt_meta_stringdata_HorizontalLabeledDivider,
      qt_meta_data_HorizontalLabeledDivider, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &HorizontalLabeledDivider::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *HorizontalLabeledDivider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *HorizontalLabeledDivider::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_HorizontalLabeledDivider))
        return static_cast<void*>(const_cast< HorizontalLabeledDivider*>(this));
    return HorizontalDivider::qt_metacast(_clname);
}

int HorizontalLabeledDivider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = HorizontalDivider::qt_metacall(_c, _id, _a);
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
