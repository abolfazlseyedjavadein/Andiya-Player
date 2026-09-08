#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVector>

class QFileSystemWatcher;

// Lists the lossless PNG frames written by PlayerController::captureFrame()
// so the UI can show the latest capture with metadata and let the user
// delete individual screenshots or clear the whole folder. Also tolerates
// files being removed outside the app (Explorer, another process, etc.) by
// watching the capture directory and silently dropping missing entries the
// next time it is touched.
class ScreenshotListModel final : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString directory READ directory NOTIFY directoryChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool hasScreenshots READ hasScreenshots NOTIFY countChanged)
    Q_PROPERTY(QString latestPath READ latestPath NOTIFY latestChanged)
    Q_PROPERTY(QString latestUrl READ latestUrl NOTIFY latestChanged)
    Q_PROPERTY(QString latestFileName READ latestFileName NOTIFY latestChanged)
    Q_PROPERTY(QString latestDateTimeText READ latestDateTimeText NOTIFY latestChanged)
    Q_PROPERTY(QString latestFileSizeText READ latestFileSizeText NOTIFY latestChanged)
    Q_PROPERTY(QString latestDimensionsText READ latestDimensionsText NOTIFY latestChanged)

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileUrlRole,
        FileNameRole,
        DateTimeTextRole,
        FileSizeTextRole,
        DimensionsTextRole,
        TimestampRole,
    };

    explicit ScreenshotListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString directory() const;
    [[nodiscard]] int count() const;
    [[nodiscard]] bool hasScreenshots() const;
    [[nodiscard]] QString latestPath() const;
    [[nodiscard]] QString latestUrl() const;
    [[nodiscard]] QString latestFileName() const;
    [[nodiscard]] QString latestDateTimeText() const;
    [[nodiscard]] QString latestFileSizeText() const;
    [[nodiscard]] QString latestDimensionsText() const;

    Q_INVOKABLE void setDirectory(const QString &path);
    Q_INVOKABLE void refresh();
    // Hides the entry from Andiya's list only; the PNG stays on disk untouched.
    Q_INVOKABLE bool removeFromList(int index);
    // Permanently deletes the underlying file, then removes the entry.
    Q_INVOKABLE bool deletePermanently(int index);
    // Hides every currently listed entry without touching any files.
    Q_INVOKABLE void clearListOnly();
    // Permanently deletes every currently listed file from disk.
    Q_INVOKABLE void deleteAllPermanently();
    Q_INVOKABLE void revealInFolder(int index);
    Q_INVOKABLE void openExternally(int index);

signals:
    void directoryChanged();
    void countChanged();
    void latestChanged();
    void notification(const QString &title, const QString &message);

private:
    struct Entry {
        QString filePath;
        QString fileName;
        QDateTime capturedAt;
        qint64 fileSize = 0;
        QString dimensionsText;
    };

    void rescan();
    void watchDirectory();
    void loadHiddenPaths();
    void saveHiddenPaths() const;
    [[nodiscard]] static QString formatFileSize(qint64 bytes);
    [[nodiscard]] static QString normalizedPath(const QString &path);

    QFileSystemWatcher *m_watcher = nullptr;
    QString m_directory;
    QVector<Entry> m_entries;
    QSet<QString> m_hiddenPaths;
};
