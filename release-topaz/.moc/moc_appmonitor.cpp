/****************************************************************************
** Meta object code from reading C++ file 'appmonitor.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/systeminterface/appmonitor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'appmonitor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DimensionsSystemInterface__AppMonitor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      24,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: signature, parameters, type, tag, flags
      51,   39,   38,   38, 0x05,
      81,   38,   38,   38, 0x25,
     119,  107,   38,   38, 0x05,
     231,  226,   38,   38, 0x25,
     320,  284,   38,   38, 0x05,
     453,  424,   38,   38, 0x25,
     503,  107,   38,   38, 0x05,
     614,  226,   38,   38, 0x25,
     692,  671,   38,   38, 0x05,
     792,  778,   38,   38, 0x25,
     852,  824,   38,   38, 0x05,
     971,  950,   38,   38, 0x25,
    1015,  107,   38,   38, 0x05,
    1126,  226,   38,   38, 0x25,

 // slots: signature, parameters, type, tag, flags
    1183,   38,   38,   38, 0x0a,
    1206,   38,   38,   38, 0x0a,
    1242, 1227,   38,   38, 0x0a,
    1314, 1290,   38,   38, 0x0a,
    1398, 1227,   38,   38, 0x0a,
    1460, 1449,   38,   38, 0x0a,
    1491, 1227,   38,   38, 0x0a,
    1546, 1537,   38,   38, 0x0a,
    1597, 1537,   38,   38, 0x0a,
    1650, 1537,   38,   38, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DimensionsSystemInterface__AppMonitor[] = {
    "DimensionsSystemInterface::AppMonitor\0"
    "\0initialScan\0signalFullScanCompleted(bool)\0"
    "signalFullScanCompleted()\0eapp,origin\0"
    "signalNewApp(DimensionsSystemInterface::ExternalApp,DimensionsSystemIn"
    "terface::AppMonitorSignalType::Enum)\0"
    "eapp\0signalNewApp(DimensionsSystemInterface::ExternalApp)\0"
    "appUid,newLaunchPointIconUid,origin\0"
    "signalNewAdditionalLaunchPointForApp(QUuid,QUuid,DimensionsSystemInter"
    "face::AppMonitorSignalType::Enum)\0"
    "appUid,newLaunchPointIconUid\0"
    "signalNewAdditionalLaunchPointForApp(QUuid,QUuid)\0"
    "signalRemovedApp(DimensionsSystemInterface::ExternalApp,DimensionsSyst"
    "emInterface::AppMonitorSignalType::Enum)\0"
    "signalRemovedApp(DimensionsSystemInterface::ExternalApp)\0"
    "removedAppUid,origin\0"
    "signalRemovedAppComplete(QUuid,DimensionsSystemInterface::AppMonitorSi"
    "gnalType::Enum)\0"
    "removedAppUid\0signalRemovedAppComplete(QUuid)\0"
    "appUid,launchpointId,origin\0"
    "signalAppAuxiliaryIconRemove(QUuid,QString,DimensionsSystemInterface::"
    "AppMonitorSignalType::Enum)\0"
    "appUid,launchpointId\0"
    "signalAppAuxiliaryIconRemove(QUuid,QString)\0"
    "signalUpdatedApp(DimensionsSystemInterface::ExternalApp,DimensionsSyst"
    "emInterface::AppMonitorSignalType::Enum)\0"
    "signalUpdatedApp(DimensionsSystemInterface::ExternalApp)\0"
    "slotInitialScanStart()\0slotInitialScanEnd()\0"
    "pAppdescriptor\0"
    "slotScanFoundApp(const ApplicationDescription*)\0"
    "p_appDesc,p_launchPoint\0"
    "slotScanFoundAuxiliaryLaunchPoint(const ApplicationDescription*,const "
    "LaunchPoint*)\0"
    "slotAppBeingRemoved(const ApplicationDescription*)\0"
    "p_webOSapp\0slotAppBeingRemoved(WebOSApp*)\0"
    "slotAppUpdated(const ApplicationDescription*)\0"
    ",reasons\0slotLaunchPointAdded(const LaunchPoint*,QBitArray)\0"
    "slotLaunchPointUpdated(const LaunchPoint*,QBitArray)\0"
    "slotLaunchPointRemoved(const LaunchPoint*,QBitArray)\0"
};

void DimensionsSystemInterface::AppMonitor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AppMonitor *_t = static_cast<AppMonitor *>(_o);
        switch (_id) {
        case 0: _t->signalFullScanCompleted((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->signalFullScanCompleted(); break;
        case 2: _t->signalNewApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 3: _t->signalNewApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1]))); break;
        case 4: _t->signalNewAdditionalLaunchPointForApp((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QUuid(*)>(_a[2])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[3]))); break;
        case 5: _t->signalNewAdditionalLaunchPointForApp((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QUuid(*)>(_a[2]))); break;
        case 6: _t->signalRemovedApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 7: _t->signalRemovedApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1]))); break;
        case 8: _t->signalRemovedAppComplete((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 9: _t->signalRemovedAppComplete((*reinterpret_cast< const QUuid(*)>(_a[1]))); break;
        case 10: _t->signalAppAuxiliaryIconRemove((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[3]))); break;
        case 11: _t->signalAppAuxiliaryIconRemove((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 12: _t->signalUpdatedApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1])),(*reinterpret_cast< DimensionsSystemInterface::AppMonitorSignalType::Enum(*)>(_a[2]))); break;
        case 13: _t->signalUpdatedApp((*reinterpret_cast< const DimensionsSystemInterface::ExternalApp(*)>(_a[1]))); break;
        case 14: _t->slotInitialScanStart(); break;
        case 15: _t->slotInitialScanEnd(); break;
        case 16: _t->slotScanFoundApp((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1]))); break;
        case 17: _t->slotScanFoundAuxiliaryLaunchPoint((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1])),(*reinterpret_cast< const LaunchPoint*(*)>(_a[2]))); break;
        case 18: _t->slotAppBeingRemoved((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1]))); break;
        case 19: _t->slotAppBeingRemoved((*reinterpret_cast< WebOSApp*(*)>(_a[1]))); break;
        case 20: _t->slotAppUpdated((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1]))); break;
        case 21: _t->slotLaunchPointAdded((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        case 22: _t->slotLaunchPointUpdated((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        case 23: _t->slotLaunchPointRemoved((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DimensionsSystemInterface::AppMonitor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DimensionsSystemInterface::AppMonitor::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DimensionsSystemInterface__AppMonitor,
      qt_meta_data_DimensionsSystemInterface__AppMonitor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DimensionsSystemInterface::AppMonitor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DimensionsSystemInterface::AppMonitor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DimensionsSystemInterface::AppMonitor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DimensionsSystemInterface__AppMonitor))
        return static_cast<void*>(const_cast< AppMonitor*>(this));
    return QObject::qt_metacast(_clname);
}

int DimensionsSystemInterface::AppMonitor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    }
    return _id;
}

// SIGNAL 0
void DimensionsSystemInterface::AppMonitor::signalFullScanCompleted(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void DimensionsSystemInterface::AppMonitor::signalNewApp(const DimensionsSystemInterface::ExternalApp & _t1, DimensionsSystemInterface::AppMonitorSignalType::Enum _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void DimensionsSystemInterface::AppMonitor::signalNewAdditionalLaunchPointForApp(const QUuid & _t1, const QUuid & _t2, DimensionsSystemInterface::AppMonitorSignalType::Enum _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void DimensionsSystemInterface::AppMonitor::signalRemovedApp(const DimensionsSystemInterface::ExternalApp & _t1, DimensionsSystemInterface::AppMonitorSignalType::Enum _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 8
void DimensionsSystemInterface::AppMonitor::signalRemovedAppComplete(const QUuid & _t1, DimensionsSystemInterface::AppMonitorSignalType::Enum _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 10
void DimensionsSystemInterface::AppMonitor::signalAppAuxiliaryIconRemove(const QUuid & _t1, const QString & _t2, DimensionsSystemInterface::AppMonitorSignalType::Enum _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 12
void DimensionsSystemInterface::AppMonitor::signalUpdatedApp(const DimensionsSystemInterface::ExternalApp & _t1, DimensionsSystemInterface::AppMonitorSignalType::Enum _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_END_MOC_NAMESPACE
