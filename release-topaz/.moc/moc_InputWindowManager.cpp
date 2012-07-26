/****************************************************************************
** Meta object code from reading C++ file 'InputWindowManager.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/ime/InputWindowManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'InputWindowManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_InputWindowManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      22,   20,   19,   19, 0x08,
      64,   54,   19,   19, 0x08,
      98,   19,   19,   19, 0x08,
     112,   19,   19,   19, 0x08,
     132,  126,   19,   19, 0x08,
     179,  171,   19,   19, 0x08,
     204,   19,   19,   19, 0x08,
     229,   19,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_InputWindowManager[] = {
    "InputWindowManager\0\0r\0"
    "slotNegativeSpaceChanged(QRect)\0"
    "newHeight\0slotKeyboardHeightChanged(qint32)\0"
    "slotHideIME()\0slotShowIME()\0state\0"
    "slotRestartInput(PalmIME::EditorState)\0"
    "enabled\0slotAutoCapChanged(bool)\0"
    "slotEnterBrickMode(bool)\0slotExitBrickMode()\0"
};

void InputWindowManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        InputWindowManager *_t = static_cast<InputWindowManager *>(_o);
        switch (_id) {
        case 0: _t->slotNegativeSpaceChanged((*reinterpret_cast< QRect(*)>(_a[1]))); break;
        case 1: _t->slotKeyboardHeightChanged((*reinterpret_cast< const qint32(*)>(_a[1]))); break;
        case 2: _t->slotHideIME(); break;
        case 3: _t->slotShowIME(); break;
        case 4: _t->slotRestartInput((*reinterpret_cast< const PalmIME::EditorState(*)>(_a[1]))); break;
        case 5: _t->slotAutoCapChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->slotEnterBrickMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->slotExitBrickMode(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData InputWindowManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject InputWindowManager::staticMetaObject = {
    { &WindowManagerBase::staticMetaObject, qt_meta_stringdata_InputWindowManager,
      qt_meta_data_InputWindowManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &InputWindowManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *InputWindowManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *InputWindowManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_InputWindowManager))
        return static_cast<void*>(const_cast< InputWindowManager*>(this));
    return WindowManagerBase::qt_metacast(_clname);
}

int InputWindowManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WindowManagerBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
