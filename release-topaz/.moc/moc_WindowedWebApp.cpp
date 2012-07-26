/****************************************************************************
** Meta object code from reading C++ file 'WindowedWebApp.h'
**
** Created: Mon Jul 23 16:14:42 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/webbase/WindowedWebApp.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WindowedWebApp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_WindowedWebApp[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      16,   15,   15,   15, 0x0a,
      37,   15,   15,   15, 0x09,
      63,   15,   15,   15, 0x09,
      88,   15,   15,   15, 0x09,
     115,   15,   15,   15, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_WindowedWebApp[] = {
    "WindowedWebApp\0\0closeWindowRequest()\0"
    "slotInvalidateRect(QRect)\0"
    "slotResizeContent(QSize)\0"
    "slotGeometryChanged(QRect)\0"
    "PrvCbPaintTimeout()\0"
};

void WindowedWebApp::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        WindowedWebApp *_t = static_cast<WindowedWebApp *>(_o);
        switch (_id) {
        case 0: _t->closeWindowRequest(); break;
        case 1: _t->slotInvalidateRect((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 2: _t->slotResizeContent((*reinterpret_cast< const QSize(*)>(_a[1]))); break;
        case 3: _t->slotGeometryChanged((*reinterpret_cast< const QRect(*)>(_a[1]))); break;
        case 4: _t->PrvCbPaintTimeout(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData WindowedWebApp::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject WindowedWebApp::staticMetaObject = {
    { &WebAppBase::staticMetaObject, qt_meta_stringdata_WindowedWebApp,
      qt_meta_data_WindowedWebApp, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &WindowedWebApp::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *WindowedWebApp::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *WindowedWebApp::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WindowedWebApp))
        return static_cast<void*>(const_cast< WindowedWebApp*>(this));
    if (!strcmp(_clname, "PIpcChannelListener"))
        return static_cast< PIpcChannelListener*>(const_cast< WindowedWebApp*>(this));
    return WebAppBase::qt_metacast(_clname);
}

int WindowedWebApp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WebAppBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
