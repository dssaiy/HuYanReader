#ifndef NOVELSEARCHMANAGER_H
#define NOVELSEARCHMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QMetaType>
#include <QSet>
#include "NovelModels.h"
#include "ChapterDownloader.h"

// Forward declarations
class NovelSearcher;
class ChapterDownloader;
class FileGenerator;
class RuleManager;
class HttpClient;
class ContentParser;
class Settings;

/**
 * @brief Novel search manager class
 *
 * Manages novel search, download, and file generation core logic
 * Similar to TextDocumentManager design pattern
 */
class NovelSearchManager : public QObject
{
    Q_OBJECT

public:
    explicit NovelSearchManager(Settings* settings, QObject *parent = nullptr);
    ~NovelSearchManager();

    // Link view and manager (deprecated methods removed)

    // Set novel configuration
    void setNovelConfig(class NovelConfig* config);

    // Get available book sources
    QList<BookSource> getAvailableSources() const;

    // Search functions
    void startSearch(const QString &keyword, int sourceId = -1);
    void cancelSearch();

    // Download functions
    void startDownload(const SearchResult &result, int startChapter, int endChapter, int mode, const QString &customPath);
    void cancelDownload();

    // Status query
    bool isSearching() const { return m_isSearching; }
    bool isDownloading() const { return m_isDownloading; }

signals:
    void searchStarted(const QString &keyword);
    void searchProgress(const QString &status, int current, int total);
    void searchCompleted(const QList<SearchResult> &results);
    void searchFailed(const QString &error);
    void searchResultsUpdated(const QList<SearchResult> &results, int sourceId);

    void downloadStarted(const SearchResult &result);
    void downloadProgress(const QString &status, int current, int total);
    void downloadCompleted(const QString &filePath);
    void downloadFailed(const QString &error);

private slots:
    void onSearchCompleted(const QList<SearchResult> &results, int sourceId);
    void onSearchFailed(const QString &error, int sourceId);
    void onSequentialSearchFailed(const QString &error, int sourceId);
    void onSearchTimeout();
    void onSearchTimeout(int sourceId);
    
    void onDownloadProgress(int current, int total);
    void onDownloadTaskCompleted(const QString &taskId, const QString &content);
    void onDownloadTaskFailed(const QString &taskId, const QString &error);

    // Real download handling slots
    void onRealChapterDownloaded(const QString &taskId, const DownloadTask &task);
    void onAllChaptersDownloaded(const DownloadStats &stats);
    void onDownloadError(const QString &error);

    void onFileGenerated(const QString &filePath);
    void onFileGenerationFailed(const QString &error);

private:
    void setupComponents();
    void setupConnections();
    void loadBookSources();
    void startSingleSourceSearch(const QString &keyword, int sourceId);
    void startMultiSourceSearch(const QString &keyword);
    void startConcurrentSearch(const QString &keyword);  // Backup concurrent search method
    void startSequentialSearch(const QString &keyword);
    void searchNextSource();
    void onSequentialSearchCompleted(const QList<SearchResult> &results, int sourceId);
    void generateFile();
    void resetSearchState();
    void resetDownloadState();

    // Special book source error handling
    bool isSpecialBookSourceError(int sourceId, const QString &error);
    bool handleSpecialBookSourceError(int sourceId, const QString &error);
    void adjustDownloadStrategyForSource(int sourceId);

    // Core components
    Settings* m_settings;
    class NovelConfig* m_novelConfig;
    
    HttpClient* m_httpClient;
    RuleManager* m_ruleManager;
    ContentParser* m_parser;
    NovelSearcher* m_searcher;
    ChapterDownloader* m_downloader;
    FileGenerator* m_generator;
    
    // Search state
    bool m_isSearching;
    QString m_currentKeyword;
    QList<BookSource> m_availableSources;
    QHash<int, QList<SearchResult>> m_searchResultsBySource;
    QSet<int> m_completedSources;
    QHash<int, QTimer*> m_sourceTimeoutTimers;
    int m_totalSourcesSearching;
    QTimer* m_searchTimeoutTimer;

    // Sequential search state
    QList<BookSource> m_searchQueue;
    int m_currentSearchIndex;
    QList<SearchResult> m_accumulatedResults;
    QTimer* m_sequentialTimer;
    
    // Download state
    bool m_isDownloading;
    SearchResult m_currentResult;
    const BookSource* m_currentBookSource;
    int m_totalChapters;
    int m_downloadedChapters;
    QHash<QString, QString> m_downloadedContent;
    int m_specialSourceRetryCount;
    
    // Thread safety
    QMutex m_searchMutex;
    QMutex m_downloadMutex;
};

// Meta types are already declared in NovelModels.h

#endif // NOVELSEARCHMANAGER_H
