/****************************************************************************
** Meta object code from reading C++ file 'IMEData.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/ime/IMEData.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'IMEData.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_IMEData_QSize[] = {

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
      24,   15,   14,   14, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_QSize[] = {
    "IMEData_QSize\0\0newValue\0valueChanged(QSize)\0"
};

void IMEData_QSize::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_QSize *_t = static_cast<IMEData_QSize *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_QSize::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_QSize::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_QSize,
      qt_meta_data_IMEData_QSize, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_QSize::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_QSize::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_QSize::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_QSize))
        return static_cast<void*>(const_cast< IMEData_QSize*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_QSize::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_QSize::valueChanged(const QSize & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_QRect[] = {

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
      24,   15,   14,   14, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_QRect[] = {
    "IMEData_QRect\0\0newValue\0valueChanged(QRect)\0"
};

void IMEData_QRect::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_QRect *_t = static_cast<IMEData_QRect *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_QRect::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_QRect::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_QRect,
      qt_meta_data_IMEData_QRect, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_QRect::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_QRect::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_QRect::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_QRect))
        return static_cast<void*>(const_cast< IMEData_QRect*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_QRect::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_QRect::valueChanged(const QRect & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_QRegion[] = {

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
      26,   17,   16,   16, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_QRegion[] = {
    "IMEData_QRegion\0\0newValue\0"
    "valueChanged(QRegion)\0"
};

void IMEData_QRegion::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_QRegion *_t = static_cast<IMEData_QRegion *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const QRegion(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_QRegion::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_QRegion::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_QRegion,
      qt_meta_data_IMEData_QRegion, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_QRegion::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_QRegion::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_QRegion::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_QRegion))
        return static_cast<void*>(const_cast< IMEData_QRegion*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_QRegion::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_QRegion::valueChanged(const QRegion & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_bool[] = {

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
      23,   14,   13,   13, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_bool[] = {
    "IMEData_bool\0\0newValue\0valueChanged(bool)\0"
};

void IMEData_bool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_bool *_t = static_cast<IMEData_bool *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_bool::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_bool::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_bool,
      qt_meta_data_IMEData_bool, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_bool::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_bool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_bool::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_bool))
        return static_cast<void*>(const_cast< IMEData_bool*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_bool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_bool::valueChanged(const bool & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_qint32[] = {

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
      25,   16,   15,   15, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_qint32[] = {
    "IMEData_qint32\0\0newValue\0valueChanged(qint32)\0"
};

void IMEData_qint32::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_qint32 *_t = static_cast<IMEData_qint32 *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const qint32(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_qint32::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_qint32::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_qint32,
      qt_meta_data_IMEData_qint32, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_qint32::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_qint32::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_qint32::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_qint32))
        return static_cast<void*>(const_cast< IMEData_qint32*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_qint32::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_qint32::valueChanged(const qint32 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_QString[] = {

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
      26,   17,   16,   16, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_QString[] = {
    "IMEData_QString\0\0newValue\0"
    "valueChanged(QString)\0"
};

void IMEData_QString::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_QString *_t = static_cast<IMEData_QString *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_QString::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_QString::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_QString,
      qt_meta_data_IMEData_QString, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_QString::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_QString::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_QString::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_QString))
        return static_cast<void*>(const_cast< IMEData_QString*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_QString::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_QString::valueChanged(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_IMEData_EditorState[] = {

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
      30,   21,   20,   20, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_IMEData_EditorState[] = {
    "IMEData_EditorState\0\0newValue\0"
    "valueChanged(PalmIME::EditorState)\0"
};

void IMEData_EditorState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        IMEData_EditorState *_t = static_cast<IMEData_EditorState *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< const PalmIME::EditorState(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData IMEData_EditorState::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject IMEData_EditorState::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IMEData_EditorState,
      qt_meta_data_IMEData_EditorState, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &IMEData_EditorState::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *IMEData_EditorState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *IMEData_EditorState::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_IMEData_EditorState))
        return static_cast<void*>(const_cast< IMEData_EditorState*>(this));
    return QObject::qt_metacast(_clname);
}

int IMEData_EditorState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void IMEData_EditorState::valueChanged(const PalmIME::EditorState & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
