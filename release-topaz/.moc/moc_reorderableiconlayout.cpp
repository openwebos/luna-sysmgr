/****************************************************************************
** Meta object code from reading C++ file 'reorderableiconlayout.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/icon_layouts/reorderableiconlayout.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'reorderableiconlayout.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ReorderableIconLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   22,   22,   22, 0x05,
      46,   22,   22,   22, 0x05,
      67,   22,   22,   22, 0x05,
      99,   22,   22,   22, 0x05,
     129,   22,   22,   22, 0x05,
     162,   22,   22,   22, 0x05,
     196,   22,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
     228,   22,   22,   22, 0x09,
     260,   22,   22,   22, 0x09,
     306,   22,   22,   22, 0x09,
     330,   22,   22,   22, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_ReorderableIconLayout[] = {
    "ReorderableIconLayout\0\0signalReorderStarted()\0"
    "signalReorderEnded()\0"
    "signalFSMTrackStarted_Trigger()\0"
    "signalFSMTrackEnded_Trigger()\0"
    "signalFSMLastTrackEndedTrigger()\0"
    "signalFSMReorderStarted_Trigger()\0"
    "signalFSMReorderEnded_Trigger()\0"
    "slotReorderAnimationsFinished()\0"
    "slotTrackedIconReplacementAnimationFinished()\0"
    "slotTrackForIconEnded()\0"
    "dbg_reorderFSMStateEntered()\0"
};

void ReorderableIconLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ReorderableIconLayout *_t = static_cast<ReorderableIconLayout *>(_o);
        switch (_id) {
        case 0: _t->signalReorderStarted(); break;
        case 1: _t->signalReorderEnded(); break;
        case 2: _t->signalFSMTrackStarted_Trigger(); break;
        case 3: _t->signalFSMTrackEnded_Trigger(); break;
        case 4: _t->signalFSMLastTrackEndedTrigger(); break;
        case 5: _t->signalFSMReorderStarted_Trigger(); break;
        case 6: _t->signalFSMReorderEnded_Trigger(); break;
        case 7: _t->slotReorderAnimationsFinished(); break;
        case 8: _t->slotTrackedIconReplacementAnimationFinished(); break;
        case 9: _t->slotTrackForIconEnded(); break;
        case 10: _t->dbg_reorderFSMStateEntered(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ReorderableIconLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ReorderableIconLayout::staticMetaObject = {
    { &IconLayout::staticMetaObject, qt_meta_stringdata_ReorderableIconLayout,
      qt_meta_data_ReorderableIconLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ReorderableIconLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ReorderableIconLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ReorderableIconLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ReorderableIconLayout))
        return static_cast<void*>(const_cast< ReorderableIconLayout*>(this));
    return IconLayout::qt_metacast(_clname);
}

int ReorderableIconLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IconLayout::qt_metacall(_c, _id, _a);
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
void ReorderableIconLayout::signalReorderStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void ReorderableIconLayout::signalReorderEnded()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void ReorderableIconLayout::signalFSMTrackStarted_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void ReorderableIconLayout::signalFSMTrackEnded_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void ReorderableIconLayout::signalFSMLastTrackEndedTrigger()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void ReorderableIconLayout::signalFSMReorderStarted_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void ReorderableIconLayout::signalFSMReorderEnded_Trigger()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}
QT_END_MOC_NAMESPACE
