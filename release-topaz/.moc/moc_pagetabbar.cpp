/****************************************************************************
** Meta object code from reading C++ file 'pagetabbar.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/elements/bars/pagetabbar.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pagetabbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PageTabBar[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x05,
      48,   11,   11,   11, 0x05,
      91,   11,   11,   11, 0x05,
     119,   11,   11,   11, 0x05,
     147,   11,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
     203,  176,   11,   11, 0x0a,
     241,  229,   11,   11, 0x2a,
     261,   11,   11,   11, 0x0a,
     288,  282,   11,   11, 0x09,
     331,  282,   11,   11, 0x09,
     373,  282,   11,   11, 0x09,
     418,   11,   11,   11, 0x09,
     442,   11,   11,   11, 0x09,
     464,   11,   11,   11, 0x09,
     493,   11,   11,   11, 0x09,
     511,   11,   11,   11, 0x09,
     545,   11,   11,   11, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_PageTabBar[] = {
    "PageTabBar\0\0signalTabForPageActivatedTap(Page*)\0"
    "signalTabForPageActivatedTapAndHold(Page*)\0"
    "signalAllTabHighlightsOff()\0"
    "signalInteractionsBlocked()\0"
    "signalInteractionsRestored()\0"
    "labelString,p_refersToPage\0"
    "slotAddTab(QString,Page*)\0labelString\0"
    "slotAddTab(QString)\0slotUnHighlightAll()\0"
    "event\0mousePressEvent(QGraphicsSceneMouseEvent*)\0"
    "mouseMoveEvent(QGraphicsSceneMouseEvent*)\0"
    "mouseReleaseEvent(QGraphicsSceneMouseEvent*)\0"
    "slotAnimationFinished()\0slotTabActivatedTap()\0"
    "slotTabActivatedTapAndHold()\0"
    "slotHighlighted()\0slotLauncherBlockedInteractions()\0"
    "slotLauncherAllowedInteractions()\0"
};

void PageTabBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PageTabBar *_t = static_cast<PageTabBar *>(_o);
        switch (_id) {
        case 0: _t->signalTabForPageActivatedTap((*reinterpret_cast< Page*(*)>(_a[1]))); break;
        case 1: _t->signalTabForPageActivatedTapAndHold((*reinterpret_cast< Page*(*)>(_a[1]))); break;
        case 2: _t->signalAllTabHighlightsOff(); break;
        case 3: _t->signalInteractionsBlocked(); break;
        case 4: _t->signalInteractionsRestored(); break;
        case 5: _t->slotAddTab((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< Page*(*)>(_a[2]))); break;
        case 6: _t->slotAddTab((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->slotUnHighlightAll(); break;
        case 8: _t->mousePressEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 9: _t->mouseMoveEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 10: _t->mouseReleaseEvent((*reinterpret_cast< QGraphicsSceneMouseEvent*(*)>(_a[1]))); break;
        case 11: _t->slotAnimationFinished(); break;
        case 12: _t->slotTabActivatedTap(); break;
        case 13: _t->slotTabActivatedTapAndHold(); break;
        case 14: _t->slotHighlighted(); break;
        case 15: _t->slotLauncherBlockedInteractions(); break;
        case 16: _t->slotLauncherAllowedInteractions(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PageTabBar::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PageTabBar::staticMetaObject = {
    { &ThingPaintable::staticMetaObject, qt_meta_stringdata_PageTabBar,
      qt_meta_data_PageTabBar, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PageTabBar::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PageTabBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PageTabBar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PageTabBar))
        return static_cast<void*>(const_cast< PageTabBar*>(this));
    if (!strcmp(_clname, "com.trolltech.Qt.QGraphicsItem"))
        return static_cast< QGraphicsItem*>(const_cast< PageTabBar*>(this));
    return ThingPaintable::qt_metacast(_clname);
}

int PageTabBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ThingPaintable::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void PageTabBar::signalTabForPageActivatedTap(Page * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PageTabBar::signalTabForPageActivatedTapAndHold(Page * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PageTabBar::signalAllTabHighlightsOff()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void PageTabBar::signalInteractionsBlocked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void PageTabBar::signalInteractionsRestored()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
