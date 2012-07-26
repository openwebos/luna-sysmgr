/****************************************************************************
** Meta object code from reading C++ file 'quicklaunchbar.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/bars/quicklaunchbar.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'quicklaunchbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QuickLaunchBar[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      17,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   16,   15,   15, 0x05,
      47,   16,   15,   15, 0x05,
      72,   15,   15,   15, 0x05,
     114,   15,   15,   15, 0x05,
     183,  163,   15,   15, 0x05,
     210,   15,   15,   15, 0x25,
     230,   15,   15,   15, 0x05,
     261,  253,   15,   15, 0x05,
     304,   15,   15,   15, 0x05,
     346,   15,   15,   15, 0x05,
     389,   15,   15,   15, 0x05,
     422,   15,   15,   15, 0x05,
     456,   15,   15,   15, 0x05,
     489,   15,   15,   15, 0x05,
     523,   15,   15,   15, 0x05,
     553,   15,   15,   15, 0x05,
     588,  582,   15,   15, 0x05,

 // slots: signature, parameters, type, tag, flags
     634,  622,   15,   15, 0x0a,
     750,  745,   15,   15, 0x2a,
     828,  807,   15,   15, 0x0a,
     921,  907,   15,   15, 0x2a,
     974,  946,   15,   15, 0x0a,
    1091, 1070,   15,   15, 0x2a,
    1135, 1133,   15,   15, 0x09,
    1161,   15,   15,   15, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_QuickLaunchBar[] = {
    "QuickLaunchBar\0\0iconUid\0signalIconAdded(QUuid)\0"
    "signalIconRemoved(QUuid)\0"
    "signalQuickLaunchBarTriggerActivatedTap()\0"
    "signalQuickLaunchBarTriggerActivatedTapAndHold()\0"
    "normVectorDirection\0signalFlickAction(QPointF)\0"
    "signalFlickAction()\0signalToggleLauncher()\0"
    "touchId\0signalTouchFSMTouchBegin_Trigger(QVariant)\0"
    "signalTouchFSMTouchPausedMotion_Trigger()\0"
    "signalTouchFSMTouchResumedMotion_Trigger()\0"
    "signalTouchFSMTouchEnd_Trigger()\0"
    "signalTouchFSMBeginTrack_Action()\0"
    "signalTouchFSMTouchLost_Action()\0"
    "signalTouchFSMStationary_Action()\0"
    "signalTouchFSMMoving_Action()\0"
    "signalTouchEndTrack_Action()\0pIcon\0"
    "signalIconActivatedTap(IconBase*)\0"
    "eapp,origin\0"
    "slotAppPreRemove(DimensionsSystemInterface::ExternalApp,DimensionsSyst"
    "emInterface::AppMonitorSignalType::Enum)\0"
    "eapp\0slotAppPreRemove(DimensionsSystemInterface::ExternalApp)\0"
    "removedAppUid,origin\0"
    "slotAppPostRemove(QUuid,DimensionsSystemInterface::AppMonitorSignalTyp"
    "e::Enum)\0"
    "removedAppUid\0slotAppPostRemove(QUuid)\0"
    "appUid,launchpointId,origin\0"
    "slotAppAuxiliaryIconRemove(QUuid,QString,DimensionsSystemInterface::Ap"
    "pMonitorSignalType::Enum)\0"
    "appUid,launchpointId\0"
    "slotAppAuxiliaryIconRemove(QUuid,QString)\0"
    "p\0slotIconDeleted(QObject*)\0"
    "slotCancelLaunchFeedback()\0"
};

void QuickLaunchBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QuickLaunchBar *_t = static_cast<QuickLaunchBar *>(_o);
        switch (_id) {
        case 0: _t->signalIconAdded((*reinterpret_cast< QUuid(*)>(_a[1]))); break;
        case 1: _t->signalIconRemoved((*reinterpret_cast< QUuid(*)>(_a[1]))); break;
        case 2: _t->signalQuickLaunchBarTriggerActivatedTap(); break;
        case 3: _t->signalQuickLaunchBarTriggerActivatedTapAndHold(); break;
        case 4: _t->signalFlickAction((*reinterpret_cast< QPointF(*)>(_a[1]))); break;
        case 5: _t->signalFlickAction(); break;
        case 6: _t->signalToggleLauncher(); break;
        case 7: _t->signalTouchFSMTouchBegin_Trigger((*reinterpret_cast< QVariant(*)>(_a[1]))); break;
        case 8: _t->signalTouchFSMTouchPausedMotion_Trigger(); break;
        case 9: _t->signalTouchFSMTouchResumedMotion_Trigger(); break;
        case 10: _t->signalTouchFSMTouchEnd_Trigger(); break;
        case 11: _t->signalTouchFSMBeginTrack_Action(); break;
        case 12: _t->signalTouchFSMTouchLost_Action(); break;
        case 13: _t->signalTouchFSMStationary_Action(); break;
        case 14: _t->signalTouchFSMMoving_Action(); break;
        case 15: _t->signalTouchEndTrack_Action(); break;
        case 16: _t->signalIconActivatedTap((*reinterpret_cast< IconBase*(*)>(_a[1]))); break;
        case 17: _t->slotAppPreRemove((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 18: _t->slotAppPreRemove((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1]))); break;
        case 19: _t->slotAppPostRemove((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 20: _t->slotAppPostRemove((*reinterpret_cast< const QUuid(*)>(_a[1]))); break;
        case 21: _t->slotAppAuxiliaryIconRemove((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[3]))); break;
        case 22: _t->slotAppAuxiliaryIconRemove((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 23: _t->slotIconDeleted((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        case 24: _t->slotCancelLaunchFeedback(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QuickLaunchBar::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QuickLaunchBar::staticMetaObject = {
    { &ThingPaintable::staticMetaObject, qt_meta_stringdata_QuickLaunchBar,
      qt_meta_data_QuickLaunchBar, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QuickLaunchBar::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QuickLaunchBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QuickLaunchBar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QuickLaunchBar))
        return static_cast<void*>(const_cast< QuickLaunchBar*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< QuickLaunchBar*>(this));
    return ThingPaintable::qt_metacast(_clname);
}

int QuickLaunchBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ThingPaintable::qt_metacall(_c, _id, _a);
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
void QuickLaunchBar::signalIconAdded(QUuid _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QuickLaunchBar::signalIconRemoved(QUuid _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void QuickLaunchBar::signalQuickLaunchBarTriggerActivatedTap()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void QuickLaunchBar::signalQuickLaunchBarTriggerActivatedTapAndHold()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void QuickLaunchBar::signalFlickAction(QPointF _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void QuickLaunchBar::signalToggleLauncher()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void QuickLaunchBar::signalTouchFSMTouchBegin_Trigger(QVariant _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void QuickLaunchBar::signalTouchFSMTouchPausedMotion_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void QuickLaunchBar::signalTouchFSMTouchResumedMotion_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void QuickLaunchBar::signalTouchFSMTouchEnd_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 10, 0);
}

// SIGNAL 11
void QuickLaunchBar::signalTouchFSMBeginTrack_Action()
{
    QMetaObject::activate(this, &staticMetaObject, 11, 0);
}

// SIGNAL 12
void QuickLaunchBar::signalTouchFSMTouchLost_Action()
{
    QMetaObject::activate(this, &staticMetaObject, 12, 0);
}

// SIGNAL 13
void QuickLaunchBar::signalTouchFSMStationary_Action()
{
    QMetaObject::activate(this, &staticMetaObject, 13, 0);
}

// SIGNAL 14
void QuickLaunchBar::signalTouchFSMMoving_Action()
{
    QMetaObject::activate(this, &staticMetaObject, 14, 0);
}

// SIGNAL 15
void QuickLaunchBar::signalTouchEndTrack_Action()
{
    QMetaObject::activate(this, &staticMetaObject, 15, 0);
}

// SIGNAL 16
void QuickLaunchBar::signalIconActivatedTap(IconBase * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}
QT_END_MOC_NAMESPACE
