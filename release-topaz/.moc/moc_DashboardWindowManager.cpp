/****************************************************************************
** Meta object code from reading C++ file 'DashboardWindowManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/notifications/DashboardWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DashboardWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DashboardWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   23,   23,   23, 0x05,
      57,   23,   23,   23, 0x05,
      81,   70,   23,   23, 0x05,
      99,   23,   23,   23, 0x05,
     140,   23,   23,   23, 0x05,

 // slots: signature, parameters, type, tag, flags
     173,  171,   23,   23, 0x08,
     205,  171,   23,   23, 0x08,
     244,  171,   23,   23, 0x08,
     276,  171,   23,   23, 0x08,
     315,   23,   23,   23, 0x08,
     335,   70,   23,   23, 0x08,
     360,   23,   23,   23, 0x08,
     379,  377,   23,   23, 0x08,
     422,  377,   23,   23, 0x08,
     468,   23,   23,   23, 0x08,
     505,   23,   23,   23, 0x08,
     535,   23,   23,   23, 0x08,
     575,  568,   23,   23, 0x08,
     613,   23,   23,   23, 0x08,
     644,   23,   23,   23, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DashboardWindowManager[] = {
    "DashboardWindowManager\0\0"
    "signalActiveAlertWindowChanged()\0"
    "signalOpen()\0forceClose\0signalClose(bool)\0"
    "signalAlertWindowActivated(AlertWindow*)\0"
    "signalAlertWindowDeactivated()\0r\0"
    "slotPositiveSpaceChanged(QRect)\0"
    "slotPositiveSpaceChangeFinished(QRect)\0"
    "slotNegativeSpaceChanged(QRect)\0"
    "slotNegativeSpaceChangeFinished(QRect)\0"
    "slotOpenDashboard()\0slotCloseDashboard(bool)\0"
    "slotCloseAlert()\0w\0"
    "slotDashboardWindowAdded(DashboardWindow*)\0"
    "slotDashboardWindowsRemoved(DashboardWindow*)\0"
    "slotDashboardViewportHeightChanged()\0"
    "slotDeleteAnimationFinished()\0"
    "slotTransientAnimationFinished()\0"
    "offset\0slotDashboardAreaRightEdgeOffset(int)\0"
    "slotDockModeAnimationStarted()\0"
    "slotDockModeAnimationComplete()\0"
};

void DashboardWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DashboardWindowManager *_t = static_cast<DashboardWindowManager *>(_o);
        switch (_id) {
        case 0: _t->signalActiveAlertWindowChanged(); break;
        case 1: _t->signalOpen(); break;
        case 2: _t->signalClose((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->signalAlertWindowActivated((*reinterpret_cast< AlertWindow*(*)>(_a[1]))); break;
        case 4: _t->signalAlertWindowDeactivated(); break;
        case 5: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 6: _t->slotPositiveSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 7: _t->slotNegativeSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 8: _t->slotNegativeSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 9: _t->slotOpenDashboard(); break;
        case 10: _t->slotCloseDashboard((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->slotCloseAlert(); break;
        case 12: _t->slotDashboardWindowAdded((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        case 13: _t->slotDashboardWindowsRemoved((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        case 14: _t->slotDashboardViewportHeightChanged(); break;
        case 15: _t->slotDeleteAnimationFinished(); break;
        case 16: _t->slotTransientAnimationFinished(); break;
        case 17: _t->slotDashboardAreaRightEdgeOffset((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 18: _t->slotDockModeAnimationStarted(); break;
        case 19: _t->slotDockModeAnimationComplete(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DashboardWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DashboardWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_DashboardWindowManager,
      qt_meta_data_DashboardWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DashboardWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DashboardWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DashboardWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DashboardWindowManager))
        return static_cast<void*>(const_cast< DashboardWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int DashboardWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void DashboardWindowManager::signalActiveAlertWindowChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void DashboardWindowManager::signalOpen()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void DashboardWindowManager::signalClose(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DashboardWindowManager::signalAlertWindowActivated(AlertWindow * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DashboardWindowManager::signalAlertWindowDeactivated()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
