#include "ScreenshotListModel.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QLocale>
#include <QSettings>
#include <QUrl>

#include <algorithm>

namespace {
constexpr const char *kScreenshotFilter = "*.png";
constexpr const char *kHiddenPathsKey = "screenshots/hiddenPaths";
}

ScreenshotListModel::ScreenshotListModel(QObject *parent)
    : QAbstractListModel(parent),
      m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotListModel::rescan);
    loadHiddenPaths();
}

int ScreenshotListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ScreenshotListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case FilePathRole: return entry.filePath;
    case FileUrlRole: return QUrl::fromLocalFile(entry.filePath).toString();
    case FileNameRole: return entry.fileName;
    case DateTimeTextRole: return QLocale().toString(entry.capturedAt, QLocale::ShortFormat);
    case FileSizeTextRole: return formatFileSize(entry.fileSize);
    case DimensionsTextRole: return entry.dimensionsText;
    case TimestampRole: return entry.capturedAt.toMSecsSinceEpoch();
    default: return {};
    }
}

QHash<int, QByteArray> ScreenshotListModel::roleNames() const
{
    return {
        {FilePathRole, "filePath"},
        {FileUrlRole, "fileUrl"},
        {FileNameRole, "fileName"},
        {DateTimeTextRole, "dateTimeText"},
        {FileSizeTextRole, "fileSizeText"},
        {DimensionsTextRole, "dimensionsText"},
        {TimestampRole, "timestamp"},
    };
}

QString ScreenshotListModel::directory() const { return m_directory; }
int ScreenshotListModel::count() const { return m_entries.size(); }
bool ScreenshotListModel::hasScreenshots() const { return !m_entries.isEmpty(); }

QString ScreenshotListModel::latestPath() const
{
    return m_entries.isEmpty() ? QString{} : m_entries.first().filePath;
}

QString ScreenshotListModel::latestUrl() const
{
    return m_entries.isEmpty() ? QString{} : QUrl::fromLocalFile(m_entries.first().filePath).toString();
}

QString ScreenshotListModel::latestFileName() const
{
    return m_entries.isEmpty() ? QString{} : m_entries.first().fileName;
}

QString ScreenshotListModel::latestDateTimeText() const
{
    return m_entries.isEmpty() ? QString{} : QLocale().toString(m_entries.first().capturedAt, QLocale::ShortFormat);
}

QString ScreenshotListModel::latestFileSizeText() const
{
    return m_entries.isEmpty() ? QString{} : formatFileSize(m_entries.first().fileSize);
}

QString ScreenshotListModel::latestDimensionsText() const
{
    return m_entries.isEmpty() ? QString{} : m_entries.first().dimensionsText;
}

void ScreenshotListModel::setDirectory(const QString &path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned == m_directory) {
        rescan();
        return;
    }
    if (!m_directory.isEmpty() && m_watcher->directories().contains(m_directory)) {
        m_watcher->removePath(m_directory);
    }
    m_directory = cleaned;
    emit directoryChanged();
    watchDirectory();
    rescan();
}

void ScreenshotListModel::refresh()
{
    watchDirectory();
    rescan();
}

void ScreenshotListModel::watchDirectory()
{
    if (m_directory.isEmpty() || m_watcher->directories().contains(m_directory)) {
        return;
    }
    if (QFileInfo::exists(m_directory)) {
        m_watcher->addPath(m_directory);
    }
}

void ScreenshotListModel::rescan()
{
    watchDirectory();

    QVector<Entry> entries;
    if (!m_directory.isEmpty()) {
        QDir dir(m_directory);
        const QFileInfoList files = dir.entryInfoList({QString::fromLatin1(kScreenshotFilter)},
                                                       QDir::Files, QDir::Time);
        entries.reserve(files.size());
        for (const QFileInfo &info : files) {
            if (m_hiddenPaths.contains(normalizedPath(info.absoluteFilePath()))) {
                continue;
            }
            Entry entry;
            entry.filePath = QDir::toNativeSeparators(info.absoluteFilePath());
            entry.fileName = info.fileName();
            entry.capturedAt = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
            entry.fileSize = info.size();
            const QImageReader reader(info.absoluteFilePath());
            const QSize size = reader.size();
            entry.dimensionsText = size.isValid()
                                        ? QStringLiteral("%1 \u00D7 %2").arg(size.width()).arg(size.height())
                                        : QStringLiteral("Unknown size");
            entries.append(entry);
        }
        std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
            return a.capturedAt > b.capturedAt;
        });
    }

    const bool hadEntries = !m_entries.isEmpty();
    const QString previousLatest = latestPath();

    beginResetModel();
    m_entries = entries;
    endResetModel();

    emit countChanged();
    if (previousLatest != latestPath() || hadEntries != !m_entries.isEmpty()) {
        emit latestChanged();
    }
}

