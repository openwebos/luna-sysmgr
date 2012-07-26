/****************************************************************************
** Meta object code from reading C++ file 'CardWindow.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/cards/CardWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CardWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CardWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       2,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x09,
      40,   34,   11,   11, 0x09,
      69,   11,   11,   11, 0x09,
      85,   11,   11,   11, 0x09,
      99,   11,   11,   11, 0x09,

 // properties: name, type, flags
     134,  113, 0x0009510b,
     149,  143, 0x87095103,

       0        // eod
};

static const char qt_meta_stringdata_CardWindow[] = {
    "CardWindow\0\0slotLoadingFinished()\0"
    "state\0slotDisplayStateChanged(int)\0"
    "slotUiRotated()\0slotShowIME()\0"
    "slotHideIME()\0CardWindow::Position\0"
    "position\0float\0dimming\0"
};

void CardWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CardWindow *_t = static_cast<CardWindow *>(_o);
        switch (_id) {
        case 0: _t->slotLoadingFinished(); break;
        case 1: _t->slotDisplayStateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->slotUiRotated(); break;
        case 3: _t->slotShowIME(); break;
        case 4: _t->slotHideIME(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CardWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CardWindow::staticMetaObject = {
    { &HostWindow::staticMetaObject, qt_meta_stringdata_CardWindow,
      qt_meta_data_CardWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CardWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CardWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CardWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CardWindow))
        return static_cast<void*>(const_cast< CardWindow*>(this));
    return HostWindow::qt_metacast(_clname);
}

int CardWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = HostWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< CardWindow::Position*>(_v) = position(); break;
        case 1: *reinterpret_cast< float*>(_v) = dimming(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setPosition(*reinterpret_cast< CardWindow::Position*>(_v)); break;
        case 1: setDimming(*reinterpret_cast< float*>(_v)); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
