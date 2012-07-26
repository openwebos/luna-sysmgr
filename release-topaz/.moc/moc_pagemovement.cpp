/****************************************************************************
** Meta object code from reading C++ file 'pagemovement.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/pagemovement.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pagemovement.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PageMovementControl[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   20,   20,   20, 0x05,
      59,   20,   20,   20, 0x05,
      95,   20,   20,   20, 0x05,
     134,   20,   20,   20, 0x05,
     171,   20,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
     201,   20,   20,   20, 0x09,
     223,   20,   20,   20, 0x09,
     246,   20,   20,   20, 0x09,
     270,   20,   20,   20, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_PageMovementControl[] = {
    "PageMovementControl\0\0"
    "signalPageMovementFSM_MovedPageLeft()\0"
    "signalPageMovementFSM_LeftTimeout()\0"
    "signalPageMovementFSM_MovedPageRight()\0"
    "signalPageMovementFSM_RightTimeout()\0"
    "signalPageMovementFSM_Reset()\0"
    "slotStopResetTimers()\0slotRestartLeftTimer()\0"
    "slotRestartRightTimer()\0dbgSlotPrint()\0"
};

void PageMovementControl::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PageMovementControl *_t = static_cast<PageMovementControl *>(_o);
        switch (_id) {
        case 0: _t->signalPageMovementFSM_MovedPageLeft(); break;
        case 1: _t->signalPageMovementFSM_LeftTimeout(); break;
        case 2: _t->signalPageMovementFSM_MovedPageRight(); break;
        case 3: _t->signalPageMovementFSM_RightTimeout(); break;
        case 4: _t->signalPageMovementFSM_Reset(); break;
        case 5: _t->slotStopResetTimers(); break;
        case 6: _t->slotRestartLeftTimer(); break;
        case 7: _t->slotRestartRightTimer(); break;
        case 8: _t->dbgSlotPrint(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PageMovementControl::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PageMovementControl::staticMetaObject = {
    { &QStateMachine::staticMetaObject, qt_meta_stringdata_PageMovementControl,
      qt_meta_data_PageMovementControl, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PageMovementControl::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PageMovementControl::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PageMovementControl::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PageMovementControl))
        return static_cast<void*>(const_cast< PageMovementControl*>(this));
    return QStateMachine::qt_metacast(_clname);
}

int PageMovementControl::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStateMachine::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void PageMovementControl::signalPageMovementFSM_MovedPageLeft()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void PageMovementControl::signalPageMovementFSM_LeftTimeout()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void PageMovementControl::signalPageMovementFSM_MovedPageRight()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void PageMovementControl::signalPageMovementFSM_RightTimeout()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void PageMovementControl::signalPageMovementFSM_Reset()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
