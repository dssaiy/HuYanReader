#ifndef CHAPTERDOWNLOADER_H
#define CHAPTERDOWNLOADER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QFuture>
#include <QFutureWatcher>
#include "NovelModels.h"

class HttpClient;
class ContentParser;
class BookSource;

/**
 * @brief Chapter download task status
 */
enum class DownloadStatus {
    Pending,    // Waiting for download
    Downloading, // Currently downloading
    Completed,  // Download completed
    Failed,     // Download failed
    Paused,     // Download paused
    Cancelled   // Download cancelled
};

/**
 * @brief Chapter download task
 */
struct DownloadTask {
    QString taskId;           // Unique task ID
    Chapter chapter;          // Chapter information
    BookSource bookSource;    // Book source information
    DownloadStatus status;    // Download status
    QString content;          // Downloaded content
    QString error;            // Error message
    int retryCount;           // Retry count
    qint64 downloadTime;      // Download time (milliseconds)
    
    DownloadTask() : status(DownloadStatus::Pending), retryCount(0), downloadTime(0) {}
};

/**
 * @brief Download configuration
 */
struct DownloadConfig {
    int maxConcurrent = 2;        // Maximum concurrent downloads (conservative start)
    int requestInterval = 1000;   // Request interval (milliseconds)
    int timeout = 15000;          // Timeout duration (milliseconds)
    int maxRetries = 2;           // Maximum retry attempts
    bool enableSmartInterval = true; // Enable smart interval adjustment
    bool enableProgressCallback = true; // Enable progress callback
    QString downloadPath;         // Download path
    bool saveToFile = false;      // Whether to save to file
    bool saveIndividualChapters = false; // Save each chapter as individual file
    QString chapterSaveDir;       // Directory for individual chapter files
    bool enableAutoMerge = true;  // Auto merge chapter files after download
};

/**
 * @brief Download statistics information
 */
struct DownloadStats {
    int totalTasks = 0;           // Total number of tasks
    int completedTasks = 0;       // Number of completed tasks
    int failedTasks = 0;          // Number of failed tasks
    int pendingTasks = 0;         // Number of pending tasks
    qint64 totalDownloadTime = 0; // Total download time
    qint64 totalContentSize = 0;  // Total content size
    double averageSpeed = 0.0;    // Average download speed
    
    double getProgress() const {
        return totalTasks > 0 ? (double)completedTasks / totalTasks * 100.0 : 0.0;
    }
};

/**
 * @brief Chapter download manager
 *
 * Provides multi-threaded concurrent downloading, progress tracking, pause/resume functionality
 */
class ChapterDownloader : public QObject
{
    Q_OBJECT

public:
    explicit ChapterDownloader(QObject *parent = nullptr);
    ~ChapterDownloader();

    // Set dependency components
    void setHttpClient(HttpClient *httpClient);
    void setContentParser(ContentParser *parser);

    // Download configuration
    void setDownloadConfig(const DownloadConfig &config);
    DownloadConfig getDownloadConfig() const { return m_config; }

    // Task management
    QString addDownloadTask(const Chapter &chapter, const BookSource &bookSource);
    QStringList addDownloadTasks(const QList<Chapter> &chapters, const BookSource &bookSource);
    bool removeDownloadTask(const QString &taskId);
    void clearAllTasks();

    // Download control
    void startDownload();
    void pauseDownload();
    void resumeDownload();
    void stopDownload();
    bool isDownloading() const { return m_isDownloading; }
    bool isPaused() const { return m_isPaused; }

    // Task query
    DownloadTask getDownloadTask(const QString &taskId) const;
    QList<DownloadTask> getAllTasks() const;
    QList<DownloadTask> getTasksByStatus(DownloadStatus status) const;
    DownloadStats getDownloadStats() const;

    // Result retrieval
    QList<DownloadTask> getCompletedTasks() const;
    QStringList getDownloadedContent() const;
    bool saveTasksToFile(const QString &filePath) const;

signals:
    /**
     * @brief Download started signal
     * @param totalTasks Total number of tasks
     */
    void downloadStarted(int totalTasks);

    /**
     * @brief Download progress signal
     * @param completed Number of completed tasks
     * @param total Total number of tasks
     * @param currentTask Current task information
     */
    void downloadProgress(int completed, int total, const QString &currentTask);

    /**
     * @brief Single task completed signal
     * @param taskId Task ID
     * @param task Task information
     */
    void taskCompleted(const QString &taskId, const DownloadTask &task);

    /**
     * @brief Single task failed signal
     * @param taskId Task ID
     * @param error Error message
     */
    void taskFailed(const QString &taskId, const QString &error);

    /**
     * @brief Download finished signal
     * @param stats Download statistics information
     */
    void downloadFinished(const DownloadStats &stats);

    /**
     * @brief Download paused signal
     */
    void downloadPaused();

    /**
     * @brief Download resumed signal
     */
    void downloadResumed();

    /**
     * @brief Download error signal
     * @param error Error message
     */
    void downloadError(const QString &error);

    /**
     * @brief Debug message signal
     * @param message Debug message
     */
    void debugMessage(const QString &message);

private slots:
    void onTaskCompleted(const QString &taskId, const DownloadTask &task);
    void onTaskFailed(const QString &taskId, const QString &error);
    void onIntervalTimer();
    void processNextTask();

private:
    // Internal methods
    void initializeDownloader();
    QString generateTaskId() const;
    void updateStats();
    void scheduleNextTask();
    void adjustRequestInterval();
    DownloadTask* findTask(const QString &taskId);
    void setError(const QString &error);
    void emitDebugMessage(const QString &message);
    void executeSyncDownload(const DownloadTask &task);

    // Special book source handling
    void applyBookSourceConfig(const BookSource &bookSource);
    int calculateRequestInterval(const BookSource &bookSource);
    bool isSpecialBookSource(int sourceId);

    // Pagination support
    QString downloadPaginatedChapterContent(const QString &firstPageHtml, const ChapterRule &rule, const QString &baseUrl);

    // Member variables
    HttpClient *m_httpClient;
    ContentParser *m_contentParser;
    QThreadPool *m_threadPool;

    // Task management
    QQueue<DownloadTask> m_taskQueue;
    QList<DownloadTask> m_allTasks;
    QMutex m_taskMutex;

    // Download control
    DownloadConfig m_config;
    bool m_isDownloading;
    bool m_isPaused;
    int m_activeDownloads;
    QString m_lastError;

    // Timers and statistics
    QTimer *m_intervalTimer;
    QElapsedTimer m_downloadTimer;
    DownloadStats m_stats;

    // Smart interval adjustment
    QList<qint64> m_recentDownloadTimes;
    int m_currentInterval;
    int m_consecutiveFailures;
};

/**
 * @brief Thread-safe chapter download worker
 */
class ThreadSafeDownloadWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ThreadSafeDownloadWorker(const DownloadTask &task,
                            const DownloadConfig &config,
                            const BookSource &bookSource);

    void run() override;

signals:
    void taskCompleted(const QString &taskId, const DownloadTask &task);
    void taskFailed(const QString &taskId, const QString &error);

private:
    DownloadTask m_task;
    DownloadConfig m_config;
    BookSource m_bookSource;

    // Thread-local components (created in run())
    QString downloadChapterContent(const QString &url);
    QString parseChapterContent(const QString &html);
    void saveChapterToFile(const Chapter &chapter, const QString &content);
};

#endif // CHAPTERDOWNLOADER_H
