// SPDX-FileCopyrightText: 2026 xiyidaiwa
// SPDX-License-Identifier: GPL-3.0-or-later

#include "resultbuilder.h"

#include <QJsonArray>
#include <QJsonDocument>

QJsonObject ResultBuilder::makeRoot(const QString &mID)
{
    QJsonObject root;
    root["ver"] = "1.0";
    root["mID"] = mID;
    root["cont"] = QJsonArray();
    return root;
}

void ResultBuilder::addGroup(QJsonObject &root, const QString &group,
                             const QList<Item> &items)
{
    if (items.isEmpty())
        return;

    QJsonArray contents = root["cont"].toArray();
    QJsonObject groupObj;
    groupObj["group"] = group;

    QJsonArray itemArr;
    for (const Item &it : items) {
        if (it.key.isEmpty() || it.name.isEmpty() || it.type.isEmpty())
            continue; // 协议要求三个字段非空，否则被跳过
        QJsonObject obj;
        obj["item"] = it.key;
        obj["name"] = it.name;
        if (!it.icon.isEmpty())
            obj["icon"] = it.icon;
        obj["type"] = it.type;
        itemArr.append(obj);
    }

    if (itemArr.isEmpty())
        return;

    groupObj["items"] = itemArr;
    contents.append(groupObj);
    root["cont"] = contents;
}

QString ResultBuilder::buildEmpty(const QString &mID)
{
    QJsonObject root = makeRoot(mID);
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

QString ResultBuilder::toJson(QJsonObject &root, const QString &mID)
{
    if (root.isEmpty())
        root = makeRoot(mID);
    else
        root["mID"] = mID;
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}
