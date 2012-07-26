/****************************************************************************
** Meta object code from reading C++ file 'reorderablepage.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/reorderablepage.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'reorderablepage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ReorderablePage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       1,   39, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      21,   17,   16,   16, 0x09,
      55,   16,   16,   16, 0x09,
      84,   16,   16,   16, 0x09,
     111,   16,   16,   16, 0x09,
     145,   16,   16,   16, 0x09,

 // constructors: signature, parameters, type, tag, flags
     202,  177,   16,   16, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_ReorderablePage[] = {
    "ReorderablePage\0\0uid\0"
    "slotTrackedIconCancelTrack(QUuid)\0"
    "slotReorderInLayoutStarted()\0"
    "slotReorderInLayoutEnded()\0"
    "slotLauncherCmdStartReorderMode()\0"
    "slotLauncherCmdEndReorderMode()\0"
    "pageGeometry,p_belongsTo\0"
    "ReorderablePage(QRectF,LauncherObject*)\0"
};

void ReorderablePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { ReorderablePage *_r = new ReorderablePage((*reinterpret_cast< const QRectF(*)>(_a[1])),(*reinterpret_cast< LauncherObject*(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ReorderablePage *_t = static_cast<ReorderablePage *>(_o);
        switch (_id) {
        case 0: _t->slotTrackedIconCancelTrack((*reinterpret_cast< const QUuid(*)>(_a[1]))); break;
        case 1: _t->slotReorderInLayoutStarted(); break;
        case 2: _t->slotReorderInLayoutEnded(); break;
        case 3: _t->slotLauncherCmdStartReorderMode(); break;
        case 4: _t->slotLauncherCmdEndReorderMode(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ReorderablePage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ReorderablePage::staticMetaObject = {
    { &Page::staticMetaObject, qt_meta_stringdata_ReorderablePage,
      qt_meta_data_ReorderablePage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ReorderablePage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ReorderablePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ReorderablePage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ReorderablePage))
        return static_cast<void*>(const_cast< ReorderablePage*>(this));
    return Page::qt_metacast(_clname);
}

int ReorderablePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Page::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
