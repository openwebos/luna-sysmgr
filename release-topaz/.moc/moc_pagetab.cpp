/****************************************************************************
** Meta object code from reading C++ file 'pagetab.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/bars/pagetab.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pagetab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PageTab[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x05,
      30,    8,    8,    8, 0x05,
      58,    8,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
      82,    8,    8,    8, 0x0a,
      98,    8,    8,    8, 0x0a,
     121,  116,    8,    8, 0x09,
     166,    8,    8,    8, 0x09,
     190,    8,    8,    8, 0x09,
     225,  219,    8,    8, 0x09,
     268,  219,    8,    8, 0x09,
     310,  219,    8,    8, 0x09,
     355,    8,    8,    8, 0x09,
     379,    8,    8,    8, 0x09,
     405,    8,    8,    8, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_PageTab[] = {
    "PageTab\0\0signalActivatedTap()\0"
    "signalActivatedTapAndHold()\0"
    "signalSlotHighlighted()\0slotHighlight()\0"
    "slotUnHighlight()\0mode\0"
    "slotSetDisplayMode(PageTabDisplayMode::Enum)\0"
    "slotRelatedPageActive()\0"
    "slotRelatedPageDeactivated()\0event\0"
    "mousePressEvent(QGraphicsSceneMouseEvent*)\0"
    "mouseMoveEvent(QGraphicsSceneMouseEvent*)\0"
    "mouseReleaseEvent(QGraphicsSceneMouseEvent*)\0"
    "slotAnimationFinished()\0"
    "slotInteractionsBlocked()\0"
    "slotInteractionsAllowed()\0"
};

void PageTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PageTab *_t = static_cast<PageTab *>(_o);
        switch (_id) {
        case 0: _t->signalActivatedTap(); break;
        case 1: _t->signalActivatedTapAndHold(); break;
        case 2: _t->signalSlotHighlighted(); break;
        case 3: _t->slotHighlight(); break;
        case 4: _t->slotUnHighlight(); break;
        case 5: _t->slotSetDisplayMode((*reinterpret_cast< PageTabDisplayMode::Enum(*)>(_a[1]))); break;
        case 6: _t->slotRelatedPageActive(); break;
        case 7: _t->slotRelatedPageDeactivated(); break;
        case 8: _t->mousePressEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 9: _t->mouseMoveEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 10: _t->mouseReleaseEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 11: _t->slotAnimationFinished(); break;
        case 12: _t->slotInteractionsBlocked(); break;
        case 13: _t->slotInteractionsAllowed(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PageTab::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PageTab::staticMetaObject = {
    { &ThingPaintable::staticMetaObject, qt_meta_stringdata_PageTab,
      qt_meta_data_PageTab, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PageTab::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PageTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PageTab::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PageTab))
        return static_cast<void*>(const_cast< PageTab*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< PageTab*>(this));
    return ThingPaintable::qt_metacast(_clname);
}

int PageTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ThingPaintable::qt_metacall(_c, _id, _a);
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
void PageTab::signalActivatedTap()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void PageTab::signalActivatedTapAndHold()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void PageTab::signalSlotHighlighted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
