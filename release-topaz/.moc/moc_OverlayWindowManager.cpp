/****************************************************************************
** Meta object code from reading C++ file 'OverlayWindowManager.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/OverlayWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OverlayWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_OverlayWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      61,   14, // methods
       8,  319, // properties
       4,  343, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x05,
      48,   21,   21,   21, 0x05,
      74,   21,   21,   21, 0x05,
      98,   21,   21,   21, 0x05,
     122,   21,   21,   21, 0x05,
     153,   21,   21,   21, 0x05,
     184,   21,   21,   21, 0x05,
     204,   21,   21,   21, 0x05,
     224,   21,   21,   21, 0x05,
     247,   21,   21,   21, 0x05,
     268,   21,   21,   21, 0x05,
     287,   21,   21,   21, 0x05,
     310,   21,   21,   21, 0x05,
     333,   21,   21,   21, 0x05,
     356,   21,   21,   21, 0x05,
     386,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
     416,   21,   21,   21, 0x08,
     448,   21,   21,   21, 0x08,
     487,  479,   21,   21, 0x08,
     524,   21,   21,   21, 0x08,
     554,   21,   21,   21, 0x08,
     583,   21,   21,   21, 0x08,
     611,   21,   21,   21, 0x08,
     643,   21,   21,   21, 0x08,
     667,   21,   21,   21, 0x08,
     721,  693,   21,   21, 0x08,
     771,  769,   21,   21, 0x08,
     810,  769,   21,   21, 0x08,
     842,   21,   21,   21, 0x08,
     868,   21,   21,   21, 0x08,
     894,  479,   21,   21, 0x08,
     931,  925,   21,   21, 0x08,
    1007,   21,   21,   21, 0x28,
    1051,  925,   21,   21, 0x08,
    1127,   21,   21,   21, 0x28,
    1171,   21,   21,   21, 0x08,
    1191,   21,   21,   21, 0x08,
    1214,   21,   21,   21, 0x08,
    1237,   21,   21,   21, 0x08,
    1263,   21,   21,   21, 0x08,
    1278,   21,   21,   21, 0x08,
    1293,   21,   21,   21, 0x08,
    1325,   21,   21,   21, 0x08,
    1357,   21,   21,   21, 0x08,
    1379,   21,   21,   21, 0x08,
    1401,   21,   21,   21, 0x08,
    1432,   21,   21,   21, 0x08,
    1462,   21,   21,   21, 0x08,
    1495,   21,   21,   21, 0x08,
    1536, 1528,   21,   21, 0x08,
    1555,   21,   21,   21, 0x08,
    1576,   21,   21,   21, 0x08,
    1610, 1597,   21,   21, 0x08,
    1640,   21,   21,   21, 0x28,
    1689, 1666,   21,   21, 0x08,
    1724,   21,   21,   21, 0x08,
    1752,   21,   21,   21, 0x08,
    1776,   21,   21,   21, 0x08,
    1818,   21,   21,   21, 0x08,
    1840,   21,   21,   21, 0x08,
    1866,   21,   21,   21, 0x08,

 // properties: name, type, flags
    1904, 1894, 0x0009510b,
    1928, 1914, 0x0009510b,
    1958, 1942, 0x0009510b,
    1995, 1974, 0x0009510b,
    2021, 2016, 0x01095103,
    2031, 2016, 0x01095103,
    2059, 2052, 0x14095001,
    2090, 2082, 0x1a095001,

 // enums: name, flags, count, data
    1914, 0x0,    3,  359,
    1894, 0x0,    3,  365,
    1942, 0x0,    2,  371,
    1974, 0x0,    2,  375,

 // enum data: key, value
    2117, uint(OverlayWindowManager::StateNoLauncher),
    2133, uint(OverlayWindowManager::StateLauncherRegular),
    2154, uint(OverlayWindowManager::StateLauncherReorder),
    2175, uint(OverlayWindowManager::StateNoDock),
    2187, uint(OverlayWindowManager::StateDockNormal),
    2203, uint(OverlayWindowManager::StateDockReorder),
    2220, uint(OverlayWindowManager::StateSearchPillHidden),
    2242, uint(OverlayWindowManager::StateSearchPillVisible),
    2265, uint(OverlayWindowManager::StateUSearchHidden),
    2284, uint(OverlayWindowManager::StateUSearchVisible),

       0        // eod
};

static const char qt_meta_stringdata_OverlayWindowManager[] = {
    "OverlayWindowManager\0\0signalFSMShowSearchPill()\0"
    "signalFSMHideSearchPill()\0"
    "signalFSMShowLauncher()\0signalFSMHideLauncher()\0"
    "signalFSMShowUniversalSearch()\0"
    "signalFSMHideUniversalSearch()\0"
    "signalFSMShowDock()\0signalFSMHideDock()\0"
    "signalFSMDragStarted()\0signalFSMDragEnded()\0"
    "signalPenUpEvent()\0signalPenCancelEvent()\0"
    "signalLauncherOpened()\0signalLauncherClosed()\0"
    "signalUniversalSearchOpened()\0"
    "signalUniversalSearchClosed()\0"
    "slotFSMUniversalSearchVisible()\0"
    "slotFSMUniversalSearchHidden()\0enabled\0"
    "slotUniversalSearchFocusChange(bool)\0"
    "slotSystemAPIToggleLauncher()\0"
    "slotSystemAPICloseLauncher()\0"
    "slotSystemAPIOpenLauncher()\0"
    "slotSystemAPIRecreateLauncher()\0"
    "slotLauncherOpenState()\0"
    "slotLauncherClosedState()\0"
    "r,fullScreen,screenResizing\0"
    "slotPositiveSpaceAboutToChange(QRect,bool,bool)\0"
    "r\0slotPositiveSpaceChangeFinished(QRect)\0"
    "slotPositiveSpaceChanged(QRect)\0"
    "slotCardWindowMaximized()\0"
    "slotCardWindowMinimized()\0"
    "slotEmergencyModeChanged(bool)\0cause\0"
    "slotLauncherRequestShowDimensionsLauncher(DimensionsTypes::ShowCause::"
    "Enum)\0"
    "slotLauncherRequestShowDimensionsLauncher()\0"
    "slotLauncherRequestHideDimensionsLauncher(DimensionsTypes::HideCause::"
    "Enum)\0"
    "slotLauncherRequestHideDimensionsLauncher()\0"
    "slotLauncherReady()\0slotLauncherNotReady()\0"
    "slotQuickLaunchReady()\0slotQuickLaunchNotReady()\0"
    "slotShowDock()\0slotHideDock()\0"
    "slotStartShowLauncherSequence()\0"
    "slotStartHideLauncherSequence()\0"
    "slotAnimateShowDock()\0slotAnimateHideDock()\0"
    "slotAnimateFadeOutSearchPill()\0"
    "slotAnimateFadeInSearchPill()\0"
    "slotAnimateHideUniversalSearch()\0"
    "slotAnimateShowUniversalSearch()\0"
    "fadeOut\0slotFadeDock(bool)\0"
    "slotLauncherOpened()\0slotLauncherClosed()\0"
    "hideLauncher\0slotShowUniversalSearch(bool)\0"
    "slotShowUniversalSearch()\0"
    "showLauncher,speedDial\0"
    "slotHideUniversalSearch(bool,bool)\0"
    "launcherAnimationFinished()\0"
    "dockAnimationFinished()\0"
    "universalSearchOpacityAnimationFinished()\0"
    "dockStateTransition()\0launcherStateTransition()\0"
    "slotDisplayStateChange(int)\0DockState\0"
    "dockState\0LauncherState\0launcherState\0"
    "SearchPillState\0searchPillState\0"
    "UniversalSearchState\0universalSearchState\0"
    "bool\0dockShown\0universalSearchShown\0"
    "QRectF\0quickLaunchVisibleArea\0QPointF\0"
    "quickLaunchVisiblePosition\0StateNoLauncher\0"
    "StateLauncherRegular\0StateLauncherReorder\0"
    "StateNoDock\0StateDockNormal\0"
    "StateDockReorder\0StateSearchPillHidden\0"
    "StateSearchPillVisible\0StateUSearchHidden\0"
    "StateUSearchVisible\0"
};

void OverlayWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        OverlayWindowManager *_t = static_cast<OverlayWindowManager *>(_o);
        switch (_id) {
        case 0: _t->signalFSMShowSearchPill(); break;
        case 1: _t->signalFSMHideSearchPill(); break;
        case 2: _t->signalFSMShowLauncher(); break;
        case 3: _t->signalFSMHideLauncher(); break;
        case 4: _t->signalFSMShowUniversalSearch(); break;
        case 5: _t->signalFSMHideUniversalSearch(); break;
        case 6: _t->signalFSMShowDock(); break;
        case 7: _t->signalFSMHideDock(); break;
        case 8: _t->signalFSMDragStarted(); break;
        case 9: _t->signalFSMDragEnded(); break;
        case 10: _t->signalPenUpEvent(); break;
        case 11: _t->signalPenCancelEvent(); break;
        case 12: _t->signalLauncherOpened(); break;
        case 13: _t->signalLauncherClosed(); break;
        case 14: _t->signalUniversalSearchOpened(); break;
        case 15: _t->signalUniversalSearchClosed(); break;
        case 16: _t->slotFSMUniversalSearchVisible(); break;
        case 17: _t->slotFSMUniversalSearchHidden(); break;
        case 18: _t->slotUniversalSearchFocusChange((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 19: _t->slotSystemAPIToggleLauncher(); break;
        case 20: _t->slotSystemAPICloseLauncher(); break;
        case 21: _t->slotSystemAPIOpenLauncher(); break;
        case 22: _t->slotSystemAPIRecreateLauncher(); break;
        case 23: _t->slotLauncherOpenState(); break;
        case 24: _t->slotLauncherClosedState(); break;
        case 25: _t->slotPositiveSpaceAboutToChange((*reinterpret_cast< const QRect(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 26: _t->slotPositiveSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 27: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 28: _t->slotCardWindowMaximized(); break;
        case 29: _t->slotCardWindowMinimized(); break;
        case 30: _t->slotEmergencyModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 31: _t->slotLauncherRequestShowDimensionsLauncher((*reinterpret_cast< DimensionsTypes::ShowCause::Enum(*)>(_a[1]))); break;
        case 32: _t->slotLauncherRequestShowDimensionsLauncher(); break;
        case 33: _t->slotLauncherRequestHideDimensionsLauncher((*reinterpret_cast< DimensionsTypes::HideCause::Enum(*)>(_a[1]))); break;
        case 34: _t->slotLauncherRequestHideDimensionsLauncher(); break;
        case 35: _t->slotLauncherReady(); break;
        case 36: _t->slotLauncherNotReady(); break;
        case 37: _t->slotQuickLaunchReady(); break;
        case 38: _t->slotQuickLaunchNotReady(); break;
        case 39: _t->slotShowDock(); break;
        case 40: _t->slotHideDock(); break;
        case 41: _t->slotStartShowLauncherSequence(); break;
        case 42: _t->slotStartHideLauncherSequence(); break;
        case 43: _t->slotAnimateShowDock(); break;
        case 44: _t->slotAnimateHideDock(); break;
        case 45: _t->slotAnimateFadeOutSearchPill(); break;
        case 46: _t->slotAnimateFadeInSearchPill(); break;
        case 47: _t->slotAnimateHideUniversalSearch(); break;
        case 48: _t->slotAnimateShowUniversalSearch(); break;
        case 49: _t->slotFadeDock((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 50: _t->slotLauncherOpened(); break;
        case 51: _t->slotLauncherClosed(); break;
        case 52: _t->slotShowUniversalSearch((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 53: _t->slotShowUniversalSearch(); break;
        case 54: _t->slotHideUniversalSearch((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 55: _t->launcherAnimationFinished(); break;
        case 56: _t->dockAnimationFinished(); break;
        case 57: _t->universalSearchOpacityAnimationFinished(); break;
        case 58: _t->dockStateTransition(); break;
        case 59: _t->launcherStateTransition(); break;
        case 60: _t->slotDisplayStateChange((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData OverlayWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject OverlayWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_OverlayWindowManager,
      qt_meta_data_OverlayWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &OverlayWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *OverlayWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *OverlayWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_OverlayWindowManager))
        return static_cast<void*>(const_cast< OverlayWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int OverlayWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 61)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 61;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< DockState*>(_v) = dockState(); break;
        case 1: *reinterpret_cast< LauncherState*>(_v) = launcherState(); break;
        case 2: *reinterpret_cast< SearchPillState*>(_v) = searchPillState(); break;
        case 3: *reinterpret_cast< UniversalSearchState*>(_v) = universalSearchState(); break;
        case 4: *reinterpret_cast< bool*>(_v) = dockShown(); break;
        case 5: *reinterpret_cast< bool*>(_v) = universalSearchShown(); break;
        case 6: *reinterpret_cast< QRectF*>(_v) = quickLaunchVisibleArea(); break;
        case 7: *reinterpret_cast< QPointF*>(_v) = quickLaunchVisiblePosition(); break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setDockState(*reinterpret_cast< DockState*>(_v)); break;
        case 1: setLauncherState(*reinterpret_cast< LauncherState*>(_v)); break;
        case 2: setSearchPillState(*reinterpret_cast< SearchPillState*>(_v)); break;
        case 3: setUniversalSearchState(*reinterpret_cast< UniversalSearchState*>(_v)); break;
        case 4: setDockShown(*reinterpret_cast< bool*>(_v)); break;
        case 5: setUniversalSearchShown(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 8;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void OverlayWindowManager::signalFSMShowSearchPill()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void OverlayWindowManager::signalFSMHideSearchPill()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void OverlayWindowManager::signalFSMShowLauncher()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void OverlayWindowManager::signalFSMHideLauncher()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void OverlayWindowManager::signalFSMShowUniversalSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void OverlayWindowManager::signalFSMHideUniversalSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void OverlayWindowManager::signalFSMShowDock()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void OverlayWindowManager::signalFSMHideDock()
{
    QMetaObject::activate(this, &staticMetaObject, 7, 0);
}

// SIGNAL 8
void OverlayWindowManager::signalFSMDragStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void OverlayWindowManager::signalFSMDragEnded()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void OverlayWindowManager::signalPenUpEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 10, 0);
}

// SIGNAL 11
void OverlayWindowManager::signalPenCancelEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 11, 0);
}

// SIGNAL 12
void OverlayWindowManager::signalLauncherOpened()
{
    QMetaObject::activate(this, &staticMetaObject, 12, 0);
}

// SIGNAL 13
void OverlayWindowManager::signalLauncherClosed()
{
    QMetaObject::activate(this, &staticMetaObject, 13, 0);
}

// SIGNAL 14
void OverlayWindowManager::signalUniversalSearchOpened()
{
    QMetaObject::activate(this, &staticMetaObject, 14, 0);
}

// SIGNAL 15
void OverlayWindowManager::signalUniversalSearchClosed()
{
    QMetaObject::activate(this, &staticMetaObject, 15, 0);
}
QT_END_MOC_NAMESPACE
