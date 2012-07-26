/****************************************************************************
** Meta object code from reading C++ file 'DashboardWindowContainer.h'
**
** Created: Mon Jul 23 16:14:43 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/notifications/DashboardWindowContainer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DashboardWindowContainer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DashboardWindowContainer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: signature, parameters, type, tag, flags
      28,   26,   25,   25, 0x05,
      64,   26,   25,   25, 0x05,
     103,   25,   25,   25, 0x05,
     133,   25,   25,   25, 0x05,
     147,   25,   25,   25, 0x05,
     193,  176,   25,   25, 0x05,

 // slots: signature, parameters, type, tag, flags
     219,   25,   25,   25, 0x08,
     250,   25,   25,   25, 0x08,
     298,  280,   25,   25, 0x08,
     348,  346,   25,   25, 0x08,
     380,  346,   25,   25, 0x08,

 // methods: signature, parameters, type, tag, flags
     423,   25,  419,   25, 0x02,
     434,   25,  419,   25, 0x02,
     446,   25,   25,   25, 0x02,

       0        // eod
};

static const char qt_meta_stringdata_DashboardWindowContainer[] = {
    "DashboardWindowContainer\0\0w\0"
    "signalWindowAdded(DashboardWindow*)\0"
    "signalWindowsRemoved(DashboardWindow*)\0"
    "signalViewportHeightChanged()\0"
    "signalEmpty()\0signalContainerSizeChanged()\0"
    "itemBeingDragged\0signalItemDragState(bool)\0"
    "slotProcessAnimationComplete()\0"
    "slotDeleteAnimationFinished()\0"
    "r,,screenResizing\0"
    "slotNegativeSpaceAboutToChange(QRect,bool,bool)\0"
    "r\0slotNegativeSpaceChanged(QRect)\0"
    "slotNegativeSpaceChangeFinished(QRect)\0"
    "int\0getWidth()\0getHeight()\0"
    "mouseWasGrabbedByParent()\0"
};

void DashboardWindowContainer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DashboardWindowContainer *_t = static_cast<DashboardWindowContainer *>(_o);
        switch (_id) {
        case 0: _t->signalWindowAdded((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        case 1: _t->signalWindowsRemoved((*reinterpret_cast< DashboardWindow*(*)>(_a[1]))); break;
        case 2: _t->signalViewportHeightChanged(); break;
        case 3: _t->signalEmpty(); break;
        case 4: _t->signalContainerSizeChanged(); break;
        case 5: _t->signalItemDragState((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->slotProcessAnimationComplete(); break;
        case 7: _t->slotDeleteAnimationFinished(); break;
        case 8: _t->slotNegativeSpaceAboutToChange((*reinterpret_cast< const QRect(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 9: _t->slotNegativeSpaceChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 10: _t->slotNegativeSpaceChangeFinished((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 11: { int _r = _t->getWidth();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 12: { int _r = _t->getHeight();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 13: _t->mouseWasGrabbedByParent(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DashboardWindowContainer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DashboardWindowContainer::staticMetaObject = {
    { &GraphicsItemContainer::staticMetaObject, qt_meta_stringdata_DashboardWindowContainer,
      qt_meta_data_DashboardWindowContainer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DashboardWindowContainer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DashboardWindowContainer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DashboardWindowContainer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DashboardWindowContainer))
        return static_cast<void*>(const_cast< DashboardWindowContainer*>(this));
    return GraphicsItemContainer::qt_metacast(_clname);
}

int DashboardWindowContainer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = GraphicsItemContainer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void DashboardWindowContainer::signalWindowAdded(DashboardWindow * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DashboardWindowContainer::signalWindowsRemoved(DashboardWindow * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DashboardWindowContainer::signalViewportHeightChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void DashboardWindowContainer::signalEmpty()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void DashboardWindowContainer::signalContainerSizeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void DashboardWindowContainer::signalItemDragState(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
