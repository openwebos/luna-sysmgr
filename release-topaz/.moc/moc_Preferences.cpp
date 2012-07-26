/****************************************************************************
** Meta object code from reading C++ file 'Preferences.h'
**
** Created: Mon Jul 23 16:14:41 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/base/settings/Preferences.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Preferences.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Preferences[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   13,   12,   12, 0x05,
      58,   13,   12,   12, 0x05,
     110,  102,   12,   12, 0x05,
     149,  141,   12,   12, 0x05,
     181,   12,   12,   12, 0x05,
     213,   12,   12,   12, 0x05,
     244,  237,   12,   12, 0x05,
     283,  281,   12,   12, 0x05,
     332,  319,   12,   12, 0x05,
     396,  389,   12,   12, 0x05,
     432,  425,   12,   12, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_Preferences[] = {
    "Preferences\0\0filePath\0"
    "signalWallPaperChanged(const char*)\0"
    "signalDockModeWallPaperChanged(const char*)\0"
    "timeout\0signalSetLockTimeout(uint32_t)\0"
    "enabled\0signalAirplaneModeChanged(bool)\0"
    "signalRoamingIndicatorChanged()\0"
    "signalDualRssiEnabled()\0format\0"
    "signalTimeFormatChanged(const char*)\0"
    "v\0signalVoiceDialSettingChanged(bool)\0"
    "rotationLock\0"
    "signalRotationLockChanged(OrientationEvent::Orientation)\0"
    "muteOn\0signalMuteSoundChanged(bool)\0"
    "enable\0signalAlsEnabled(bool)\0"
};

void Preferences::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Preferences *_t = static_cast<Preferences *>(_o);
        switch (_id) {
        case 0: _t->signalWallPaperChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 1: _t->signalDockModeWallPaperChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 2: _t->signalSetLockTimeout((*reinterpret_cast< uint32_t(*)>(_a[1]))); break;
        case 3: _t->signalAirplaneModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->signalRoamingIndicatorChanged(); break;
        case 5: _t->signalDualRssiEnabled(); break;
        case 6: _t->signalTimeFormatChanged((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 7: _t->signalVoiceDialSettingChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->signalRotationLockChanged((*reinterpret_cast< OrientationEvent::Orientation(*)>(_a[1]))); break;
        case 9: _t->signalMuteSoundChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->signalAlsEnabled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Preferences::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Preferences::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Preferences,
      qt_meta_data_Preferences, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Preferences::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Preferences::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Preferences::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Preferences))
        return static_cast<void*>(const_cast< Preferences*>(this));
    return QObject::qt_metacast(_clname);
}

int Preferences::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void Preferences::signalWallPaperChanged(const char * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Preferences::signalDockModeWallPaperChanged(const char * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Preferences::signalSetLockTimeout(uint32_t _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Preferences::signalAirplaneModeChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Preferences::signalRoamingIndicatorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void Preferences::signalDualRssiEnabled()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void Preferences::signalTimeFormatChanged(const char * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Preferences::signalVoiceDialSettingChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Preferences::signalRotationLockChanged(OrientationEvent::Orientation _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Preferences::signalMuteSoundChanged(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void Preferences::signalAlsEnabled(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}
QT_END_MOC_NAMESPACE
