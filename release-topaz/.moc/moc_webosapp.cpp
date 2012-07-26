/****************************************************************************
** Meta object code from reading C++ file 'webosapp.h'
**
** Created: Mon Jul 23 16:14:45 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Src/lunaui/launcher/systeminterface/webosapp.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'webosapp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DimensionsSystemInterface__WebOSApp[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_DimensionsSystemInterface__WebOSApp[] = {
    "DimensionsSystemInterface::WebOSApp\0"
};

void DimensionsSystemInterface::WebOSApp::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData DimensionsSystemInterface::WebOSApp::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DimensionsSystemInterface::WebOSApp::staticMetaObject = {
    { &ExternalApp::staticMetaObject, qt_meta_stringdata_DimensionsSystemInterface__WebOSApp,
      qt_meta_data_DimensionsSystemInterface__WebOSApp, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DimensionsSystemInterface::WebOSApp::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DimensionsSystemInterface::WebOSApp::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DimensionsSystemInterface::WebOSApp::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DimensionsSystemInterface__WebOSApp))
        return static_cast<void*>(const_cast< WebOSApp*>(this));
    return ExternalApp::qt_metacast(_clname);
}

int DimensionsSystemInterface::WebOSApp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ExternalApp::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
