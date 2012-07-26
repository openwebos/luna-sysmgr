/****************************************************************************
** Meta object code from reading C++ file 'HostArm.h'
**
** Created: Mon Jul 23 16:14:46 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/hosts/HostArm.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'HostArm.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_HostArm[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x09,
      23,    8,    8,    8, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_HostArm[] = {
    "HostArm\0\0readALSData()\0readProxData()\0"
};

void HostArm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        HostArm *_t = static_cast<HostArm *>(_o);
        switch (_id) {
        case 0: _t->readALSData(); break;
        case 1: _t->readProxData(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData HostArm::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject HostArm::staticMetaObject = {
    { &HostBase::staticMetaObject, qt_meta_stringdata_HostArm,
      qt_meta_data_HostArm, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &HostArm::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *HostArm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *HostArm::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_HostArm))
        return static_cast<void*>(const_cast< HostArm*>(this));
    if (!strcmp(_clname, "NYXConnectorObserver"))
        return static_cast< NYXConnectorObserver*>(const_cast< HostArm*>(this));
    return HostBase::qt_metacast(_clname);
}

int HostArm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = HostBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
