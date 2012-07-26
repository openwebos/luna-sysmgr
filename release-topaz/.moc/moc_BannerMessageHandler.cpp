/****************************************************************************
** Meta object code from reading C++ file 'BannerMessageHandler.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/notifications/BannerMessageHandler.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BannerMessageHandler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_BannerMessageHandler[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
      41,   21,   21,   21, 0x08,
      61,   21,   21,   21, 0x08,
      81,   21,   21,   21, 0x08,
     105,   21,   21,   21, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_BannerMessageHandler[] = {
    "BannerMessageHandler\0\0signalHideBanner()\0"
    "aboutToShowBanner()\0aboutToHideBanner()\0"
    "bannerPositionChanged()\0"
    "bannerStateMachineFinished()\0"
};

void BannerMessageHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        BannerMessageHandler *_t = static_cast<BannerMessageHandler *>(_o);
        switch (_id) {
        case 0: _t->signalHideBanner(); break;
        case 1: _t->aboutToShowBanner(); break;
        case 2: _t->aboutToHideBanner(); break;
        case 3: _t->bannerPositionChanged(); break;
        case 4: _t->bannerStateMachineFinished(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData BannerMessageHandler::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject BannerMessageHandler::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_BannerMessageHandler,
      qt_meta_data_BannerMessageHandler, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &BannerMessageHandler::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *BannerMessageHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *BannerMessageHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_BannerMessageHandler))
        return static_cast<void*>(const_cast< BannerMessageHandler*>(this));
    return QObject::qt_metacast(_clname);
}

int BannerMessageHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void BannerMessageHandler::signalHideBanner()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
