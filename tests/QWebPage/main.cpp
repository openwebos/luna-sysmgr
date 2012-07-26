
#include <QApplication>
#include <QtWebKit>
#include <QWebInspector>
#include <QUrl>
#include <QDir>
#include "SysMgrWebBridge.h"
#include <QDebug>

QUrl setupLocalFile()
{
//    QFile::remove("./index.html");
    QFile html(":/html/index.html");
    html.setPermissions(QFile::ReadOther|QFile::WriteOther);
    html.copy("./index.html");
    QFile html1(":/html/child.html");
    html1.setPermissions(QFile::ReadOther|QFile::WriteOther);
    html1.copy("./child.html");
    return QUrl::fromLocalFile(QDir::currentPath() + "/index.html");
}

int main(int argc, char **argv)
{
    ::setenv("QT_PLUGIN_PATH", "/usr/plugins", 1);
    ::setenv("QT_QPA_FONTDIR", "/usr/share/fonts", 1);
    ::setenv("QT_QPA_PLATFORM", "minimal", 1);

    QApplication app(argc, argv);

//    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    QUrl url = setupLocalFile();
    SysMgrWebBridge* bridge = new SysMgrWebBridge(true, url);

//    QWebInspector* inspector = new QWebInspector;
//    inspector->setPage(bridge->page());
//    inspector->setVisible(true);

    return app.exec();
}
