/****************************************************************************
** Meta object code from reading C++ file 'CardWindowManager.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/cards/CardWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CardWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CardWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   19,   18,   18, 0x05,
      54,   18,   18,   18, 0x05,
      82,   19,   18,   18, 0x05,
     117,   18,   18,   18, 0x05,
     146,   18,   18,   18, 0x05,
     184,  175,   18,   18, 0x05,
     224,  215,   18,   18, 0x05,
     248,   18,   18,   18, 0x25,
     268,   18,   18,   18, 0x05,

 // slots: signature, parameters, type, tag, flags
     317,  289,   18,   18, 0x08,
     367,  365,   18,   18, 0x08,
     406,  365,   18,   18, 0x08,
     455,  438,   18,   18, 0x08,
     490,  486,   18,   18, 0x08,
     514,   18,   18,   18, 0x08,
     539,   18,   18,   18, 0x08,
     570,   18,   18,   18, 0x08,
     606,   18,   18,   18, 0x08,
     637,   18,   18,   18, 0x08,
     673,  668,   18,   18, 0x08,
     706,  700,   18,   18, 0x08,
     747,  741,   18,   18, 0x08,
     793,   18,   18,   18, 0x08,
     824,   18,   18,   18, 0x08,
     855,   18,   18,   18, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CardWindowManager[] = {
    "CardWindowManager\0\0win\0"
    "signalFocusWindow(CardWindow*)\0"
    "signalLoadingActiveWindow()\0"
    "signalPreparingWindow(CardWindow*)\0"
    "signalMaximizeActiveWindow()\0"
    "signalMinimizeActiveWindow()\0pt,slice\0"
    "signalEnterReorder(QPoint,int)\0canceled\0"
    "signalExitReorder(bool)\0signalExitReorder()\0"
    "signalFirstCardRun()\0r,fullScreen,screenResizing\0"
    "slotPositiveSpaceAboutToChange(QRect,bool,bool)\0"
    "r\0slotPositiveSpaceChangeFinished(QRect)\0"
    "slotPositiveSpaceChanged(QRect)\0"
    "val,fullyVisible\0slotLauncherVisible(bool,bool)\0"
    "val\0slotLauncherShown(bool)\0"
    "slotAnimationsFinished()\0"
    "slotDeletedAnimationFinished()\0"
    "slotTouchToShareAnimationFinished()\0"
    "slotMaximizeActiveCardWindow()\0"
    "slotMinimizeActiveCardWindow()\0next\0"
    "slotChangeCardWindow(bool)\0focus\0"
    "slotFocusMaximizedCardWindow(bool)\0"
    "appId\0slotTouchToShareAppUrlTransfered(std::string)\0"
    "slotOpacityAnimationFinished()\0"
    "slotDismissActiveModalWindow()\0"
    "slotDismissModalTimerStopped()\0"
};

void CardWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CardWindowManager *_t = static_cast<CardWindowManager *>(_o);
        switch (_id) {
        case 0: _t->signalFocusWindow((*reinterpret_cast< CardWindow*(*)>(_a[1]))); break;
        case 1: _t->signalLoadingActiveWindow(); break;
        case 2: _t->signalPreparingWindow((*reinterpret_cast< CardWindow*(*)>(_a[1]))); break;
        case 3: _t->signalMaximizeActiveWindow(); break;
        case 4: _t->signalMinimizeActiveWindow(); break;
        case 5: _t->signalEnterReorder((*reinterpret_cast< QPoint(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->signalExitReorder((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->signalExitReorder(); break;
        case 8: _t->signalFirstCardRun(); break;
        case 9: _t->slotPositiveSpaceAboutToChange((*reinterpret_cast< const QRect(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 10: _t->slotPositiveSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 11: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 12: _t->slotLauncherVisible((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 13: _t->slotLauncherShown((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->slotAnimationsFinished(); break;
        case 15: _t->slotDeletedAnimationFinished(); break;
        case 16: _t->slotTouchToShareAnimationFinished(); break;
        case 17: _t->slotMaximizeActiveCardWindow(); break;
        case 18: _t->slotMinimizeActiveCardWindow(); break;
        case 19: _t->slotChangeCardWindow((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 20: _t->slotFocusMaximizedCardWindow((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 21: _t->slotTouchToShareAppUrlTransfered((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 22: _t->slotOpacityAnimationFinished(); break;
        case 23: _t->slotDismissActiveModalWindow(); break;
        case 24: _t->slotDismissModalTimerStopped(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CardWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CardWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_CardWindowManager,
      qt_meta_data_CardWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CardWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CardWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CardWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CardWindowManager))
        return static_cast<void*>(const_cast< CardWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int CardWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void CardWindowManager::signalFocusWindow(CardWindow * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CardWindowManager::signalLoadingActiveWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void CardWindowManager::signalPreparingWindow(CardWindow * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CardWindowManager::signalMaximizeActiveWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void CardWindowManager::signalMinimizeActiveWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void CardWindowManager::signalEnterReorder(QPoint _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CardWindowManager::signalExitReorder(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 8
void CardWindowManager::signalFirstCardRun()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}
QT_END_MOC_NAMESPACE
