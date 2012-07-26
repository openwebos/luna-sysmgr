/****************************************************************************
** Meta object code from reading C++ file 'TabletKeyboard.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/ime/TabletKeyboard.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TabletKeyboard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Tablet_Keyboard__TabletKeyboard[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      38,   33,   32,   32, 0x0a,
      75,   67,   32,   32, 0x0a,
     102,   96,   32,   32, 0x0a,
     151,  143,   32,   32, 0x0a,
     172,   32,   32,   32, 0x0a,
     189,   32,   32,   32, 0x0a,
     211,   32,   32,   32, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_Tablet_Keyboard__TabletKeyboard[] = {
    "Tablet_Keyboard::TabletKeyboard\0\0size\0"
    "availableSpaceChanged(QRect)\0visible\0"
    "visibleChanged(bool)\0state\0"
    "editorStateChanged(PalmIME::EditorState)\0"
    "autoCap\0autoCapChanged(bool)\0"
    "triggerRepaint()\0candidateBarResized()\0"
    "repeatChar()\0"
};

void Tablet_Keyboard::TabletKeyboard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TabletKeyboard *_t = static_cast<TabletKeyboard *>(_o);
        switch (_id) {
        case 0: _t->availableSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 1: _t->visibleChanged((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 2: _t->editorStateChanged((*reinterpret_cast< const PalmIME::EditorState(*)>(_a[1]))); break;
        case 3: _t->autoCapChanged((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 4: _t->triggerRepaint(); break;
        case 5: _t->candidateBarResized(); break;
        case 6: _t->repeatChar(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Tablet_Keyboard::TabletKeyboard::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Tablet_Keyboard::TabletKeyboard::staticMetaObject = {
    { &VirtualKeyboard::staticMetaObject, qt_meta_stringdata_Tablet_Keyboard__TabletKeyboard,
      qt_meta_data_Tablet_Keyboard__TabletKeyboard, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Tablet_Keyboard::TabletKeyboard::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Tablet_Keyboard::TabletKeyboard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Tablet_Keyboard::TabletKeyboard::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Tablet_Keyboard__TabletKeyboard))
        return static_cast<void*>(const_cast< TabletKeyboard*>(this));
    return VirtualKeyboard::qt_metacast(_clname);
}

int Tablet_Keyboard::TabletKeyboard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = VirtualKeyboard::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
