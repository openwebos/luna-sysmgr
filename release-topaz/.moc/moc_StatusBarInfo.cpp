/****************************************************************************
** Meta object code from reading C++ file 'StatusBarInfo.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarInfo.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarInfo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarInfoItem[] = {

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

static const char qt_meta_stringdata_StatusBarInfoItem[] = {
    "StatusBarInfoItem\0"
};

void StatusBarInfoItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData StatusBarInfoItem::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarInfoItem::staticMetaObject = {
    { &StatusBarIcon::staticMetaObject, qt_meta_stringdata_StatusBarInfoItem,
      qt_meta_data_StatusBarInfoItem, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarInfoItem::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarInfoItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarInfoItem::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarInfoItem))
        return static_cast<void*>(const_cast< StatusBarInfoItem*>(this));
    return StatusBarIcon::qt_metacast(_clname);
}

int StatusBarInfoItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarIcon::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_StatusBarInfo[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x08,
      45,   14,   14,   14, 0x08,
      73,   67,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarInfo[] = {
    "StatusBarInfo\0\0slotRoamingIndicatorChanged()\0"
    "slotDualRssiEnabled()\0state\0"
    "slotAirplaneModeState(t_airplaneModeState)\0"
};

void StatusBarInfo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarInfo *_t = static_cast<StatusBarInfo *>(_o);
        switch (_id) {
        case 0: _t->slotRoamingIndicatorChanged(); break;
        case 1: _t->slotDualRssiEnabled(); break;
        case 2: _t->slotAirplaneModeState((*reinterpret_cast< t_airplaneModeState(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBarInfo::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarInfo::staticMetaObject = {
    { &StatusBarItem::staticMetaObject, qt_meta_stringdata_StatusBarInfo,
      qt_meta_data_StatusBarInfo, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarInfo::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarInfo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarInfo::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarInfo))
        return static_cast<void*>(const_cast< StatusBarInfo*>(this));
    if (!strcmp(_clname, "StatusBarIconContainer"))
        return static_cast< StatusBarIconContainer*>(const_cast< StatusBarInfo*>(this));
    return StatusBarItem::qt_metacast(_clname);
}

int StatusBarInfo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
