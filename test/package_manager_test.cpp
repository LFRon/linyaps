/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong@uniontech.com
 *
 * Maintainer: huqinghong@uniontech.com
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>
#include <QDebug>
#include <thread>
#include <future>

#include "src/module/package/package.h"
#include "src/module/util/app_status.h"
#include "src/service/impl/json_register_inc.h"
#include "src/service/impl/param_option.h"
#include "src/service/impl/reply.h"
#include "package_manager.h"

/*!
 * start service
 */
void start_ll_service()
{
    setenv("DISPLAY", ":0", 1);
    setenv("XAUTHORITY", "~/.Xauthority", 1);
    // 需要进入到test目录进行ll-test
    system("../bin/ll-service &");
}

/*!
 * stop service
 */
void stop_ll_service()
{
    system("pkill -f ../bin/ll-service");
}

/*
 * 查询测试服务器连接状态
 *
 * @return bool: true: 可以连上测试服务器 false:连不上服务器
 */
bool getConnectStatus()
{
    // curl -o /dev/null -s -m 10 --connect-timeout 10 -w %{http_code} "https://linglong-api-dev.deepin.com/ostree/"
    // 返回200表示通
    const QString testServer = "https://linglong-api-dev.deepin.com/ostree/";
    QProcess proc;
    QStringList argstrList;
    argstrList << "-o"
               << "/dev/null"
               << "-s"
               << "-m"
               << "10"
               << "--connect-timeout"
               << "10"
               << "-w"
               << "%{http_code}" << testServer;
    argstrList.join(" ");
    proc.start("curl", argstrList);
    if (!proc.waitForStarted()) {
        qCritical() << "start curl failed!";
        return false;
    }
    proc.waitForFinished(3000);
    QString ret = proc.readAllStandardOutput();
    qInfo() << "ret msg:" << ret;
    bool connect = (ret.indexOf("200", Qt::CaseInsensitive) >= 0);
    return connect;
}

TEST(Package, install01)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    linglong::service::registerAllMetaType();
    linglong::package::registerAllMetaType();

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    // call dbus
    linglong::service::InstallParamOption installParam;
    installParam.appId = "com.deepin.linglong.test";
    QDBusPendingReply<linglong::service::Reply> reply = pm.Install(installParam);
    reply.waitForFinished();
    linglong::service::Reply retReply = reply.value();
    EXPECT_NE(retReply.code, RetCode(RetCode::pkg_install_success));
    // stop service
    stop_ll_service();
}

TEST(Package, install02)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    // call dbus
    linglong::service::InstallParamOption installParam;
    installParam.appId = "org.deepin.calculator";

    linglong::service::QueryParamOption paramOption;
    paramOption.appId = "installed";
    // 查询是否已安装
    linglong::package::AppMetaInfoList retMsg;
    QDBusPendingReply<linglong::service::QueryReply> dbusQuery = pm.Query(paramOption);
    dbusQuery.waitForFinished();
    linglong::service::QueryReply reply = dbusQuery.value();
    linglong::util::getAppMetaInfoListByJson(reply.result, retMsg);
    bool expectRet = true;
    for (auto const &it : retMsg) {
        if (it->appId == "org.deepin.calculator") {
            expectRet = false;
            break;
        }
    }
    QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Install(installParam);
    dbusReply.waitForFinished();
    linglong::service::Reply retReply = dbusReply.value();

    bool connect = getConnectStatus();
    if (!connect) {
        expectRet = false;
    }
    if (expectRet) {
        EXPECT_EQ(retReply.code, RetCode(RetCode::pkg_install_success));
    } else {
        EXPECT_NE(retReply.code, RetCode(RetCode::pkg_install_success));
    }

    // stop service
    stop_ll_service();
}

TEST(Package, query01)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    // test app not in repo
    linglong::service::QueryParamOption paramOption;
    paramOption.appId = "test.deepin.test";

    linglong::package::AppMetaInfoList retMsg;
    QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
    dbusReply.waitForFinished();
    linglong::service::QueryReply reply = dbusReply.value();
    linglong::util::getAppMetaInfoListByJson(reply.result, retMsg);
    bool ret = retMsg.size() == 0 ? true : false;
    EXPECT_EQ(ret, true);
    // stop service
    stop_ll_service();
}

TEST(Package, query02)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    // test app not in repo
    linglong::service::QueryParamOption paramOption;
    paramOption.appId = QString();

    linglong::package::AppMetaInfoList retMsg;
    QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
    dbusReply.waitForFinished();
    linglong::service::QueryReply reply = dbusReply.value();
    linglong::util::getAppMetaInfoListByJson(reply.result, retMsg);
    bool ret = retMsg.size() == 0 ? true : false;
    EXPECT_EQ(ret, true);
    // stop service
    stop_ll_service();
}

