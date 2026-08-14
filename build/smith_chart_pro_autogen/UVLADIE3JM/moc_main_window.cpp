/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/main_window.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[26];
    char stringdata0[11];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[11];
    char stringdata5[12];
    char stringdata6[5];
    char stringdata7[22];
    char stringdata8[19];
    char stringdata9[12];
    char stringdata10[10];
    char stringdata11[10];
    char stringdata12[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 19),  // "updateLoadImpedance"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 15),  // "updateFrequency"
        QT_MOC_LITERAL(48, 10),  // "addElement"
        QT_MOC_LITERAL(59, 11),  // "ElementType"
        QT_MOC_LITERAL(71, 4),  // "type"
        QT_MOC_LITERAL(76, 21),  // "removeSelectedElement"
        QT_MOC_LITERAL(98, 18),  // "synthesizeLNetwork"
        QT_MOC_LITERAL(117, 11),  // "loadS1PFile"
        QT_MOC_LITERAL(129, 9),  // "exportSVG"
        QT_MOC_LITERAL(139, 9),  // "exportPDF"
        QT_MOC_LITERAL(149, 13)   // "updateToggles"
    },
    "MainWindow",
    "updateLoadImpedance",
    "",
    "updateFrequency",
    "addElement",
    "ElementType",
    "type",
    "removeSelectedElement",
    "synthesizeLNetwork",
    "loadS1PFile",
    "exportSVG",
    "exportPDF",
    "updateToggles"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   68,    2, 0x08,    1 /* Private */,
       3,    0,   69,    2, 0x08,    2 /* Private */,
       4,    1,   70,    2, 0x08,    3 /* Private */,
       7,    0,   73,    2, 0x08,    5 /* Private */,
       8,    0,   74,    2, 0x08,    6 /* Private */,
       9,    0,   75,    2, 0x08,    7 /* Private */,
      10,    0,   76,    2, 0x08,    8 /* Private */,
      11,    0,   77,    2, 0x08,    9 /* Private */,
      12,    0,   78,    2, 0x08,   10 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'updateLoadImpedance'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateFrequency'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addElement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ElementType, std::false_type>,
        // method 'removeSelectedElement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'synthesizeLNetwork'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadS1PFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportSVG'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportPDF'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateToggles'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateLoadImpedance(); break;
        case 1: _t->updateFrequency(); break;
        case 2: _t->addElement((*reinterpret_cast< std::add_pointer_t<ElementType>>(_a[1]))); break;
        case 3: _t->removeSelectedElement(); break;
        case 4: _t->synthesizeLNetwork(); break;
        case 5: _t->loadS1PFile(); break;
        case 6: _t->exportSVG(); break;
        case 7: _t->exportPDF(); break;
        case 8: _t->updateToggles(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
