/****************************************************************************
** Meta object code from reading C++ file 'StatusBarItemGroup.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/status-bar/StatusBarItemGroup.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusBarItemGroup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StatusBarItemGroup[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       2,   44, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      27,   20,   19,   19, 0x05,
      61,   55,   19,   19, 0x05,

 // slots: signature, parameters, type, tag, flags
      98,   19,   19,   19, 0x08,
     129,   19,   19,   19, 0x08,
     157,   19,   19,   19, 0x08,
     194,  188,   19,   19, 0x08,

 // properties: name, type, flags
     238,  232, ((uint)QMetaType::QReal << 24) | 0x00095103,
     256,  232, ((uint)QMetaType::QReal << 24) | 0x00095103,

       0        // eod
};

static const char qt_meta_stringdata_StatusBarItemGroup[] = {
    "StatusBarItemGroup\0\0active\0"
    "signalActionTriggered(bool)\0group\0"
    "signalActivated(StatusBarItemGroup*)\0"
    "slotChildBoundingRectChanged()\0"
    "slotFadeAnimationFinished()\0"
    "slotOverlayAnimationFinished()\0value\0"
    "slotOverlayAnimValueChanged(QVariant)\0"
    "qreal\0arrowAnimProgress\0overlayOpacity\0"
};

void StatusBarItemGroup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StatusBarItemGroup *_t = static_cast<StatusBarItemGroup *>(_o);
        switch (_id) {
        case 0: _t->signalActionTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->signalActivated((*reinterpret_cast< StatusBarItemGroup*(*)>(_a[1]))); break;
        case 2: _t->slotChildBoundingRectChanged(); break;
        case 3: _t->slotFadeAnimationFinished(); break;
        case 4: _t->slotOverlayAnimationFinished(); break;
        case 5: _t->slotOverlayAnimValueChanged((*reinterpret_cast< const QVariant(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StatusBarItemGroup::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StatusBarItemGroup::staticMetaObject = {
    { &StatusBarItem::staticMetaObject, qt_meta_stringdata_StatusBarItemGroup,
      qt_meta_data_StatusBarItemGroup, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StatusBarItemGroup::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StatusBarItemGroup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StatusBarItemGroup::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBarItemGroup))
        return static_cast<void*>(const_cast< StatusBarItemGroup*>(this));
    return StatusBarItem::qt_metacast(_clname);
}

int StatusBarItemGroup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StatusBarItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< qreal*>(_v) = arrowAnimProgress(); break;
        case 1: *reinterpret_cast< qreal*>(_v) = overlayOpacity(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setArrowAnimProgress(*reinterpret_cast< qreal*>(_v)); break;
        case 1: setOverlayOpacity(*reinterpret_cast< qreal*>(_v)); break;
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

// SIGNAL 0
void StatusBarItemGroup::signalActionTriggered(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StatusBarItemGroup::signalActivated(StatusBarItemGroup * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
