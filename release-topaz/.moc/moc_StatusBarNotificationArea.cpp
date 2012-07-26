/****************************************************************************
** Meta object code from reading C++ file 'StatusBarNotificationArea.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarNotificationArea.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarNotificationArea.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarNotificationIcon[] = {

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

static const char qt_meta_stringdata_StatusBarNotificationIcon[] = {
    "StatusBarNotificationIcon\0"
};

void StatusBarNotificationIcon::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData StatusBarNotificationIcon::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarNotificationIcon::staticMetaObject = {
    { &StatusBarIcon::staticMetaObject, qt_meta_stringdata_StatusBarNotificationIcon,
      qt_meta_data_StatusBarNotificationIcon, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarNotificationIcon::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarNotificationIcon::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarNotificationIcon::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarNotificationIcon))
        return static_cast<void*>(const_cast< StatusBarNotificationIcon*>(this));
    return StatusBarIcon::qt_metacast(_clname);
}

int StatusBarNotificationIcon::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarIcon::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_StatusBarNotificationArea[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      35,   27,   26,   26, 0x05,
      82,   26,   26,   26, 0x05,

 // slots: signature, parameters, type, tag, flags
     115,  113,   26,   26, 0x08,
     158,  113,   26,   26, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarNotificationArea[] = {
    "StatusBarNotificationArea\0\0visible\0"
    "signalNotificationArealVisibilityChanged(bool)\0"
    "signalBannerMessageActivated()\0w\0"
    "slotDashboardWindowAdded(DashboardWindow*)\0"
    "slotDashboardWindowsRemoved(DashboardWindow*)\0"
};

void StatusBarNotificationArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarNotificationArea *_t = static_cast<StatusBarNotificationArea *>(_o);
        switch (_id) {
        case 0: _t->signalNotificationArealVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->signalBannerMessageActivated(); break;
        case 2: _t->slotDashboardWindowAdded((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        case 3: _t->slotDashboardWindowsRemoved((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBarNotificationArea::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarNotificationArea::staticMetaObject = {
    { &StatusBarItem::staticMetaObject, qt_meta_stringdata_StatusBarNotificationArea,
      qt_meta_data_StatusBarNotificationArea, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarNotificationArea::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarNotificationArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarNotificationArea::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarNotificationArea))
        return static_cast<void*>(const_cast< StatusBarNotificationArea*>(this));
    if (!strcmp(_clname, "StatusBarIconContainer"))
        return static_cast< StatusBarIconContainer*>(const_cast< StatusBarNotificationArea*>(this));
    if (!strcmp(_clname, "BannerMessageView"))
        return static_cast< BannerMessageView*>(const_cast< StatusBarNotificationArea*>(this));
    return StatusBarItem::qt_metacast(_clname);
}

int StatusBarNotificationArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void StatusBarNotificationArea::signalNotificationArealVisibilityChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StatusBarNotificationArea::signalBannerMessageActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
