/****************************************************************************
** Meta object code from reading C++ file 'StatusBarTitle.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarTitle.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarTitle.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarTitle[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      16,   15,   15,   15, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarTitle[] = {
    "StatusBarTitle\0\0slotAnimFinished()\0"
};

void StatusBarTitle::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarTitle *_t = static_cast<StatusBarTitle *>(_o);
        switch (_id) {
        case 0: _t->slotAnimFinished(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData StatusBarTitle::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarTitle::staticMetaObject = {
    { &StatusBarItem::staticMetaObject, qt_meta_stringdata_StatusBarTitle,
      qt_meta_data_StatusBarTitle, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarTitle::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarTitle::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarTitle::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarTitle))
        return static_cast<void*>(const_cast< StatusBarTitle*>(this));
    return StatusBarItem::qt_metacast(_clname);
}

int StatusBarTitle::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
