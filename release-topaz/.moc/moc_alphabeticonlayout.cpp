/****************************************************************************
** Meta object code from reading C++ file 'alphabeticonlayout.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/icon_layouts/alphabeticonlayout.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'alphabeticonlayout.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_AlphabetIconLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x05,
      43,   19,   19,   19, 0x05,
      64,   19,   19,   19, 0x05,
      96,   19,   19,   19, 0x05,
     126,   19,   19,   19, 0x05,
     159,   19,   19,   19, 0x05,
     193,   19,   19,   19, 0x05,

 // slots: signature, parameters, type, tag, flags
     225,   19,   19,   19, 0x09,
     260,   19,   19,   19, 0x09,
     284,   19,   19,   19, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_AlphabetIconLayout[] = {
    "AlphabetIconLayout\0\0signalReorderStarted()\0"
    "signalReorderEnded()\0"
    "signalFSMTrackStarted_Trigger()\0"
    "signalFSMTrackEnded_Trigger()\0"
    "signalFSMLastTrackEndedTrigger()\0"
    "signalFSMReorderStarted_Trigger()\0"
    "signalFSMReorderEnded_Trigger()\0"
    "slotTrackedIconAnimationFinished()\0"
    "slotTrackForIconEnded()\0"
    "dbg_reorderFSMStateEntered()\0"
};

void AlphabetIconLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AlphabetIconLayout *_t = static_cast<AlphabetIconLayout *>(_o);
        switch (_id) {
        case 0: _t->signalReorderStarted(); break;
        case 1: _t->signalReorderEnded(); break;
        case 2: _t->signalFSMTrackStarted_Trigger(); break;
        case 3: _t->signalFSMTrackEnded_Trigger(); break;
        case 4: _t->signalFSMLastTrackEndedTrigger(); break;
        case 5: _t->signalFSMReorderStarted_Trigger(); break;
        case 6: _t->signalFSMReorderEnded_Trigger(); break;
        case 7: _t->slotTrackedIconAnimationFinished(); break;
        case 8: _t->slotTrackForIconEnded(); break;
        case 9: _t->dbg_reorderFSMStateEntered(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData AlphabetIconLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject AlphabetIconLayout::staticMetaObject = {
    { &IconLayout::staticMetaObject, qt_meta_stringdata_AlphabetIconLayout,
      qt_meta_data_AlphabetIconLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &AlphabetIconLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *AlphabetIconLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *AlphabetIconLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AlphabetIconLayout))
        return static_cast<void*>(const_cast< AlphabetIconLayout*>(this));
    return IconLayout::qt_metacast(_clname);
}

int AlphabetIconLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IconLayout::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void AlphabetIconLayout::signalReorderStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void AlphabetIconLayout::signalReorderEnded()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void AlphabetIconLayout::signalFSMTrackStarted_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void AlphabetIconLayout::signalFSMTrackEnded_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void AlphabetIconLayout::signalFSMLastTrackEndedTrigger()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void AlphabetIconLayout::signalFSMReorderStarted_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void AlphabetIconLayout::signalFSMReorderEnded_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}
QT_END_MOC_NAMESPACE
