/*
 Copyright (c) 2024 glaumar <glaumar@geekgo.tech>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "vrp_public.h"

#include <QCoroNetworkReply>
#include <QCoroSignal>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

VrpPublic::VrpPublic(QObject *parent)
    : QObject(parent)
    , base_url_("https://theapp.vrrookie.xyz/")
    , password_("gL59VfgPxoHR")
    , manager_(nullptr)
{
}

QCoro::Task<bool> VrpPublic::update()
{
    // TODO: save vrp-public.json to local file
    // TODO: load vrp-public.json from local file when update failed
    static const QVector<QString> urls = {"https://raw.githubusercontent.com/vrpyou/quest/main/vrp-public.json",
                                          "https://vrpirates.wiki/downloads/vrp-public.json"};

    for (auto url : urls) {
        qDebug() << "Downloading vrp-public.json from " << url;
        auto [success, data] = co_await downloadJson(url);
        if (!success) {
            continue;
        }

        auto [base_url, password] = parseJson(data);
        if (!base_url.isEmpty() && !password.isEmpty()) {
            base_url_ = base_url;
            password_ = password;
            co_return true;
        }
    }
}

QCoro::Task<QPair<bool, QByteArray>> VrpPublic::downloadJson(const QString url)
{
    QNetworkRequest request(url);
    auto *reply = manager_.get(request);
    reply->ignoreSslErrors();
    co_await qCoro(reply, &QNetworkReply::finished);

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Downloading json Error: " << reply->errorString();
        co_return QPair<bool, QByteArray>(false, QByteArray());
    }
    auto data = reply->readAll();
    delete reply;
    co_return QPair<bool, QByteArray>(true, data);
}

QPair<QString, QString> VrpPublic::parseJson(const QByteArray &json)
{
    /*
    vrp-public.json example:
    {
        "baseUri":"https://theapp.vrrookie.xyz/",
        "password":"Z0w1OVZmZ1B4b0hS"
    }
    */

    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull()) {
        qDebug() << "Parsing vrp-public.json Error: invalid json: " << json;

        return QPair<QString, QString>("", "");
    }

    QJsonObject obj = doc.object();
    if (obj.isEmpty()) {
        qDebug() << "Parsing vrp-public.json Error: empty json: " << json;
        return QPair<QString, QString>("", "");
    }

    QString base_url = obj["baseUri"].toString();
    // password is base64 encoded
    QString password = QString(QByteArray::fromBase64(obj["password"].toString().toUtf8()));

    qDebug() << "Parsed vrp-public.json: baseUri=" << base_url << ", password=" << password;
    return QPair<QString, QString>(base_url, password);
}
