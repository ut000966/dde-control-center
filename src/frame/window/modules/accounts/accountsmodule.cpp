/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     liuhong <liuhong_cm@deepin.com>
 *
 * Maintainer: liuhong <liuhong_cm@deepin.com>
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

#include "accountsmodule.h"
#include "createaccountpage.h"
#include "modifypasswdpage.h"
#include "accountswidget.h"
#include "accountsdetailwidget.h"
#include "modules/accounts/accountsworker.h"
#include "modules/accounts/user.h"
#include "modules/accounts/usermodel.h"
#include "modules/accounts/fingerworker.h"
#include "modules/accounts/fingermodel.h"
#include "addfingedialog.h"

#include <QStringList>
#include <QTimer>
#include <QDebug>

using namespace dcc::accounts;
using namespace DCC_NAMESPACE::accounts;

AccountsModule::AccountsModule(FrameProxyInterface *frame, QObject *parent)
    : QObject(parent)
    , ModuleInterface(frame)
{
    m_frameProxy =  frame;
}

void AccountsModule::initialize()
{
    m_userList = new UserModel(this);
    m_accountsWorker = new AccountsWorker(m_userList);

    m_accountsWorker->moveToThread(qApp->thread());
    m_userList->moveToThread(qApp->thread());

    m_fingerModel = new FingerModel(this);
    m_fingerWorker = new FingerWorker(m_fingerModel);

    m_fingerModel->moveToThread(qApp->thread());
    m_fingerWorker->moveToThread(qApp->thread());

    m_accountsWorker->active();
    m_fingerWorker->refreshDevice();
    connect(m_fingerWorker, &FingerWorker::requestShowAddThumb, this, &AccountsModule::onShowAddThumb);
    connect(m_fingerModel, &FingerModel::vaildChanged, [ = ](const bool isVaild) {
        if (isVaild) {
            initFingerData();
        } else {
            for (const auto &user : m_userList->userList()) {
                disconnect(user, &User::nameChanged, m_fingerWorker, &FingerWorker::refreshUserEnrollList);
            }
        }
    });
}

void AccountsModule::reset()
{

}

const QString AccountsModule::name() const
{
    return QStringLiteral("accounts");
}

void AccountsModule::showPage(const QString &pageName)
{
    Q_UNUSED(pageName);
}

void AccountsModule::contentPopped(QWidget *const w)
{
    Q_UNUSED(w)
}

void AccountsModule::active()
{
    m_accountsWidget = new AccountsWidget;
    m_accountsWidget->setModel(m_userList);
    m_accountsWidget->setShowFirstUserInfo(true);
    connect(m_accountsWidget, &AccountsWidget::requestShowAccountsDetail, this, &AccountsModule::onShowAccountsDetailWidget);
    connect(m_accountsWidget, &AccountsWidget::requestCreateAccount, this, &AccountsModule::onShowCreateAccountPage);
    m_frameProxy->pushWidget(this, m_accountsWidget);
}

void AccountsModule::load(QString path)
{
    if (!m_accountsWidget) {
        active();
    }

    QStringList searchList;
    searchList << "Change Password"
               << "Delete Account"
               << "Auto Login"
               << "Login Without Password"
               << "New Account";

    User *pUser = nullptr;
    for (auto &user : m_userList->userList()) {
        if (user->isCurrentUser()) {
            pUser = user;
            break;
        }
    }

    if (pUser == nullptr) {
        return;
    }

    if (path == searchList[0] || path == searchList[1] || path == searchList[2] || path == searchList[3]) {
        onShowAccountsDetailWidget(pUser);
    } else if (path == searchList[4]) {
        onShowCreateAccountPage();
    }
}

