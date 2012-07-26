/****************************************************************************
** Meta object code from reading C++ file 'colorroundrectbutton.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/buttons/colorroundrectbutton.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'colorroundrectbutton.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ColorRoundRectButton[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       1,   54, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x05,
      43,   21,   21,   21, 0x05,
      59,   21,   21,   21, 0x05,
      75,   21,   21,   21, 0x05,
      95,   21,   21,   21, 0x05,
     113,   21,   21,   21, 0x05,
     140,   21,   21,   21, 0x05,
     160,   21,   21,   21, 0x05,

 // properties: name, type, flags
     187,  182, 0x01495103,

 // properties: notify_signal_id
       5,

       0        // eod
};

static const char qt_meta_stringdata_ColorRoundRectButton[] = {
    "ColorRoundRectButton\0\0signalFirstContact()\0"
    "signalContact()\0signalRelease()\0"
    "signalLastRelease()\0signalActivated()\0"
    "signalStateActiveChanged()\0"
    "signalFSMActivate()\0signalFSMDeactivate()\0"
    "bool\0stateActive\0"
};

void ColorRoundRectButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ColorRoundRectButton *_t = static_cast<ColorRoundRectButton *>(_o);
        switch (_id) {
        case 0: _t->signalFirstContact(); break;
        case 1: _t->signalContact(); break;
        case 2: _t->signalRelease(); break;
        case 3: _t->signalLastRelease(); break;
        case 4: _t->signalActivated(); break;
        case 5: _t->signalStateActiveChanged(); break;
        case 6: _t->signalFSMActivate(); break;
        case 7: _t->signalFSMDeactivate(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ColorRoundRectButton::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ColorRoundRectButton::staticMetaObject = {
    { &LabeledButton::staticMetaObject, qt_meta_stringdata_ColorRoundRectButton,
      qt_meta_data_ColorRoundRectButton, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ColorRoundRectButton::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ColorRoundRectButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ColorRoundRectButton::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ColorRoundRectButton))
        return static_cast<void*>(const_cast< ColorRoundRectButton*>(this));
    return LabeledButton::qt_metacast(_clname);
}

int ColorRoundRectButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LabeledButton::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = stateActive(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setStateActive(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ColorRoundRectButton::signalFirstContact()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void ColorRoundRectButton::signalContact()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void ColorRoundRectButton::signalRelease()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void ColorRoundRectButton::signalLastRelease()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void ColorRoundRectButton::signalActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void ColorRoundRectButton::signalStateActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void ColorRoundRectButton::signalFSMActivate()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void ColorRoundRectButton::signalFSMDeactivate()
{
    QMetaObject::activate(this, &staticMetaObject, 7, 0);
}
QT_END_MOC_NAMESPACE
