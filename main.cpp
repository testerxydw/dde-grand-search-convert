// SPDX-FileCopyrightText: 2026 Deepin Plugin Contest
// SPDX-License-Identifier: GPL-3.0-or-later

#include "convertsearch.h"
#include "searchpluginadaptor.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QLoggingCategory>

#define DBUS_SERVICE_NAME "org.deepin.grandsearch.convert"
#define DBUS_OBJECT_PATH  "/org/deepin/grandsearch/convert"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("convert-search-plugin");
    QLoggingCategory::setFilterRules("convert.search.plugin.debug=true");

    ConvertSearch searcher;
    SearchPluginAdaptor adaptor(&searcher);

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService(DBUS_SERVICE_NAME)) {
        qCritical() << "Failed to register DBus service:" << DBUS_SERVICE_NAME
                    << connection.lastError().message();
        return 1;
    }
    if (!connection.registerObject(DBUS_OBJECT_PATH, &searcher)) {
        qCritical() << "Failed to register DBus object:" << DBUS_OBJECT_PATH;
        return 1;
    }

    qInfo() << "Convert search plugin started - Service:" << DBUS_SERVICE_NAME
            << "Path:" << DBUS_OBJECT_PATH;
    return app.exec();
}
