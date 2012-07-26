/****************************************************************************
** Meta object code from reading C++ file 'QmlAlertWindow.h'
**
** Created: Mon Jul 23 16:14:44 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/notifications/QmlAlertWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QmlAlertWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QmlAlertWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      16,   15,   15,   15, 0x08,
      28,   15,   15,   15, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_QmlAlertWindow[] = {
    "QmlAlertWindow\0\0slotClose()\0slotMore()\0"
};

void QmlAlertWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QmlAlertWindow *_t = static_cast<QmlAlertWindow *>(_o);
        switch (_id) {
        case 0: _t->slotClose(); break;
        case 1: _t->slotMore(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QmlAlertWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QmlAlertWindow::staticMetaObject = {
    { &AlertWindow::staticMetaObject, qt_meta_stringdata_QmlAlertWindow,
      qt_meta_data_QmlAlertWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QmlAlertWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QmlAlertWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QmlAlertWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QmlAlertWindow))
        return static_cast<void*>(const_cast< QmlAlertWindow*>(this));
    return AlertWindow::qt_metacast(_clname);
}

int QmlAlertWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AlertWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