bool ScreenshotListModel::removeFromList(int index)
{
    if (index < 0 || index >= m_entries.size()) {
        return false;
    }

    const Entry entry = m_entries.at(index);
    m_hiddenPaths.insert(normalizedPath(entry.filePath));
    saveHiddenPaths();

    beginRemoveRows(QModelIndex{}, index, index);
    m_entries.removeAt(index);
    endRemoveRows();

    emit countChanged();
    emit latestChanged();
    emit notification(QStringLiteral("Removed from list"),
                      QStringLiteral("%1 was hidden from Andiya but kept on disk.").arg(entry.fileName));
    return true;
}

bool ScreenshotListModel::deletePermanently(int index)
{
    if (index < 0 || index >= m_entries.size()) {
        return false;
    }

    const Entry entry = m_entries.at(index);
    const bool removedFromDisk = !QFileInfo::exists(entry.filePath) || QFile::remove(entry.filePath);
    if (!removedFromDisk) {
        emit notification(QStringLiteral("Could not delete screenshot"),
                          QStringLiteral("%1 may be open in another program.").arg(entry.fileName));
        return false;
    }

    m_hiddenPaths.remove(normalizedPath(entry.filePath));

    beginRemoveRows(QModelIndex{}, index, index);
    m_entries.removeAt(index);
    endRemoveRows();

    emit countChanged();
    emit latestChanged();
    return true;
}

void ScreenshotListModel::clearListOnly()
{
    if (m_entries.isEmpty()) {
        return;
    }

    for (const Entry &entry : std::as_const(m_entries)) {
        m_hiddenPaths.insert(normalizedPath(entry.filePath));
    }
    saveHiddenPaths();

    const int removedCount = m_entries.size();
    beginResetModel();
    m_entries.clear();
    endResetModel();

    emit countChanged();
    emit latestChanged();
    emit notification(QStringLiteral("List cleared"),
                      QStringLiteral("%1 screenshot(s) hidden from Andiya but kept on disk.").arg(removedCount));
}

void ScreenshotListModel::deleteAllPermanently()
{
    if (m_entries.isEmpty()) {
        return;
    }

    int failures = 0;
    for (const Entry &entry : std::as_const(m_entries)) {
        if (QFileInfo::exists(entry.filePath) && !QFile::remove(entry.filePath)) {
            ++failures;
        } else {
            m_hiddenPaths.remove(normalizedPath(entry.filePath));
        }
    }

    beginResetModel();
    m_entries.clear();
    endResetModel();

    emit countChanged();
    emit latestChanged();

    if (failures > 0) {
        emit notification(QStringLiteral("Some screenshots were not deleted"),
                          QStringLiteral("%1 file(s) could not be removed. They may be open elsewhere.")
                              .arg(failures));
    } else {
        emit notification(QStringLiteral("Screenshots deleted"),
                          QStringLiteral("All captured frames were permanently removed."));
    }
    rescan();
}

void ScreenshotListModel::revealInFolder(int index)
{
    if (index < 0 || index >= m_entries.size()) {
        if (!m_directory.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_directory));
        }
        return;
    }
    const QFileInfo info(m_entries.at(index).filePath);
    if (!info.exists()) {
        emit notification(QStringLiteral("File not found"),
                          QStringLiteral("%1 was deleted outside Andiya.").arg(info.fileName()));
        rescan();
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
}

void ScreenshotListModel::openExternally(int index)
{
    if (index < 0 || index >= m_entries.size()) {
        return;
    }
    const QString path = m_entries.at(index).filePath;
    if (!QFileInfo::exists(path)) {
        emit notification(QStringLiteral("File not found"),
                          QStringLiteral("This screenshot was deleted outside Andiya."));
        rescan();
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QString ScreenshotListModel::formatFileSize(qint64 bytes)
{
    return QLocale().formattedDataSize(bytes);
}

QString ScreenshotListModel::normalizedPath(const QString &path)
{
    return QDir::cleanPath(path);
}

void ScreenshotListModel::loadHiddenPaths()
{
    const QStringList stored = QSettings().value(QString::fromLatin1(kHiddenPathsKey)).toStringList();
    m_hiddenPaths = QSet<QString>(stored.begin(), stored.end());
}

void ScreenshotListModel::saveHiddenPaths() const
{
    QSettings().setValue(QString::fromLatin1(kHiddenPathsKey),
                         QStringList(m_hiddenPaths.begin(), m_hiddenPaths.end()));
}
