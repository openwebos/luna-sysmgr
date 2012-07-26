/****************************************************************************
** Meta object code from reading C++ file 'ApplicationManager.h'
**
** Created: Mon Jul 23 16:14:41 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/application/ApplicationManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ApplicationManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ApplicationManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: signature, parameters, type, tag, flags
      29,   20,   19,   19, 0x05,
      84,   19,   19,   19, 0x25,
     129,   20,   19,   19, 0x05,
     184,   19,   19,   19, 0x25,
     229,   20,   19,   19, 0x05,
     282,   19,   19,   19, 0x25,
     325,   19,   19,   19, 0x05,
     350,   19,   19,   19, 0x05,
     373,   19,   19,   19, 0x05,
     425,  423,   19,   19, 0x05,
     511,   19,   19,   19, 0x05,
     564,   19,   19,   19, 0x05,

 // slots: signature, parameters, type, tag, flags
     642,  618,   19,   19, 0x0a,
     689,  618,   19,   19, 0x0a,
     739,  737,   19,   19, 0x0a,
     778,  618,   19,   19, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ApplicationManager[] = {
    "ApplicationManager\0\0,reasons\0"
    "signalLaunchPointRemoved(const LaunchPoint*,QBitArray)\0"
    "signalLaunchPointRemoved(const LaunchPoint*)\0"
    "signalLaunchPointUpdated(const LaunchPoint*,QBitArray)\0"
    "signalLaunchPointUpdated(const LaunchPoint*)\0"
    "signalLaunchPointAdded(const LaunchPoint*,QBitArray)\0"
    "signalLaunchPointAdded(const LaunchPoint*)\0"
    "signalInitialScanStart()\0"
    "signalInitialScanEnd()\0"
    "signalScanFoundApp(const ApplicationDescription*)\0"
    ",\0"
    "signalScanFoundAuxiliaryLaunchPoint(const ApplicationDescription*,cons"
    "t LaunchPoint*)\0"
    "signalDockModeLaunchPointEnabled(const LaunchPoint*)\0"
    "signalDockModeLaunchPointDisabled(const LaunchPoint*)\0"
    "argsAsStringEncodedJson\0"
    "slotBuiltInAppEntryPoint_DockMode(std::string)\0"
    "slotBuiltInAppEntryPoint_VoiceDial(std::string)\0"
    "v\0slotVoiceDialAllowSettingChanged(bool)\0"
    "slotBuiltInAppEntryPoint_Launchermode0(std::string)\0"
};

void ApplicationManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ApplicationManager *_t = static_cast<ApplicationManager *>(_o);
        switch (_id) {
        case 0: _t->signalLaunchPointRemoved((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        case 1: _t->signalLaunchPointRemoved((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 2: _t->signalLaunchPointUpdated((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        case 3: _t->signalLaunchPointUpdated((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 4: _t->signalLaunchPointAdded((*reinterpret_cast< const LaunchPoint*(*)>(_a[1])),(*reinterpret_cast< QBitArray(*)>(_a[2]))); break;
        case 5: _t->signalLaunchPointAdded((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 6: _t->signalInitialScanStart(); break;
        case 7: _t->signalInitialScanEnd(); break;
        case 8: _t->signalScanFoundApp((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1]))); break;
        case 9: _t->signalScanFoundAuxiliaryLaunchPoint((*reinterpret_cast< const ApplicationDescription*(*)>(_a[1])),(*reinterpret_cast< const LaunchPoint*(*)>(_a[2]))); break;
        case 10: _t->signalDockModeLaunchPointEnabled((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 11: _t->signalDockModeLaunchPointDisabled((*reinterpret_cast< const LaunchPoint*(*)>(_a[1]))); break;
        case 12: _t->slotBuiltInAppEntryPoint_DockMode((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 13: _t->slotBuiltInAppEntryPoint_VoiceDial((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 14: _t->slotVoiceDialAllowSettingChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->slotBuiltInAppEntryPoint_Launchermode0((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ApplicationManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ApplicationManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ApplicationManager,
      qt_meta_data_ApplicationManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ApplicationManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ApplicationManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ApplicationManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ApplicationManager))
        return static_cast<void*>(const_cast< ApplicationManager*>(this));
    return QObject::qt_metacast(_clname);
}

int ApplicationManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void ApplicationManager::signalLaunchPointRemoved(const LaunchPoint * _t1, QBitArray _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void ApplicationManager::signalLaunchPointUpdated(const LaunchPoint * _t1, QBitArray _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void ApplicationManager::signalLaunchPointAdded(const LaunchPoint * _t1, QBitArray _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void ApplicationManager::signalInitialScanStart()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void ApplicationManager::signalInitialScanEnd()
{
    QMetaObject::activate(this, &staticMetaObject, 7, 0);
}

// SIGNAL 8
void ApplicationManager::signalScanFoundApp(const ApplicationDescription * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ApplicationManager::signalScanFoundAuxiliaryLaunchPoint(const ApplicationDescription * _t1, const LaunchPoint * _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ApplicationManager::signalDockModeLaunchPointEnabled(const LaunchPoint * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void ApplicationManager::signalDockModeLaunchPointDisabled(const LaunchPoint * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_END_MOC_NAMESPACE
