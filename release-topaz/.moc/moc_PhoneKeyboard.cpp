/****************************************************************************
** Meta object code from reading C++ file 'PhoneKeyboard.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/ime/PhoneKeyboard.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PhoneKeyboard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Phone_Keyboard__PhoneKeyboard[] = {

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
      36,   31,   30,   30, 0x0a,
      73,   65,   30,   30, 0x0a,
     100,   94,   30,   30, 0x0a,
     149,  141,   30,   30, 0x0a,
     170,   30,   30,   30, 0x0a,
     187,   30,   30,   30, 0x0a,
     209,   30,   30,   30, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_Phone_Keyboard__PhoneKeyboard[] = {
    "Phone_Keyboard::PhoneKeyboard\0\0size\0"
    "availableSpaceChanged(QRect)\0visible\0"
    "visibleChanged(bool)\0state\0"
    "editorStateChanged(PalmIME::EditorState)\0"
    "autoCap\0autoCapChanged(bool)\0"
    "triggerRepaint()\0candidateBarResized()\0"
    "repeatChar()\0"
};

void Phone_Keyboard::PhoneKeyboard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PhoneKeyboard *_t = static_cast<PhoneKeyboard *>(_o);
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

const QMetaObjectExtraData Phone_Keyboard::PhoneKeyboard::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Phone_Keyboard::PhoneKeyboard::staticMetaObject = {
    { &VirtualKeyboard::staticMetaObject, qt_meta_stringdata_Phone_Keyboard__PhoneKeyboard,
      qt_meta_data_Phone_Keyboard__PhoneKeyboard, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Phone_Keyboard::PhoneKeyboard::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Phone_Keyboard::PhoneKeyboard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Phone_Keyboard::PhoneKeyboard::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Phone_Keyboard__PhoneKeyboard))
        return static_cast<void*>(const_cast< PhoneKeyboard*>(this));
    return VirtualKeyboard::qt_metacast(_clname);
}

int Phone_Keyboard::PhoneKeyboard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
