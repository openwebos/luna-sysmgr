/****************************************************************************
** Meta object code from reading C++ file 'DockModeAppMenuContainer.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/dock/DockModeAppMenuContainer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DockModeAppMenuContainer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DockModeAppMenuContainer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      30,   26,   25,   25, 0x05,
      78,   25,   25,   25, 0x05,

 // methods: signature, parameters, type, tag, flags
     111,   25,  107,   25, 0x02,
     122,   25,  107,   25, 0x02,
     134,   25,  107,   25, 0x02,
     160,   25,   25,   25, 0x02,

       0        // eod
};

static const char qt_meta_stringdata_DockModeAppMenuContainer[] = {
    "DockModeAppMenuContainer\0\0dlp\0"
    "signalDockModeAppSelected(DockModeLaunchPoint*)\0"
    "signalContainerSizeChanged()\0int\0"
    "getWidth()\0getHeight()\0getMaximumHeightForMenu()\0"
    "mouseWasGrabbedByParent()\0"
};

void DockModeAppMenuContainer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DockModeAppMenuContainer *_t = static_cast<DockModeAppMenuContainer *>(_o);
        switch (_id) {
        case 0: _t->signalDockModeAppSelected((*reinterpret_cast< DockModeLaunchPoint*(*)>(_a[1]))); break;
        case 1: _t->signalContainerSizeChanged(); break;
        case 2: { int _r = _t->getWidth();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 3: { int _r = _t->getHeight();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 4: { int _r = _t->getMaximumHeightForMenu();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 5: _t->mouseWasGrabbedByParent(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DockModeAppMenuContainer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DockModeAppMenuContainer::staticMetaObject = {
    { &QGraphicsObject::staticMetaObject, qt_meta_stringdata_DockModeAppMenuContainer,
      qt_meta_data_DockModeAppMenuContainer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DockModeAppMenuContainer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DockModeAppMenuContainer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DockModeAppMenuContainer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DockModeAppMenuContainer))
        return static_cast<void*>(const_cast< DockModeAppMenuContainer*>(this));
    return QGraphicsObject::qt_metacast(_clname);
}

int DockModeAppMenuContainer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void DockModeAppMenuContainer::signalDockModeAppSelected(DockModeLaunchPoint * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DockModeAppMenuContainer::signalContainerSizeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
