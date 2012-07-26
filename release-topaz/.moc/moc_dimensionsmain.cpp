/****************************************************************************
** Meta object code from reading C++ file 'dimensionsmain.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/dimensionsmain.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dimensionsmain.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DimensionsUI[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x05,
      28,   13,   13,   13, 0x05,
      51,   45,   13,   13, 0x05,
      98,   13,   13,   13, 0x25,
     113,   45,   13,   13, 0x05,
     160,   13,   13,   13, 0x25,
     175,   13,   13,   13, 0x05,
     212,   13,   13,   13, 0x05,
     243,   13,   13,   13, 0x05,
     275,   13,   13,   13, 0x05,
     313,   13,   13,   13, 0x05,

 // slots: signature, parameters, type, tag, flags
     352,   13,   13,   13, 0x0a,
     379,   13,   13,   13, 0x0a,
     408,   13,   13,   13, 0x0a,
     441,  432,   13,   13, 0x0a,
     471,   13,   13,   13, 0x2a,
     497,   13,   13,   13, 0x0a,
     519,   13,   13,   13, 0x0a,
     540,   13,   13,   13, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DimensionsUI[] = {
    "DimensionsUI\0\0signalReady()\0"
    "signalNotReady()\0cause\0"
    "signalHideMe(DimensionsTypes::HideCause::Enum)\0"
    "signalHideMe()\0"
    "signalShowMe(DimensionsTypes::ShowCause::Enum)\0"
    "signalShowMe()\0signalDropIconOnQuicklaunch(QString)\0"
    "signalRelayOWMHidingLauncher()\0"
    "signalRelayOWMShowingLauncher()\0"
    "signalRelayOWMHidingUniversalSearch()\0"
    "signalRelayOWMShowingUniversalSearch()\0"
    "slotQuicklaunchFullyOpen()\0"
    "slotQuicklaunchFullyClosed()\0"
    "slotLauncherFullyOpen()\0reCreate\0"
    "slotLauncherFullyClosed(bool)\0"
    "slotLauncherFullyClosed()\0"
    "slotDestroyLauncher()\0slotCreateLauncher()\0"
    "slotReCreateLauncher()\0"
};

void DimensionsUI::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DimensionsUI *_t = static_cast<DimensionsUI *>(_o);
        switch (_id) {
        case 0: _t->signalReady(); break;
        case 1: _t->signalNotReady(); break;
        case 2: _t->signalHideMe((*reinterpret_cast< DimensionsTypes::HideCause::Enum(*)>(_a[1]))); break;
        case 3: _t->signalHideMe(); break;
        case 4: _t->signalShowMe((*reinterpret_cast< DimensionsTypes::ShowCause::Enum(*)>(_a[1]))); break;
        case 5: _t->signalShowMe(); break;
        case 6: _t->signalDropIconOnQuicklaunch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->signalRelayOWMHidingLauncher(); break;
        case 8: _t->signalRelayOWMShowingLauncher(); break;
        case 9: _t->signalRelayOWMHidingUniversalSearch(); break;
        case 10: _t->signalRelayOWMShowingUniversalSearch(); break;
        case 11: _t->slotQuicklaunchFullyOpen(); break;
        case 12: _t->slotQuicklaunchFullyClosed(); break;
        case 13: _t->slotLauncherFullyOpen(); break;
        case 14: _t->slotLauncherFullyClosed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->slotLauncherFullyClosed(); break;
        case 16: _t->slotDestroyLauncher(); break;
        case 17: _t->slotCreateLauncher(); break;
        case 18: _t->slotReCreateLauncher(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DimensionsUI::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DimensionsUI::staticMetaObject = {
    { &Window::staticMetaObject, qt_meta_stringdata_DimensionsUI,
      qt_meta_data_DimensionsUI, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DimensionsUI::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DimensionsUI::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DimensionsUI::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DimensionsUI))
        return static_cast<void*>(const_cast< DimensionsUI*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< DimensionsUI*>(this));
    return Window::qt_metacast(_clname);
}

int DimensionsUI::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Window::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void DimensionsUI::signalReady()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void DimensionsUI::signalNotReady()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void DimensionsUI::signalHideMe(DimensionsTypes::HideCause::Enum _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void DimensionsUI::signalShowMe(DimensionsTypes::ShowCause::Enum _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void DimensionsUI::signalDropIconOnQuicklaunch(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void DimensionsUI::signalRelayOWMHidingLauncher()
{
    QMetaObject::activate(this, &staticMetaObject, 7, 0);
}

// SIGNAL 8
void DimensionsUI::signalRelayOWMShowingLauncher()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void DimensionsUI::signalRelayOWMHidingUniversalSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void DimensionsUI::signalRelayOWMShowingUniversalSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 10, 0);
}
static const uint qt_meta_data_Quicklauncher[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       1,   59, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x05,
      29,   14,   14,   14, 0x05,
      52,   46,   14,   14, 0x05,
      99,   14,   14,   14, 0x25,
     114,   46,   14,   14, 0x05,
     161,   14,   14,   14, 0x25,

 // slots: signature, parameters, type, tag, flags
     176,   14,   14,   14, 0x0a,
     203,   14,   14,   14, 0x0a,
     229,   14,   14,   14, 0x0a,

 // properties: name, type, flags
     263,  257, ((uint)QMetaType::QReal << 24) | 0x00095103,

       0        // eod
};

static const char qt_meta_stringdata_Quicklauncher[] = {
    "Quicklauncher\0\0signalReady()\0"
    "signalNotReady()\0cause\0"
    "signalHideMe(DimensionsTypes::HideCause::Enum)\0"
    "signalHideMe()\0"
    "signalShowMe(DimensionsTypes::ShowCause::Enum)\0"
    "signalShowMe()\0slotDestroyQuickLauncher()\0"
    "slotCreateQuickLauncher()\0"
    "slotReCreateQuickLauncher()\0qreal\0"
    "backgroundOpacity\0"
};

void Quicklauncher::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Quicklauncher *_t = static_cast<Quicklauncher *>(_o);
        switch (_id) {
        case 0: _t->signalReady(); break;
        case 1: _t->signalNotReady(); break;
        case 2: _t->signalHideMe((*reinterpret_cast< DimensionsTypes::HideCause::Enum(*)>(_a[1]))); break;
        case 3: _t->signalHideMe(); break;
        case 4: _t->signalShowMe((*reinterpret_cast< DimensionsTypes::ShowCause::Enum(*)>(_a[1]))); break;
        case 5: _t->signalShowMe(); break;
        case 6: _t->slotDestroyQuickLauncher(); break;
        case 7: _t->slotCreateQuickLauncher(); break;
        case 8: _t->slotReCreateQuickLauncher(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Quicklauncher::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Quicklauncher::staticMetaObject = {
    { &Window::staticMetaObject, qt_meta_stringdata_Quicklauncher,
      qt_meta_data_Quicklauncher, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Quicklauncher::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Quicklauncher::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Quicklauncher::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Quicklauncher))
        return static_cast<void*>(const_cast< Quicklauncher*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< Quicklauncher*>(this));
    return Window::qt_metacast(_clname);
}

int Quicklauncher::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Window::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< qreal*>(_v) = backgroundOpacity(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setBackgroundOpacity(*reinterpret_cast< qreal*>(_v)); break;
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
void Quicklauncher::signalReady()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void Quicklauncher::signalNotReady()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void Quicklauncher::signalHideMe(DimensionsTypes::HideCause::Enum _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void Quicklauncher::signalShowMe(DimensionsTypes::ShowCause::Enum _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_END_MOC_NAMESPACE
