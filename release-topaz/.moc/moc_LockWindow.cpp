/****************************************************************************
** Meta object code from reading C++ file 'LockWindow.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/lockscreen/LockWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LockWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_LockWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   12,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      54,   48,   11,   11, 0x08,
     102,   83,   11,   11, 0x08,
     132,   11,   11,   11, 0x08,
     153,   11,   11,   11, 0x08,
     176,   11,   11,   11, 0x08,
     206,   11,   11,   11, 0x08,
     238,   11,   11,   11, 0x08,
     260,   11,   11,   11, 0x08,
     284,   11,   11,   11, 0x08,
     305,   11,   11,   11, 0x08,
     346,  331,   11,   11, 0x08,
     395,  382,   11,   11, 0x08,
     433,  426,   11,   11, 0x08,
     465,   11,   11,   11, 0x08,
     488,  484,   11,   11, 0x08,
     522,  515,   11,   11, 0x08,
     565,  557,   11,   11, 0x08,
     594,   11,   11,   11, 0x08,
     621,   11,   11,   11, 0x08,
     648,   11,   11,   11, 0x08,
     675,   11,   11,   11, 0x08,
     709,   11,   11,   11, 0x08,
     755,  733,   11,   11, 0x08,
     805,  803,   11,   11, 0x08,
     837,  803,   11,   11, 0x08,
     876,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_LockWindow[] = {
    "LockWindow\0\0screenState\0signalScreenLocked(int)\0"
    "event\0slotDisplayStateChanged(int)\0"
    "event,displayEvent\0slotLockStateChanged(int,int)\0"
    "slotAlertActivated()\0slotAlertDeactivated()\0"
    "slotTransientAlertActivated()\0"
    "slotTransientAlertDeactivated()\0"
    "slotBannerActivated()\0slotBannerDeactivated()\0"
    "slotDeviceUnlocked()\0slotCancelPasswordEntry()\0"
    "password,isPIN\0slotPasswordSubmitted(QString,bool)\0"
    "focusRequest\0slotPinPanelFocusRequest(bool)\0"
    "target\0slotBannerAboutToUpdate(QRect&)\0"
    "slotBootFinished()\0win\0"
    "slotWindowUpdated(Window*)\0policy\0"
    "slotPolicyChanged(EASPolicy*const)\0"
    "timeout\0slotSetLockTimeout(uint32_t)\0"
    "slotDialogButton1Pressed()\0"
    "slotDialogButton2Pressed()\0"
    "slotDialogButton3Pressed()\0"
    "slotWindowFadeAnimationFinished()\0"
    "slotVisibilityChanged()\0r,fullscreen,resizing\0"
    "slotPositiveSpaceAboutToChange(QRect,bool,bool)\0"
    "r\0slotPositiveSpaceChanged(QRect)\0"
    "slotPositiveSpaceChangeFinished(QRect)\0"
    "slotUiRotationCompleted()\0"
};

void LockWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LockWindow *_t = static_cast<LockWindow *>(_o);
        switch (_id) {
        case 0: _t->signalScreenLocked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->slotDisplayStateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->slotLockStateChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->slotAlertActivated(); break;
        case 4: _t->slotAlertDeactivated(); break;
        case 5: _t->slotTransientAlertActivated(); break;
        case 6: _t->slotTransientAlertDeactivated(); break;
        case 7: _t->slotBannerActivated(); break;
        case 8: _t->slotBannerDeactivated(); break;
        case 9: _t->slotDeviceUnlocked(); break;
        case 10: _t->slotCancelPasswordEntry(); break;
        case 11: _t->slotPasswordSubmitted((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 12: _t->slotPinPanelFocusRequest((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->slotBannerAboutToUpdate((*reinterpret_cast< QRect(*)>(_a[1]))); break;
        case 14: _t->slotBootFinished(); break;
        case 15: _t->slotWindowUpdated((*reinterpret_cast< Window*(*)>(_a[1]))); break;
        case 16: _t->slotPolicyChanged((*reinterpret_cast< const EASPolicy*const(*)>(_a[1]))); break;
        case 17: _t->slotSetLockTimeout((*reinterpret_cast< uint32_t(*)>(_a[1]))); break;
        case 18: _t->slotDialogButton1Pressed(); break;
        case 19: _t->slotDialogButton2Pressed(); break;
        case 20: _t->slotDialogButton3Pressed(); break;
        case 21: _t->slotWindowFadeAnimationFinished(); break;
        case 22: _t->slotVisibilityChanged(); break;
        case 23: _t->slotPositiveSpaceAboutToChange((*reinterpret_cast< const QRect(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 24: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 25: _t->slotPositiveSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 26: _t->slotUiRotationCompleted(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData LockWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject LockWindow::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_LockWindow,
      qt_meta_data_LockWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &LockWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *LockWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *LockWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_LockWindow))
        return static_cast<void*>(const_cast< LockWindow*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int LockWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    return _id;
}

// SIGNAL 0
void LockWindow::signalScreenLocked(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
