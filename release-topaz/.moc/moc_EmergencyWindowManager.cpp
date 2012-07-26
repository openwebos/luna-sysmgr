/****************************************************************************
** Meta object code from reading C++ file 'EmergencyWindowManager.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/emergency/EmergencyWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EmergencyWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_EmergencyWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   24,   23,   23, 0x08,
      58,   23,   23,   23, 0x08,
      82,   23,   23,   23, 0x08,
     106,   23,   23,   23, 0x08,
     137,  130,   23,   23, 0x08,
     178,   23,   23,   23, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_EmergencyWindowManager[] = {
    "EmergencyWindowManager\0\0r\0"
    "slotPositiveSpaceChanged(QRect)\0"
    "slotIncomingPhoneCall()\0fadeAnimationFinished()\0"
    "slotHomeButtonPressed()\0enable\0"
    "slotEmergencyModeWindowFocusChange(bool)\0"
    "slotUiRotationCompleted()\0"
};

void EmergencyWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        EmergencyWindowManager *_t = static_cast<EmergencyWindowManager *>(_o);
        switch (_id) {
        case 0: _t->slotPositiveSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 1: _t->slotIncomingPhoneCall(); break;
        case 2: _t->fadeAnimationFinished(); break;
        case 3: _t->slotHomeButtonPressed(); break;
        case 4: _t->slotEmergencyModeWindowFocusChange((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->slotUiRotationCompleted(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData EmergencyWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject EmergencyWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_EmergencyWindowManager,
      qt_meta_data_EmergencyWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &EmergencyWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *EmergencyWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *EmergencyWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_EmergencyWindowManager))
        return static_cast<void*>(const_cast< EmergencyWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int EmergencyWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
