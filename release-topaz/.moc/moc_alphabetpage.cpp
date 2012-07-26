/****************************************************************************
** Meta object code from reading C++ file 'alphabetpage.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/page/alphabetpage.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'alphabetpage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_AlphabetPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       1,   44, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x0a,
      40,   36,   13,   13, 0x09,
      74,   13,   13,   13, 0x09,
     103,   13,   13,   13, 0x09,
     130,   13,   13,   13, 0x09,
     164,   13,   13,   13, 0x09,

 // constructors: signature, parameters, type, tag, flags
     221,  196,   13,   13, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_AlphabetPage[] = {
    "AlphabetPage\0\0dbg_slotPrintLayout()\0"
    "uid\0slotTrackedIconCancelTrack(QUuid)\0"
    "slotReorderInLayoutStarted()\0"
    "slotReorderInLayoutEnded()\0"
    "slotLauncherCmdStartReorderMode()\0"
    "slotLauncherCmdEndReorderMode()\0"
    "pageGeometry,p_belongsTo\0"
    "AlphabetPage(QRectF,LauncherObject*)\0"
};

void AlphabetPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { AlphabetPage *_r = new AlphabetPage((*reinterpret_cast< const QRectF(*)>(_a[1])),(*reinterpret_cast< LauncherObject*(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AlphabetPage *_t = static_cast<AlphabetPage *>(_o);
        switch (_id) {
        case 0: _t->dbg_slotPrintLayout(); break;
        case 1: _t->slotTrackedIconCancelTrack((*reinterpret_cast< const QUuid(*)>(_a[1]))); break;
        case 2: _t->slotReorderInLayoutStarted(); break;
        case 3: _t->slotReorderInLayoutEnded(); break;
        case 4: _t->slotLauncherCmdStartReorderMode(); break;
        case 5: _t->slotLauncherCmdEndReorderMode(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData AlphabetPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject AlphabetPage::staticMetaObject = {
    { &Page::staticMetaObject, qt_meta_stringdata_AlphabetPage,
      qt_meta_data_AlphabetPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &AlphabetPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *AlphabetPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *AlphabetPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AlphabetPage))
        return static_cast<void*>(const_cast< AlphabetPage*>(this));
    return Page::qt_metacast(_clname);
}

int AlphabetPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Page::qt_metacall(_c, _id, _a);
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
