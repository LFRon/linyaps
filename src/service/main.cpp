/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <DLog>
#include <QCoreApplication>
#include <module/runtime/app.h>
#include <impl/json_register_inc.h>
#include <impl/qdbus_retmsg.h>

#include "packagemanageradaptor.h"
#include "jobmanageradaptor.h"
#include "uapmanageradaptor.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    ociJsonRegister();
    qJsonRegister<PackageMoc>();
    qJsonRegister<App>();
    

    // register qdbus type
    RegisterDbusType();

    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.registerService("com.deepin.linglong.AppManager")) {
        qCritical() << "service exist" << dbus.lastError();
        return -1;
    }

    PackageManagerAdaptor pma(PackageManager::instance());
    JobManagerAdaptor jma(JobManager::instance());
    UapManagerAdaptor uma(UapManager::instance());

    // TODO(se): 需要进行错误处理
    dbus.registerObject("/com/deepin/linglong/PackageManager",
                        PackageManager::instance());
    dbus.registerObject("/com/deepin/linglong/JobManager",
                        JobManager::instance());
    dbus.registerObject("/com/deepin/linglong/UapManager",
                        UapManager::instance());

    return QCoreApplication::exec();
}
