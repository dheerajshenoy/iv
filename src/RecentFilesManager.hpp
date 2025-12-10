// Write a C++ header file for a RecentFilesManager class that manages a list of recently opened files.
// using JSON for serialization and deserialization. The class should provide methods to add a file to the recent list,
// retrieve the list of recent files, and save/load the list to/from a JSON file, and delete a file from the recent
// list.

#pragma once

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QStandardPaths>
#include <QString>
#include <qiodevicebase.h>
#include <QMenu>
#include <QAction>
#include <QObject>

class RecentFilesManager
{
public:
    RecentFilesManager(const QString &recentFilesPath, int limit) noexcept
        : m_recentFilesPath(recentFilesPath), m_recent_files_limit(limit)
    {
        loadRecentFiles();
    }

    void addFilePath(const QString &filepath) noexcept
    {
        recentFiles.removeAll(filepath);
        recentFiles.prepend(filepath);
        if (recentFiles.size() > m_recent_files_limit)
        {
            recentFiles = recentFiles.mid(0, m_recent_files_limit);
        }
        saveRecentFiles();
    }

    QList<QString> getRecentFiles() const noexcept
    {
        return recentFiles;
    }

    void removeFilePath(const QString &filepath) noexcept
    {
        recentFiles.removeAll(filepath);
        saveRecentFiles();
    }

    void PopulateRecentFilesMenu(QMenu *menu, std::function<void(const QString &)> openFileCallback) noexcept
    {
        menu->clear();
        for (const QString &filepath : recentFiles)
        {
            QAction *action = new QAction(filepath, menu);
            QObject::connect(action, &QAction::triggered, [=]() { openFileCallback(filepath); });
            menu->addAction(action);
        }
    }

private:
    QList<QString> recentFiles;
    QString m_recentFilesPath;
    int m_recent_files_limit = 10;

    void loadRecentFiles() noexcept
    {
        QFile file(m_recentFilesPath);
        if (!file.exists())
        {
            // create an empty recent files (recent_files.json) list if the file does not exist yet
            bool s = file.open(QIODevice::WriteOnly);
            if (s)
                file.write("");
            return;
        }
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Could not open recent files list:" << m_recentFilesPath;
            return;
        }

        QByteArray data   = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isArray())
        {
            qWarning() << "Invalid recent files JSON format.";
            return;
        }

        QJsonArray jsonArray = doc.array();
        for (const QJsonValue &value : jsonArray)
        {
            recentFiles.append(value.toString());
        }
    }

    void saveRecentFiles() noexcept
    {
        QJsonArray jsonArray;
        for (const QString &filepath : recentFiles)
        {
            jsonArray.append(filepath);
        }

        QJsonDocument doc(jsonArray);
        QFile file(m_recentFilesPath);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "Could not open recent files list for writing:" << m_recentFilesPath;
            return;
        }

        file.write(doc.toJson());
    }

};
