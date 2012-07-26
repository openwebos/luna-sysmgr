/****************************************************************************
** Meta object code from reading C++ file 'KineticScroller.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/util/KineticScroller.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'KineticScroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_KineticScroller[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       1,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      27,   17,   16,   16, 0x05,

 // slots: signature, parameters, type, tag, flags
      43,   16,   16,   16, 0x08,
      64,   16,   16,   16, 0x08,
      84,   16,   16,   16, 0x08,
     114,   16,   16,   16, 0x08,

 // properties: name, type, flags
     141,  135, ((uint)QMetaType::QReal << 24) | 0x00095103,

       0        // eod
};

static const char qt_meta_stringdata_KineticScroller[] = {
    "KineticScroller\0\0oldScroll\0scrolled(qreal)\0"
    "flickAnimationTick()\0overscrollTrigger()\0"
    "flickTransitionToOverscroll()\0"
    "stopFlickAnimation()\0qreal\0scrollOffset\0"
};

void KineticScroller::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        KineticScroller *_t = static_cast<KineticScroller *>(_o);
        switch (_id) {
        case 0: _t->scrolled((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 1: _t->flickAnimationTick(); break;
        case 2: _t->overscrollTrigger(); break;
        case 3: _t->flickTransitionToOverscroll(); break;
        case 4: _t->stopFlickAnimation(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData KineticScroller::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject KineticScroller::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_KineticScroller,
      qt_meta_data_KineticScroller, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &KineticScroller::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *KineticScroller::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *KineticScroller::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_KineticScroller))
        return static_cast<void*>(const_cast< KineticScroller*>(this));
    return QObject::qt_metacast(_clname);
}

int KineticScroller::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
        case 0: *reinterpret_cast< qreal*>(_v) = scrollOffset(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setScrollOffset(*reinterpret_cast< qreal*>(_v)); break;
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
void KineticScroller::scrolled(qreal _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