//显示账户信息
void AccountsModule::onShowAccountsDetailWidget(User *account)
{
    AccountsDetailWidget *w = new AccountsDetailWidget(account);
    w->setFingerModel(m_fingerModel);

    if (m_fingerModel->isVaild()) {
        initFingerData();
    }

    connect(w, &AccountsDetailWidget::requestShowPwdSettings, this, &AccountsModule::onShowPasswordPage);
    connect(w, &AccountsDetailWidget::requestSetAutoLogin, m_accountsWorker, &AccountsWorker::setAutoLogin);
    connect(w, &AccountsDetailWidget::requestNopasswdLogin, m_accountsWorker, &AccountsWorker::setNopasswdLogin);
    connect(w, &AccountsDetailWidget::requestDeleteAccount, m_accountsWorker, &AccountsWorker::deleteAccount);
    connect(w, &AccountsDetailWidget::requestBack, this, [&]() {
        m_frameProxy->popWidget(this);
        m_accountsWidget->setShowFirstUserInfo(false);
    });
    connect(w, &AccountsDetailWidget::requestDeleteAvatar, m_accountsWorker, &AccountsWorker::deleteUserIcon);
    connect(w, &AccountsDetailWidget::requestAddNewAvatar, m_accountsWorker, &AccountsWorker::addNewAvatar);
    connect(w, &AccountsDetailWidget::requestSetAvatar, m_accountsWorker, &AccountsWorker::setAvatar);
    connect(w, &AccountsDetailWidget::requestShowFullnameSettings, m_accountsWorker, &AccountsWorker::setFullname);
    connect(w, &AccountsDetailWidget::requestAddThumbs, m_fingerWorker, &FingerWorker::enrollStart);
    connect(w, &AccountsDetailWidget::requestCleanThumbs, m_fingerWorker, &FingerWorker::cleanEnroll);
    m_frameProxy->pushWidget(this, w);
}

//创建账户界面
void AccountsModule::onShowCreateAccountPage()
{
    CreateAccountPage *w = new CreateAccountPage();
    User *newUser = new User(this);
    w->setModel(newUser);
    connect(w, &CreateAccountPage::requestCreateUser, m_accountsWorker, &AccountsWorker::createAccount);
    connect(m_accountsWorker, &AccountsWorker::accountCreationFinished, w, &CreateAccountPage::setCreationResult);
    connect(w, &CreateAccountPage::requestBack, this, [&]() {
        m_frameProxy->popWidget(this);
        m_accountsWidget->setShowFirstUserInfo(false);
    });
    m_frameProxy->pushWidget(this, w);
}

AccountsModule::~AccountsModule()
{
    m_userList->deleteLater();
    m_accountsWorker->deleteLater();
}

//修改密码界面
void AccountsModule::onShowPasswordPage(User *account)
{
    ModifyPasswdPage *w = new ModifyPasswdPage(account);
    connect(w, &ModifyPasswdPage::requestChangePassword, m_accountsWorker, &AccountsWorker::setPassword);
    connect(w, &ModifyPasswdPage::requestBack, this, [&]() {
        m_frameProxy->popWidget(this);
    });
    m_frameProxy->pushWidget(this, w);
}

//添加指纹界面
void AccountsModule::onShowAddThumb(const QString &name, const QString &thumb)
{
    AddFingeDialog *dlg = new AddFingeDialog(thumb);
    dlg->setFingerModel(m_fingerModel);
    dlg->setUsername(name);

    connect(dlg, &AddFingeDialog::requestSaveThumb, m_fingerWorker, &FingerWorker::saveEnroll);
    connect(dlg, &AddFingeDialog::requestReEnrollStart, m_fingerWorker, &FingerWorker::reEnrollStart);
    connect(dlg, &AddFingeDialog::requestStopEnroll, m_fingerWorker, &FingerWorker::stopEnroll);

    dlg->exec();//Note:destroy this object when this window is closed
}

void AccountsModule::initFingerData()
{
    connect(m_userList, &UserModel::userAdded, this, [ & ](User * user) {
        connect(user, &User::nameChanged, m_fingerWorker, &FingerWorker::refreshUserEnrollList);
    });
    for (const auto &user : m_userList->userList()) {
        connect(user, &User::nameChanged, m_fingerWorker, &FingerWorker::refreshUserEnrollList);
    }
}
