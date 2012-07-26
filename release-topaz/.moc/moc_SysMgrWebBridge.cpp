/****************************************************************************
** Meta object code from reading C++ file 'SysMgrWebBridge.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/webbase/SysMgrWebBridge.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SysMgrWebBridge.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SysMgrWebPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_SysMgrWebPage[] = {
    "SysMgrWebPage\0\0setRequestedGeometry(QRect)\0"
};

void SysMgrWebPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SysMgrWebPage *_t = static_cast<SysMgrWebPage *>(_o);
        switch (_id) {
        case 0: _t->setRequestedGeometry((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SysMgrWebPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SysMgrWebPage::staticMetaObject = {
    { &QWebPage::staticMetaObject, qt_meta_stringdata_SysMgrWebPage,
      qt_meta_data_SysMgrWebPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SysMgrWebPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SysMgrWebPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SysMgrWebPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SysMgrWebPage))
        return static_cast<void*>(const_cast< SysMgrWebPage*>(this));
    return QWebPage::qt_metacast(_clname);
}

int SysMgrWebPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWebPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
static const uint qt_meta_data_SysMgrWebBridge[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x05,
      46,   16,   16,   16, 0x05,
      75,   16,   16,   16, 0x05,
     103,   16,   16,   16, 0x05,
     127,   16,   16,   16, 0x05,
     155,   16,   16,   16, 0x05,
     178,   16,   16,   16, 0x05,

 // slots: signature, parameters, type, tag, flags
     239,  230,   16,   16, 0x09,
     261,   16,   16,   16, 0x09,
     282,  279,   16,   16, 0x09,
     305,   16,   16,   16, 0x09,
     335,   16,   16,   16, 0x09,
     355,   16,   16,   16, 0x09,
     391,   16,   16,   16, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_SysMgrWebBridge[] = {
    "SysMgrWebBridge\0\0signalResizedContents(QSize)\0"
    "signalGeometryChanged(QRect)\0"
    "signalInvalidateRect(QRect)\0"
    "signalLinkClicked(QUrl)\0"
    "signalTitleChanged(QString)\0"
    "signalUrlChanged(QUrl)\0"
    "signalViewportChanged(QWebPage::ViewportAttributes)\0"
    "progress\0slotLoadProgress(int)\0"
    "slotLoadStarted()\0ok\0slotLoadFinished(bool)\0"
    "slotViewportChangeRequested()\0"
    "slotSetupPage(QUrl)\0"
    "slotJavaScriptWindowObjectCleared()\0"
    "slotMicroFocusChanged()\0"
};

void SysMgrWebBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SysMgrWebBridge *_t = static_cast<SysMgrWebBridge *>(_o);
        switch (_id) {
        case 0: _t->signalResizedContents((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        case 1: _t->signalGeometryChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 2: _t->signalInvalidateRect((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 3: _t->signalLinkClicked((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        case 4: _t->signalTitleChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->signalUrlChanged((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        case 6: _t->signalViewportChanged((*reinterpret_cast< const QWebPage::ViewportAttributes(*)>(_a[1]))); break;
        case 7: _t->slotLoadProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->slotLoadStarted(); break;
        case 9: _t->slotLoadFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->slotViewportChangeRequested(); break;
        case 11: _t->slotSetupPage((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        case 12: _t->slotJavaScriptWindowObjectCleared(); break;
        case 13: _t->slotMicroFocusChanged(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SysMgrWebBridge::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SysMgrWebBridge::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SysMgrWebBridge,
      qt_meta_data_SysMgrWebBridge, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SysMgrWebBridge::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SysMgrWebBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SysMgrWebBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SysMgrWebBridge))
        return static_cast<void*>(const_cast< SysMgrWebBridge*>(this));
    if (!strcmp(_clname, "ProcessBase"))
        return static_cast< ProcessBase*>(const_cast< SysMgrWebBridge*>(this));
    return QObject::qt_metacast(_clname);
}

int SysMgrWebBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void SysMgrWebBridge::signalResizedContents(const QSize & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SysMgrWebBridge::signalGeometryChanged(const QRect & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SysMgrWebBridge::signalInvalidateRect(const QRect & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SysMgrWebBridge::signalLinkClicked(const QUrl & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SysMgrWebBridge::signalTitleChanged(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SysMgrWebBridge::signalUrlChanged(const QUrl & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void SysMgrWebBridge::signalViewportChanged(const QWebPage::ViewportAttributes & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_END_MOC_NAMESPACE
