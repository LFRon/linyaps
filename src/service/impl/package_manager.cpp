/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "package_manager.h"

#include <signal.h>
#include <sys/types.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSysInfo>

#include "module/runtime/app.h"
#include "module/util/app_status.h"
#include "module/util/appinfo_cache.h"
#include "module/util/fs.h"
#include "module/util/sysinfo.h"
#include "module/package/info.h"
#include "module/repo/repo.h"
#include "dbus_retcode.h"
#include "job_manager.h"
#include "module/repo/ostree.h"

using linglong::util::fileExists;
using linglong::util::listDirFolders;
using linglong::dbus::RetCode;

using namespace linglong;

class PackageManagerPrivate
{
public:
    explicit PackageManagerPrivate(PackageManager *parent)
        : q_ptr(parent)
        , repo(repo::kRepoRoot)
    {
    }

    QMap<QString, QPointer<runtime::App>> apps;

    PackageManager *q_ptr = nullptr;

    repo::OSTree repo;
};

PackageManager::PackageManager()
    : dd_ptr(new PackageManagerPrivate(this))
{
    // 检查安装数据库信息
    checkInstalledAppDb();
    updateInstalledAppInfoDb();

    // 检查应用缓存信息
    checkAppCache();
}

PackageManager::~PackageManager() = default;

/*!
 * 下载软件包
 * @param packageIdList
 */
RetMessageList PackageManager::Download(const QStringList &packageIdList, const QString savePath)
{
    // Q_D(PackageManager);

    // return JobManager::instance()->CreateJob([](Job *jr) {
    //     在这里写入真正的实现
    //     QProcess p;
    //     p.setProgram("curl");
    //     p.setArguments({"https://www.baidu.com"});
    //     p.start();
    //     p.waitForStarted();
    //     p.waitForFinished(-1);
    //     qDebug() << p.readAllStandardOutput();
    //     qDebug() << "finish" << p.exitStatus() << p.state();
    // });
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qInfo() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Download(packageIdList, savePath);
}

/*!
 * 在线安装软件包
 * @param packageIdList
 */
RetMessageList PackageManager::Install(const QStringList &packageIdList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Install(packageIdList);
    }
    // Q_D(PackageManager);

    // return JobManager::instance()->CreateJob([](Job *jr) {
    //     // 在这里写入真正的实现
    //     QProcess p;
    //     p.setProgram("curl");
    //     p.setArguments({"https://www.baidu.com"});
    //     p.start();
    //     p.waitForStarted();
    //     p.waitForFinished(-1);
    //     qDebug() << p.readAllStandardOutput();
    //     qDebug() << "finish" << p.exitStatus() << p.state();
    // });
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    if (packageIdList.size() == 0) {
        qCritical() << "packageIdList input err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("packageIdList input err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qCritical() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Install(packageIdList, paramMap);
}

RetMessageList PackageManager::Uninstall(const QStringList &packageIdList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Uninstall(packageIdList);
    }

    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    if (packageIdList.size() == 0) {
        qCritical() << "packageIdList input err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("packageIdList input err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qCritical() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    return PackageManagerImpl::instance()->Uninstall(packageIdList, paramMap);
}

RetMessageList PackageManager::Update(const QStringList &packageIdList, const ParamStringMap &paramMap)
{
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    if (packageIdList.size() == 0) {
        qCritical() << "packageIdList input err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("packageIdList input err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qCritical() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    return PackageManagerImpl::instance()->Update(packageIdList, paramMap);
}

QString PackageManager::UpdateAll()
{
    sendErrorReply(QDBusError::NotSupported, message().member());
    return {};
}

/*
 * 查询软件包
 *
 * @param packageIdList: 软件包的appId
 *
 * @return linglong::package::AppMetaInfoList 查询结果列表
 */
linglong::package::AppMetaInfoList PackageManager::Query(const QStringList &packageIdList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Query(packageIdList);
    }
    if (packageIdList.size() == 0) {
        qCritical() << "packageIdList input err";
        return {};
    }
    QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qCritical() << "package name err";
        return {};
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Query(packageIdList, paramMap);
}

/*!
 * 安装本地软件包
 * @param packagePathList
 */
QString PackageManager::Import(const QStringList &packagePathList)
{
    sendErrorReply(QDBusError::NotSupported, message().member());
    return {};
}

/*
 * 执行软件包
 *
 * @param packageId: 软件包的appId
 * @param paramMap: 运行参数信息
 * 
 * @return RetMessageList: 运行结果信息
 */
RetMessageList PackageManager::Start(const QString &packageId, const ParamStringMap &paramMap)
{
    Q_D(PackageManager);

    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);

    // 获取版本信息
    QString version = "";
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_VERSION)) {
        version = paramMap[linglong::util::KEY_VERSION];
    }

    // 获取user env list
    QStringList userEnvList;
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_ENVLIST)) {
        userEnvList = paramMap[linglong::util::KEY_ENVLIST].split(",");
    }

    // 获取exec参数
    QString desktopExec;
    desktopExec.clear();
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_EXEC)) {
        desktopExec = paramMap[linglong::util::KEY_EXEC];
    }

    // 判断是否已安装
    QString err = "";
    if (!getAppInstalledStatus(packageId, version, "", "")) {
        err = packageId + " not installed";
        qCritical() << err;
        info->setcode(RetCode(RetCode::pkg_not_installed));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    JobManager::instance()->CreateJob([=]() {
        // 判断是否存在
        package::Ref ref("", packageId, version, hostArch());

        bool isFlatpakApp = !paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT);

        auto app = runtime::App::load(&d->repo, ref, desktopExec, isFlatpakApp);
        if (nullptr == app) {
            // FIXME: set job status to failed
            qCritical() << "nullptr" << app;
            return;
        }
        app->saveUserEnvList(userEnvList);
        app->setAppParamMap(paramMap);
        d->apps[app->container()->id] = QPointer<runtime::App>(app);
        app->start();
    });
    return retMsg;
}

/*
 * 停止应用
 *
 * @param containerId: 应用启动对应的容器Id
 *
 * @return RetMessageList 执行结果信息
 */
RetMessageList PackageManager::Stop(const QString &containerId)
{
    Q_D(PackageManager);

    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    QString err = "";
    if (!d->apps.contains(containerId)) {
        err = "containerId:" + containerId + " not exist";
        qCritical() << err;
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    pid_t pid = d->apps[containerId]->container()->pid;
    int ret = kill(pid, SIGKILL);
    if (ret == 0) {
        d->apps.remove(containerId);
    } else {
        err = "kill container failed, containerId:" + containerId;
        info->setcode(RetCode(RetCode::ErrorPkgKillFailed));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
    }
    qInfo() << "kill containerId:" << containerId << ",ret:" << ret;
    return retMsg;
}

ContainerList PackageManager::ListContainer()
{
    Q_D(PackageManager);
    ContainerList list;

    for (const auto &app : d->apps) {
        auto c = QPointer<Container>(new Container);
        c->id = app->container()->id;
        c->pid = app->container()->pid;
        list.push_back(c);
    }
    return list;
}
QString PackageManager::Status()
{
    return {"active"};
}
