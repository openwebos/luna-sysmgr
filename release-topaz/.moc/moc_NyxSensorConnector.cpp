/****************************************************************************
** Meta object code from reading C++ file 'NyxSensorConnector.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/nyx/NyxSensorConnector.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NyxSensorConnector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_NYXConnectorBase[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
      48,   40,   17,   17, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXConnectorBase[] = {
    "NYXConnectorBase\0\0sensorDataAvailable()\0"
    "aSocket\0readSensorData(int)\0"
};

void NYXConnectorBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXConnectorBase *_t = static_cast<NYXConnectorBase *>(_o);
        switch (_id) {
        case 0: _t->sensorDataAvailable(); break;
        case 1: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXConnectorBase::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXConnectorBase::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_NYXConnectorBase,
      qt_meta_data_NYXConnectorBase, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXConnectorBase::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXConnectorBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXConnectorBase::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXConnectorBase))
        return static_cast<void*>(const_cast< NYXConnectorBase*>(this));
    return QObject::qt_metacast(_clname);
}

int NYXConnectorBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void NYXConnectorBase::sensorDataAvailable()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_NYXAccelerationSensorConnector[] = {

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
      40,   32,   31,   31, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXAccelerationSensorConnector[] = {
    "NYXAccelerationSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXAccelerationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXAccelerationSensorConnector *_t = static_cast<NYXAccelerationSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXAccelerationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXAccelerationSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXAccelerationSensorConnector,
      qt_meta_data_NYXAccelerationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXAccelerationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXAccelerationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXAccelerationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXAccelerationSensorConnector))
        return static_cast<void*>(const_cast< NYXAccelerationSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXAccelerationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXAlsSensorConnector[] = {

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
      31,   23,   22,   22, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXAlsSensorConnector[] = {
    "NYXAlsSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXAlsSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXAlsSensorConnector *_t = static_cast<NYXAlsSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXAlsSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXAlsSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXAlsSensorConnector,
      qt_meta_data_NYXAlsSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXAlsSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXAlsSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXAlsSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXAlsSensorConnector))
        return static_cast<void*>(const_cast< NYXAlsSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXAlsSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXAngularVelocitySensorConnector[] = {

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
      43,   35,   34,   34, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXAngularVelocitySensorConnector[] = {
    "NYXAngularVelocitySensorConnector\0\0"
    "aSocket\0readSensorData(int)\0"
};

void NYXAngularVelocitySensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXAngularVelocitySensorConnector *_t = static_cast<NYXAngularVelocitySensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXAngularVelocitySensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXAngularVelocitySensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXAngularVelocitySensorConnector,
      qt_meta_data_NYXAngularVelocitySensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXAngularVelocitySensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXAngularVelocitySensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXAngularVelocitySensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXAngularVelocitySensorConnector))
        return static_cast<void*>(const_cast< NYXAngularVelocitySensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXAngularVelocitySensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXBearingSensorConnector[] = {

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
      35,   27,   26,   26, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXBearingSensorConnector[] = {
    "NYXBearingSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXBearingSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXBearingSensorConnector *_t = static_cast<NYXBearingSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXBearingSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXBearingSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXBearingSensorConnector,
      qt_meta_data_NYXBearingSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXBearingSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXBearingSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXBearingSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXBearingSensorConnector))
        return static_cast<void*>(const_cast< NYXBearingSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXBearingSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXGravitySensorConnector[] = {

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
      35,   27,   26,   26, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXGravitySensorConnector[] = {
    "NYXGravitySensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXGravitySensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXGravitySensorConnector *_t = static_cast<NYXGravitySensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXGravitySensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXGravitySensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXGravitySensorConnector,
      qt_meta_data_NYXGravitySensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXGravitySensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXGravitySensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXGravitySensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXGravitySensorConnector))
        return static_cast<void*>(const_cast< NYXGravitySensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXGravitySensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXLinearAccelearationSensorConnector[] = {

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
      47,   39,   38,   38, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXLinearAccelearationSensorConnector[] = {
    "NYXLinearAccelearationSensorConnector\0"
    "\0aSocket\0readSensorData(int)\0"
};

void NYXLinearAccelearationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXLinearAccelearationSensorConnector *_t = static_cast<NYXLinearAccelearationSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXLinearAccelearationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLinearAccelearationSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLinearAccelearationSensorConnector,
      qt_meta_data_NYXLinearAccelearationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLinearAccelearationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLinearAccelearationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLinearAccelearationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLinearAccelearationSensorConnector))
        return static_cast<void*>(const_cast< NYXLinearAccelearationSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXLinearAccelearationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXMagneticFieldSensorConnector[] = {

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
      41,   33,   32,   32, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXMagneticFieldSensorConnector[] = {
    "NYXMagneticFieldSensorConnector\0\0"
    "aSocket\0readSensorData(int)\0"
};

void NYXMagneticFieldSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXMagneticFieldSensorConnector *_t = static_cast<NYXMagneticFieldSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXMagneticFieldSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXMagneticFieldSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXMagneticFieldSensorConnector,
      qt_meta_data_NYXMagneticFieldSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXMagneticFieldSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXMagneticFieldSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXMagneticFieldSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXMagneticFieldSensorConnector))
        return static_cast<void*>(const_cast< NYXMagneticFieldSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXMagneticFieldSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXOrientationSensorConnector[] = {

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
      39,   31,   30,   30, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXOrientationSensorConnector[] = {
    "NYXOrientationSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXOrientationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXOrientationSensorConnector *_t = static_cast<NYXOrientationSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXOrientationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXOrientationSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXOrientationSensorConnector,
      qt_meta_data_NYXOrientationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXOrientationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXOrientationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXOrientationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXOrientationSensorConnector))
        return static_cast<void*>(const_cast< NYXOrientationSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXOrientationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXScreenProximitySensorConnector[] = {

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
      43,   35,   34,   34, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXScreenProximitySensorConnector[] = {
    "NYXScreenProximitySensorConnector\0\0"
    "aSocket\0readSensorData(int)\0"
};

void NYXScreenProximitySensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXScreenProximitySensorConnector *_t = static_cast<NYXScreenProximitySensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXScreenProximitySensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXScreenProximitySensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXScreenProximitySensorConnector,
      qt_meta_data_NYXScreenProximitySensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXScreenProximitySensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXScreenProximitySensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXScreenProximitySensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXScreenProximitySensorConnector))
        return static_cast<void*>(const_cast< NYXScreenProximitySensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXScreenProximitySensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXRotationSensorConnector[] = {

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
      36,   28,   27,   27, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXRotationSensorConnector[] = {
    "NYXRotationSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXRotationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXRotationSensorConnector *_t = static_cast<NYXRotationSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXRotationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXRotationSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXRotationSensorConnector,
      qt_meta_data_NYXRotationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXRotationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXRotationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXRotationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXRotationSensorConnector))
        return static_cast<void*>(const_cast< NYXRotationSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXRotationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXShakeSensorConnector[] = {

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
      33,   25,   24,   24, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXShakeSensorConnector[] = {
    "NYXShakeSensorConnector\0\0aSocket\0"
    "readSensorData(int)\0"
};

void NYXShakeSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXShakeSensorConnector *_t = static_cast<NYXShakeSensorConnector *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXShakeSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXShakeSensorConnector::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXShakeSensorConnector,
      qt_meta_data_NYXShakeSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXShakeSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXShakeSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXShakeSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXShakeSensorConnector))
        return static_cast<void*>(const_cast< NYXShakeSensorConnector*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXShakeSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_NYXLogicalSensorConnectorBase[] = {

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
      31,   30,   30,   30, 0x09,
      51,   30,   30,   30, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_NYXLogicalSensorConnectorBase[] = {
    "NYXLogicalSensorConnectorBase\0\0"
    "readSensorData(int)\0logicalSensorDataAvailable()\0"
};

void NYXLogicalSensorConnectorBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NYXLogicalSensorConnectorBase *_t = static_cast<NYXLogicalSensorConnectorBase *>(_o);
        switch (_id) {
        case 0: _t->readSensorData((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->logicalSensorDataAvailable(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NYXLogicalSensorConnectorBase::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLogicalSensorConnectorBase::staticMetaObject = {
    { &NYXConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLogicalSensorConnectorBase,
      qt_meta_data_NYXLogicalSensorConnectorBase, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLogicalSensorConnectorBase::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLogicalSensorConnectorBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLogicalSensorConnectorBase::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLogicalSensorConnectorBase))
        return static_cast<void*>(const_cast< NYXLogicalSensorConnectorBase*>(this));
    return NYXConnectorBase::qt_metacast(_clname);
}

int NYXLogicalSensorConnectorBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_NYXLogicalAccelerometerSensorConnector[] = {

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

static const char qt_meta_stringdata_NYXLogicalAccelerometerSensorConnector[] = {
    "NYXLogicalAccelerometerSensorConnector\0"
};

void NYXLogicalAccelerometerSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData NYXLogicalAccelerometerSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLogicalAccelerometerSensorConnector::staticMetaObject = {
    { &NYXLogicalSensorConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLogicalAccelerometerSensorConnector,
      qt_meta_data_NYXLogicalAccelerometerSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLogicalAccelerometerSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLogicalAccelerometerSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLogicalAccelerometerSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLogicalAccelerometerSensorConnector))
        return static_cast<void*>(const_cast< NYXLogicalAccelerometerSensorConnector*>(this));
    return NYXLogicalSensorConnectorBase::qt_metacast(_clname);
}

int NYXLogicalAccelerometerSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXLogicalSensorConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_NYXLogicalOrientationSensorConnector[] = {

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

static const char qt_meta_stringdata_NYXLogicalOrientationSensorConnector[] = {
    "NYXLogicalOrientationSensorConnector\0"
};

void NYXLogicalOrientationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData NYXLogicalOrientationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLogicalOrientationSensorConnector::staticMetaObject = {
    { &NYXLogicalSensorConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLogicalOrientationSensorConnector,
      qt_meta_data_NYXLogicalOrientationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLogicalOrientationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLogicalOrientationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLogicalOrientationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLogicalOrientationSensorConnector))
        return static_cast<void*>(const_cast< NYXLogicalOrientationSensorConnector*>(this));
    return NYXLogicalSensorConnectorBase::qt_metacast(_clname);
}

int NYXLogicalOrientationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXLogicalSensorConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_NYXLogicalDeviceOrientationSensorConnector[] = {

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

static const char qt_meta_stringdata_NYXLogicalDeviceOrientationSensorConnector[] = {
    "NYXLogicalDeviceOrientationSensorConnector\0"
};

void NYXLogicalDeviceOrientationSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData NYXLogicalDeviceOrientationSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLogicalDeviceOrientationSensorConnector::staticMetaObject = {
    { &NYXLogicalSensorConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLogicalDeviceOrientationSensorConnector,
      qt_meta_data_NYXLogicalDeviceOrientationSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLogicalDeviceOrientationSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLogicalDeviceOrientationSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLogicalDeviceOrientationSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLogicalDeviceOrientationSensorConnector))
        return static_cast<void*>(const_cast< NYXLogicalDeviceOrientationSensorConnector*>(this));
    return NYXLogicalSensorConnectorBase::qt_metacast(_clname);
}

int NYXLogicalDeviceOrientationSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXLogicalSensorConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_NYXLogicalDeviceMotionSensorConnector[] = {

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

static const char qt_meta_stringdata_NYXLogicalDeviceMotionSensorConnector[] = {
    "NYXLogicalDeviceMotionSensorConnector\0"
};

void NYXLogicalDeviceMotionSensorConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData NYXLogicalDeviceMotionSensorConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NYXLogicalDeviceMotionSensorConnector::staticMetaObject = {
    { &NYXLogicalSensorConnectorBase::staticMetaObject, qt_meta_stringdata_NYXLogicalDeviceMotionSensorConnector,
      qt_meta_data_NYXLogicalDeviceMotionSensorConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NYXLogicalDeviceMotionSensorConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NYXLogicalDeviceMotionSensorConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NYXLogicalDeviceMotionSensorConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NYXLogicalDeviceMotionSensorConnector))
        return static_cast<void*>(const_cast< NYXLogicalDeviceMotionSensorConnector*>(this));
    return NYXLogicalSensorConnectorBase::qt_metacast(_clname);
}

int NYXLogicalDeviceMotionSensorConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NYXLogicalSensorConnectorBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