/*
 * 包管理query方法模糊查找正则表达式测试用例
 */
TEST(Package, query03)
{
    QStringList pkgsList = {"ab", "adbc", "abdc", "aBc", "abCd", "dAbc", "aBcabcd"};
    QString pkgName = "abc";
    // 模糊查找 匹配 包名出现一次或多次，忽略大小写
    QString filterString = "(" + pkgName + ")+";
    QStringList appList = pkgsList.filter(QRegExp(filterString, Qt::CaseInsensitive));
    bool ret = appList.size() == 4 ? true : false;
    EXPECT_EQ(ret, true);
}

/*
 * 包管理query方法查找仓库描述文件Appstream.json中软件包信息正则表达式测试用例
 */
TEST(Package, query04)
{
    QStringList pkgsList = {"ab", "adbc", "abdc", "aBc", "abCd", "dAbc", "abcabc", "abcabc"};
    QString pkgName = "abc";
    // 匹配以包名开头的字符串，区分大小写
    QString filterString = "^" + pkgName + "+";
    QStringList appList = pkgsList.filter(QRegExp(filterString, Qt::CaseSensitive));
    bool ret = appList.size() == 2 ? true : false;
    EXPECT_EQ(ret, true);
}

TEST(Package, IDQueryRegExp)
{
    auto filter = [](const QStringList &idList, const QString &query) -> QStringList {
        QString filterString = ".*" + query + ".*";
        return idList.filter(QRegExp(filterString, Qt::CaseInsensitive));
    };

    // data collect
    QStringList packageIDList = {"com.gitlab.newsflash",
                                 "com.nextcloud.desktopclient.nextcloud",
                                 "io.typora.Typora",
                                 "org.freedesktop.LinuxAudio.Plugins.swh",
                                 "org.freedesktop.Platform.GL.default",
                                 "org.freedesktop.Platform.GL.nvidia-470-74",
                                 "org.zotero.Zotero",
                                 "org.gabmus.gfeeds",
                                 "com.diy_fever.DIYLayoutCreator"};

    // normal
    auto result = filter(packageIDList, "typora");
    EXPECT_EQ(result.size(), 1);

    // case insensitive
    result = filter(packageIDList, "diyLayoutCreator");
    EXPECT_EQ(result.size(), 1);

    // multi result
    result = filter(packageIDList, "GL");
    EXPECT_EQ(result.size(), 2);

    // empty
    result = filter(packageIDList, "");
    EXPECT_EQ(result.size(), packageIDList.size());

    // special character -
    result = filter(packageIDList, "-470-");
    EXPECT_EQ(result.size(), 1);

    // special character _
    result = filter(packageIDList, "_");
    EXPECT_EQ(result.size(), 1);

    // special character .
    result = filter(packageIDList, "lient.next");
    EXPECT_EQ(result.size(), 1);

    // illegal character 
    result = filter(packageIDList, "\u0098");
    EXPECT_EQ(result.size(), 0);
}

TEST(Package, list01)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    // test app not in repo
    linglong::service::QueryParamOption paramOption;
    paramOption.appId = "";

    linglong::package::AppMetaInfoList retMsg;
    QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
    dbusReply.waitForFinished();
    linglong::service::QueryReply reply = dbusReply.value();
    linglong::util::getAppMetaInfoListByJson(reply.result, retMsg);
    bool ret = retMsg.size() == 0 ? true : false;
    EXPECT_EQ(ret, true);
    // stop service
    stop_ll_service();
}

TEST(Package, list02)
{
    // start service
    std::thread startQdbus(start_ll_service);
    startQdbus.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ComDeepinLinglongPackageManagerInterface pm("com.deepin.linglong.AppManager", "/com/deepin/linglong/PackageManager",
                                                QDBusConnection::sessionBus());
    linglong::service::QueryParamOption paramOption;
    paramOption.appId = "installed";
    linglong::package::AppMetaInfoList retMsg;
    QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
    dbusReply.waitForFinished();
    linglong::service::QueryReply reply = dbusReply.value();
    linglong::util::getAppMetaInfoListByJson(reply.result, retMsg);
    bool ret = retMsg.size() > 0 ? true : false;

    QString dbPath = "/deepin/linglong/layers/InstalledAppInfo.db";
    bool expectRet = true;
    if (!linglong::util::fileExists(dbPath)) {
        expectRet = false;
        qInfo() << "no installed app in system";
    }
    EXPECT_EQ(ret, expectRet);
    // stop service
    stop_ll_service();
}