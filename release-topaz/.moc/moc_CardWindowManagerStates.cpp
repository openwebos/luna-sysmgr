/****************************************************************************
** Meta object code from reading C++ file 'CardWindowManagerStates.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/cards/CardWindowManagerStates.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CardWindowManagerStates.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CardWindowManagerState[] = {

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

static const char qt_meta_stringdata_CardWindowManagerState[] = {
    "CardWindowManagerState\0"
};

void CardWindowManagerState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CardWindowManagerState::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CardWindowManagerState::staticMetaObject = {
    { &QState::staticMetaObject, qt_meta_stringdata_CardWindowManagerState,
      qt_meta_data_CardWindowManagerState, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CardWindowManagerState::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CardWindowManagerState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CardWindowManagerState::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CardWindowManagerState))
        return static_cast<void*>(const_cast< CardWindowManagerState*>(this));
    return QState::qt_metacast(_clname);
}

int CardWindowManagerState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QState::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_MinimizeState[] = {

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
      15,   14,   14,   14, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_MinimizeState[] = {
    "MinimizeState\0\0signalFirstCardRun()\0"
};

void MinimizeState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MinimizeState *_t = static_cast<MinimizeState *>(_o);
        switch (_id) {
        case 0: _t->signalFirstCardRun(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData MinimizeState::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MinimizeState::staticMetaObject = {
    { &CardWindowManagerState::staticMetaObject, qt_meta_stringdata_MinimizeState,
      qt_meta_data_MinimizeState, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MinimizeState::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MinimizeState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MinimizeState::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MinimizeState))
        return static_cast<void*>(const_cast< MinimizeState*>(this));
    return CardWindowManagerState::qt_metacast(_clname);
}

int MinimizeState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CardWindowManagerState::qt_metacall(_c, _id, _a);
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
void MinimizeState::signalFirstCardRun()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_MaximizeState[] = {

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
      15,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MaximizeState[] = {
    "MaximizeState\0\0slotIncomingPhoneCall()\0"
};

void MaximizeState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MaximizeState *_t = static_cast<MaximizeState *>(_o);
        switch (_id) {
        case 0: _t->slotIncomingPhoneCall(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData MaximizeState::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MaximizeState::staticMetaObject = {
    { &CardWindowManagerState::staticMetaObject, qt_meta_stringdata_MaximizeState,
      qt_meta_data_MaximizeState, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MaximizeState::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MaximizeState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MaximizeState::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MaximizeState))
        return static_cast<void*>(const_cast< MaximizeState*>(this));
    return CardWindowManagerState::qt_metacast(_clname);
}

int MaximizeState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CardWindowManagerState::qt_metacall(_c, _id, _a);
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
