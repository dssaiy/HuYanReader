#include "ChapterDownloader.h"
#include "../network/HttpClient.h"
#include "../parser/ContentParser.h"
#include <QDebug>
#include <QUuid>
#include <QThread>
#include <QEventLoop>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QMutexLocker>
#include <QTextCodec>

ChapterDownloader::ChapterDownloader(QObject* parent)
    : QObject(parent)
    , m_httpClient(nullptr)
    , m_contentParser(nullptr)
    , m_threadPool(new QThreadPool(this))
    , m_isDownloading(false)
    , m_isPaused(false)
    , m_activeDownloads(0)
    , m_currentInterval(1000)
    , m_consecutiveFailures(0)
    , m_config()  // Explicitly initialize with default values
{
    // Register DownloadTask for cross-thread signal/slot communication
    qRegisterMetaType<DownloadTask>("DownloadTask");

    emitDebugMessage(QString("ChapterDownloader constructor: default maxConcurrent = %1").arg(m_config.maxConcurrent));
    initializeDownloader();
}

ChapterDownloader::~ChapterDownloader()
{
    stopDownload();
    if (m_threadPool) {
        m_threadPool->waitForDone(5000);
    }
}

void ChapterDownloader::initializeDownloader()
{
    m_intervalTimer = new QTimer(this);
    m_intervalTimer->setSingleShot(true);
    connect(m_intervalTimer, &QTimer::timeout, this, &ChapterDownloader::processNextTask);

    if (m_threadPool) {
        m_threadPool->setMaxThreadCount(m_config.maxConcurrent);
    }

    emitDebugMessage("ChapterDownloader initialization completed");
}

void ChapterDownloader::setHttpClient(HttpClient* httpClient)
{
    m_httpClient = httpClient;
    emitDebugMessage("HttpClient set");
}

void ChapterDownloader::setContentParser(ContentParser* parser)
{
    m_contentParser = parser;
    emitDebugMessage("ContentParser set");
}

void ChapterDownloader::setDownloadConfig(const DownloadConfig& config)
{
    emitDebugMessage(QString("=== setDownloadConfig called ==="));
    emitDebugMessage(QString("Old config: concurrent=%1").arg(m_config.maxConcurrent));
    emitDebugMessage(QString("New config: concurrent=%1").arg(config.maxConcurrent));

    m_config = config;
    if (m_threadPool) {
        m_threadPool->setMaxThreadCount(config.maxConcurrent);
        emitDebugMessage(QString("ThreadPool maxThreadCount set to: %1").arg(config.maxConcurrent));
    }
    m_currentInterval = config.requestInterval;

    emitDebugMessage(QString("Download config updated: concurrent=%1, interval=%2ms")
        .arg(config.maxConcurrent)
        .arg(config.requestInterval));
}

QString ChapterDownloader::addDownloadTask(const Chapter& chapter, const BookSource& bookSource)
{
    QMutexLocker locker(&m_taskMutex);

    DownloadTask task;
    task.taskId = generateTaskId();
    task.chapter = chapter;
    task.bookSource = bookSource;
    task.status = DownloadStatus::Pending;
    task.retryCount = 0;
    task.downloadTime = 0;

    // Apply book source specific configuration
    applyBookSourceConfig(bookSource);

    m_taskQueue.enqueue(task);
    m_allTasks.append(task);

    updateStats();

    emitDebugMessage(QString("Added download task: %1 - %2")
        .arg(chapter.title())
        .arg(task.taskId));

    return task.taskId;
}

QStringList ChapterDownloader::addDownloadTasks(const QList<Chapter>& chapters, const BookSource& bookSource)
{
    QStringList taskIds;

    for (const Chapter& chapter : chapters) {
        QString taskId = addDownloadTask(chapter, bookSource);
        taskIds.append(taskId);
    }

    emitDebugMessage(QString("Batch added %1 download tasks").arg(chapters.size()));

    return taskIds;
}

bool ChapterDownloader::removeDownloadTask(const QString& taskId)
{
    QMutexLocker locker(&m_taskMutex);

    QQueue<DownloadTask> newQueue;
    bool found = false;

    while (!m_taskQueue.isEmpty()) {
        DownloadTask task = m_taskQueue.dequeue();
        if (task.taskId != taskId) {
            newQueue.enqueue(task);
        }
        else {
            found = true;
        }
    }
    m_taskQueue = newQueue;

    for (int i = 0; i < m_allTasks.size(); ++i) {
        if (m_allTasks[i].taskId == taskId) {
            m_allTasks.removeAt(i);
            found = true;
            break;
        }
    }

    if (found) {
        updateStats();
        emitDebugMessage(QString("Removed download task: %1").arg(taskId));
    }

    return found;
}

void ChapterDownloader::clearAllTasks()
{
    QMutexLocker locker(&m_taskMutex);

    m_taskQueue.clear();
    m_allTasks.clear();
    updateStats();

    emitDebugMessage("Cleared all download tasks");
}

void ChapterDownloader::startDownload()
{
    if (m_isDownloading) {
        emitDebugMessage("Download already in progress");
        return;
    }

    if (!m_httpClient || !m_contentParser) {
        setError("HttpClient or ContentParser not set");
        return;
    }

    if (m_taskQueue.isEmpty()) {
        emitDebugMessage("No tasks to download");
        return;
    }

    m_isDownloading = true;
    m_isPaused = false;
    m_activeDownloads = 0;
    m_consecutiveFailures = 0;
    m_downloadTimer.start();

    updateStats();
    emit downloadStarted(m_stats.totalTasks);

    emitDebugMessage(QString("Started download, total %1 tasks").arg(m_stats.totalTasks));

    int tasksToStart = qMin(m_config.maxConcurrent, m_taskQueue.size());
    for (int i = 0; i < tasksToStart; ++i) {
        processNextTask();
    }
}

void ChapterDownloader::pauseDownload()
{
    if (!m_isDownloading || m_isPaused) {
        return;
    }

    m_isPaused = true;
    if (m_intervalTimer) {
        m_intervalTimer->stop();
    }

    emit downloadPaused();
    emitDebugMessage("Download paused");
}

void ChapterDownloader::resumeDownload()
{
    if (!m_isDownloading || !m_isPaused) {
        return;
    }

    m_isPaused = false;

    emit downloadResumed();
    emitDebugMessage("Download resumed");

    if (!m_taskQueue.isEmpty() && m_activeDownloads < m_config.maxConcurrent) {
        scheduleNextTask();
    }
}

void ChapterDownloader::stopDownload()
{
    if (!m_isDownloading) {
        return;
    }

    m_isDownloading = false;
    m_isPaused = false;

    if (m_intervalTimer) {
        m_intervalTimer->stop();
    }

    if (m_threadPool) {
        m_threadPool->waitForDone(3000);
    }

    updateStats();

    qDebug() << "=== ChapterDownloader::stopDownload ===";
    qDebug() << "Completed tasks:" << m_stats.completedTasks;
    qDebug() << "Total tasks:" << m_stats.totalTasks;
    qDebug() << "Emitting downloadFinished signal...";

    emit downloadFinished(m_stats);

    emitDebugMessage("Download stopped");
    qDebug() << "=== stopDownload completed ===";
}

void ChapterDownloader::processNextTask()
{
    if (!m_isDownloading || m_isPaused) {
        return;
    }

    QMutexLocker locker(&m_taskMutex);

    if (m_taskQueue.isEmpty()) {
        // Check if all tasks are completed
        if (m_activeDownloads == 0) {
            locker.unlock();
            stopDownload();
        }
        return;
    }

    if (m_activeDownloads >= m_config.maxConcurrent) {
        return;
    }

    DownloadTask task = m_taskQueue.dequeue();
    task.status = DownloadStatus::Downloading;

    // Update task in the all tasks list
    for (int i = 0; i < m_allTasks.size(); ++i) {
        if (m_allTasks[i].taskId == task.taskId) {
            m_allTasks[i] = task;
            break;
        }
    }

    m_activeDownloads++;
    locker.unlock();

    // Use new thread-safe architecture
    qDebug() << "=== PROCESSING TASK ===";
    qDebug() << "maxConcurrent:" << m_config.maxConcurrent;
    qDebug() << "saveIndividualChapters:" << m_config.saveIndividualChapters;
    qDebug() << "chapterSaveDir:" << m_config.chapterSaveDir;

    if (m_config.maxConcurrent == 1) {
        // Single-threaded mode: execute synchronously in main thread
        qDebug() << "=== USING SINGLE-THREADED MODE ===";
        emitDebugMessage(QString("Executing task synchronously: %1").arg(task.taskId));
        executeSyncDownload(task);
    } else {
        // Multi-threaded mode: create thread-safe worker
        qDebug() << "=== USING MULTI-THREADED MODE ===";
        ThreadSafeDownloadWorker* worker = new ThreadSafeDownloadWorker(task, m_config, task.bookSource);
        if (!worker) {
            emitDebugMessage("Failed to create thread-safe download worker");
            onTaskFailed(task.taskId, "Failed to create worker");
            return;
        }

        // Use Qt::QueuedConnection for thread-safe signal delivery
        connect(worker, &ThreadSafeDownloadWorker::taskCompleted, this, &ChapterDownloader::onTaskCompleted, Qt::QueuedConnection);
        connect(worker, &ThreadSafeDownloadWorker::taskFailed, this, &ChapterDownloader::onTaskFailed, Qt::QueuedConnection);

        worker->setAutoDelete(true);

        if (m_threadPool) {
            m_threadPool->start(worker);
        }
    }

    emitDebugMessage(QString("Started download task: %1").arg(task.chapter.title()));
    if (m_config.enableProgressCallback) {
        emit downloadProgress(m_stats.completedTasks, m_stats.totalTasks, task.chapter.title());
    }
}

void ChapterDownloader::onTaskCompleted(const QString& taskId, const DownloadTask& completedTask)
{
    qDebug() << "=== ChapterDownloader::onTaskCompleted CALLED ===";
    qDebug() << "Task ID:" << taskId;
    qDebug() << "Chapter:" << completedTask.chapter.title();
    qDebug() << "Content length:" << completedTask.content.length();

    QMutexLocker locker(&m_taskMutex);

    // Update task status
    DownloadTask* task = findTask(taskId);
    if (task) {
        *task = completedTask;
        m_recentDownloadTimes.append(completedTask.downloadTime);
        if (m_recentDownloadTimes.size() > 10) {
            m_recentDownloadTimes.removeFirst();
        }
    }

    m_activeDownloads--;
    m_consecutiveFailures = 0; // Reset consecutive failure count

    locker.unlock();

    updateStats();

    qDebug() << "=== About to emit taskCompleted signal ===";
    qDebug() << "Signal receivers count:" << receivers(SIGNAL(taskCompleted(QString,DownloadTask)));

    emit taskCompleted(taskId, completedTask);

    qDebug() << "=== taskCompleted signal emitted successfully ===";

    emitDebugMessage(QString("Task completed: %1 (%2ms)")
        .arg(completedTask.chapter.title())
        .arg(completedTask.downloadTime));

    // Send progress update
    if (m_config.enableProgressCallback) {
        emit downloadProgress(m_stats.completedTasks, m_stats.totalTasks, "");
    }

    // Adjust request interval
    if (m_config.enableSmartInterval) {
        adjustRequestInterval();
    }

    // Continue processing next task
    qDebug() << "=== Checking completion conditions ===";
    qDebug() << "Task queue empty:" << m_taskQueue.isEmpty();
    qDebug() << "Active downloads:" << m_activeDownloads;
    qDebug() << "Max concurrent:" << m_config.maxConcurrent;

    if (!m_taskQueue.isEmpty() && m_activeDownloads < m_config.maxConcurrent) {
        qDebug() << "=== Scheduling next task ===";
        scheduleNextTask();
    }
    else if (m_taskQueue.isEmpty() && m_activeDownloads == 0) {
        qDebug() << "=== All tasks completed, calling stopDownload ===";
        // All tasks completed
        stopDownload();
    } else {
        qDebug() << "=== No action taken - waiting for more tasks or downloads to complete ===";
    }

    qDebug() << "=== onTaskCompleted method ending ===";
}

void ChapterDownloader::onTaskFailed(const QString& taskId, const QString& error)
{
    QMutexLocker locker(&m_taskMutex);

    // Update task status
    DownloadTask* task = findTask(taskId);
    if (task) {
        task->status = DownloadStatus::Failed;
        task->error = error;
        task->retryCount++;

        // Check if retry is needed
        if (task->retryCount < m_config.maxRetries) {
            task->status = DownloadStatus::Pending;
            m_taskQueue.enqueue(*task);

            emitDebugMessage(QString("Task retry: %1 (attempt %2)")
                .arg(task->chapter.title())
                .arg(task->retryCount));
        }
        else {
            emitDebugMessage(QString("Task failed: %1 - %2")
                .arg(task->chapter.title())
                .arg(error));
        }
    }

    m_activeDownloads--;
    m_consecutiveFailures++;

    locker.unlock();

    updateStats();

    emit taskFailed(taskId, error);

    // If too many consecutive failures, increase request interval
    if (m_consecutiveFailures >= 3 && m_config.enableSmartInterval) {
        m_currentInterval = qMin(m_currentInterval * 2, 10000);
        emitDebugMessage(QString("Consecutive failures, adjusted interval to %1ms").arg(m_currentInterval));
    }

    // Continue processing next task
    if (!m_taskQueue.isEmpty() && m_activeDownloads < m_config.maxConcurrent) {
        scheduleNextTask();
    }
    else if (m_taskQueue.isEmpty() && m_activeDownloads == 0) {
        // All tasks completed
        stopDownload();
    }
}

void ChapterDownloader::scheduleNextTask()
{
    if (m_isPaused || !m_isDownloading) {
        return;
    }

    if (m_intervalTimer) {
        m_intervalTimer->start(m_currentInterval);
    }
}

void ChapterDownloader::adjustRequestInterval()
{
    // Adjust interval based on recent download times
    if (m_recentDownloadTimes.size() >= 5) {
        qint64 avgTime = 0;
        for (qint64 time : m_recentDownloadTimes) {
            avgTime += time;
        }
        avgTime /= m_recentDownloadTimes.size();

        // If average download time is too long, increase interval
        if (avgTime > 5000) {
            m_currentInterval = qMin(m_currentInterval + 500, 5000);
        }
        else if (avgTime < 1000) {
            m_currentInterval = qMax(m_currentInterval - 200, m_config.requestInterval);
        }

        m_recentDownloadTimes.clear();
    }
}

QString ChapterDownloader::generateTaskId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void ChapterDownloader::updateStats()
{
    m_stats.totalTasks = m_allTasks.size();
    m_stats.completedTasks = 0;
    m_stats.failedTasks = 0;
    m_stats.pendingTasks = 0;

    for (const DownloadTask& task : m_allTasks) {
        switch (task.status) {
        case DownloadStatus::Completed:
            m_stats.completedTasks++;
            break;
        case DownloadStatus::Failed:
            m_stats.failedTasks++;
            break;
        case DownloadStatus::Pending:
            m_stats.pendingTasks++;
            break;
        default:
            break;
        }
    }
}

DownloadTask* ChapterDownloader::findTask(const QString& taskId)
{
    for (int i = 0; i < m_allTasks.size(); ++i) {
        if (m_allTasks[i].taskId == taskId) {
            return &m_allTasks[i];
        }
    }
    return nullptr;
}

void ChapterDownloader::setError(const QString& error)
{
    m_lastError = error;
    emit downloadError(error);
    emitDebugMessage(QString("Error: %1").arg(error));
}

void ChapterDownloader::emitDebugMessage(const QString& message)
{
    emit debugMessage(QString("[ChapterDownloader] %1").arg(message));
}

void ChapterDownloader::executeSyncDownload(const DownloadTask &task)
{
    QElapsedTimer timer;
    timer.start();

    emitDebugMessage(QString("Starting sync download: %1 - %2").arg(task.taskId).arg(task.chapter.title()));

    // Validate required components
    if (!m_httpClient || !m_contentParser) {
        QString error = "HttpClient or ContentParser is null";
        emitDebugMessage(QString("Sync download failed: %1").arg(error));
        onTaskFailed(task.taskId, error);
        return;
    }

    // Check if chapter URL is valid
    if (task.chapter.url().isEmpty()) {
        QString error = "Chapter URL is empty";
        emitDebugMessage(QString("Sync download failed: %1").arg(error));
        onTaskFailed(task.taskId, error);
        return;
    }

    // Use HttpClient synchronously to get chapter HTML
    bool success = false;
    QString error;
    QString chapterHtml = m_httpClient->getSync(task.chapter.url(), QJsonObject(), &success, &error);

    if (!success || chapterHtml.isEmpty()) {
        QString errorMsg = QString("Failed to download chapter: %1").arg(error);
        emitDebugMessage(QString("Sync download failed: %1").arg(errorMsg));
        onTaskFailed(task.taskId, errorMsg);
        return;
    }

    emitDebugMessage(QString("Chapter HTML downloaded, length: %1").arg(chapterHtml.length()));

    // Parse chapter content using ContentParser with pagination support
    QString chapterContent;
    if (task.bookSource.chapterRule()) {
        const ChapterRule* chapterRule = task.bookSource.chapterRule();

        // Check if pagination is enabled for this chapter
        if (chapterRule->pagination() && !chapterRule->nextPage().isEmpty()) {
            chapterContent = downloadPaginatedChapterContent(chapterHtml, *chapterRule, task.chapter.url());
        } else {
            chapterContent = m_contentParser->parseChapterContent(chapterHtml, *chapterRule);
        }
    } else {
        // Simple HTML cleanup as fallback
        chapterContent = chapterHtml;
        chapterContent.remove(QRegularExpression("<[^>]*>"));
        chapterContent = chapterContent.trimmed();
    }

    if (chapterContent.isEmpty()) {
        QString error = "Parsed chapter content is empty";
        emitDebugMessage(QString("Sync download failed: %1").arg(error));
        onTaskFailed(task.taskId, error);
        return;
    }

    // Create completed task
    DownloadTask completedTask = task;
    completedTask.status = DownloadStatus::Completed;
    completedTask.content = chapterContent;
    completedTask.downloadTime = timer.elapsed();

    emitDebugMessage(QString("Sync download completed: %1 - Content length: %2, Time: %3ms")
                     .arg(task.taskId).arg(chapterContent.length()).arg(completedTask.downloadTime));

    // Emit completion signal
    qDebug() << "=== executeSyncDownload calling onTaskCompleted ===";
    qDebug() << "Task ID:" << task.taskId;
    qDebug() << "Content length:" << completedTask.content.length();

    onTaskCompleted(task.taskId, completedTask);

    qDebug() << "=== onTaskCompleted call finished ===";

    // Add delay between requests to avoid being blocked
    if (m_config.requestInterval > 0) {
        QThread::msleep(m_config.requestInterval);
    }
}

DownloadTask ChapterDownloader::getDownloadTask(const QString& taskId) const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_taskMutex));

    for (const DownloadTask& task : m_allTasks) {
        if (task.taskId == taskId) {
            return task;
        }
    }

    return DownloadTask(); // Return empty task
}

QList<DownloadTask> ChapterDownloader::getAllTasks() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_taskMutex));
    return m_allTasks;
}

QList<DownloadTask> ChapterDownloader::getTasksByStatus(DownloadStatus status) const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_taskMutex));

    QList<DownloadTask> result;
    for (const DownloadTask& task : m_allTasks) {
        if (task.status == status) {
            result.append(task);
        }
    }

    return result;
}

DownloadStats ChapterDownloader::getDownloadStats() const
{
    return m_stats;
}

QList<DownloadTask> ChapterDownloader::getCompletedTasks() const
{
    return getTasksByStatus(DownloadStatus::Completed);
}

QStringList ChapterDownloader::getDownloadedContent() const
{
    QStringList content;
    QList<DownloadTask> completedTasks = getCompletedTasks();

    for (const DownloadTask& task : completedTasks) {
        content.append(task.content);
    }

    return content;
}

bool ChapterDownloader::saveTasksToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    QList<DownloadTask> completedTasks = getCompletedTasks();

    for (const DownloadTask& task : completedTasks) {
        out << "=== " << task.chapter.title() << " ===" << "\n\n";
        out << task.content << "\n\n";
    }

    return true;
}

// ThreadSafeDownloadWorker implementation
ThreadSafeDownloadWorker::ThreadSafeDownloadWorker(const DownloadTask& task,
    const DownloadConfig& config,
    const BookSource& bookSource)
    : m_task(task)
    , m_config(config)
    , m_bookSource(bookSource)
{
}

void ThreadSafeDownloadWorker::run()
{
    QElapsedTimer timer;
    timer.start();

    DownloadTask task = m_task;

    // Check if chapter URL is valid
    if (task.chapter.url().isEmpty()) {
        task.status = DownloadStatus::Failed;
        task.error = "Chapter URL is empty";
        task.downloadTime = timer.elapsed();
        emit taskFailed(task.taskId, task.error);
        return;
    }

    // Download chapter content using thread-safe method
    QString downloadedContent = downloadChapterContent(task.chapter.url());

    if (downloadedContent.isEmpty()) {
        task.status = DownloadStatus::Failed;
        task.error = "Failed to download chapter content";
        task.downloadTime = timer.elapsed();
        emit taskFailed(task.taskId, task.error);
        return;
    }

    // Parse chapter content using thread-safe method
    QString parsedContent = parseChapterContent(downloadedContent);

    if (parsedContent.isEmpty()) {
        task.status = DownloadStatus::Failed;
        task.error = "Failed to parse chapter content";
        task.downloadTime = timer.elapsed();
        emit taskFailed(task.taskId, task.error);
        return;
    }

    // Save individual chapter file if enabled
    qDebug() << "=== CHECKING CHAPTER SAVE CONFIG ===";
    qDebug() << "saveIndividualChapters:" << m_config.saveIndividualChapters;
    qDebug() << "chapterSaveDir:" << m_config.chapterSaveDir;

    if (m_config.saveIndividualChapters && !m_config.chapterSaveDir.isEmpty()) {
        qDebug() << "=== SAVING INDIVIDUAL CHAPTER FILE ===";
        saveChapterToFile(task.chapter, parsedContent);
        qDebug() << "=== CHAPTER FILE SAVED ===";
    } else {
        qDebug() << "=== CHAPTER SAVE SKIPPED ===";
    }

    // Success: set task content and status
    task.status = DownloadStatus::Completed;
    task.content = parsedContent;
    task.downloadTime = timer.elapsed();

    emit taskCompleted(task.taskId, task);
}

QString ThreadSafeDownloadWorker::downloadChapterContent(const QString &url)
{
    // Create thread-local HttpClient for thread safety
    HttpClient httpClient;

    // Use HttpClient's synchronous method (same as single-threaded mode)
    bool success = false;
    QString error;
    QString result = httpClient.getSync(url, QJsonObject(), &success, &error);

    if (!success) {
        qDebug() << "ThreadSafeDownloadWorker: Download failed:" << error;
        return QString();
    }

    return result;
}

QString ThreadSafeDownloadWorker::parseChapterContent(const QString &html)
{
    if (html.isEmpty()) {
        return QString();
    }

    // Create thread-local ContentParser for thread safety
    ContentParser parser;

    // Use the same parsing logic as single-threaded mode
    QString chapterContent;
    if (m_bookSource.chapterRule()) {
        const ChapterRule* chapterRule = m_bookSource.chapterRule();

        // For now, use simple parsing (pagination support can be added later)
        chapterContent = parser.parseChapterContent(html, *chapterRule);
    } else {
        // Simple HTML cleanup as fallback
        chapterContent = html;
        chapterContent.remove(QRegularExpression("<[^>]*>"));
        chapterContent = chapterContent.trimmed();
    }

    return chapterContent;
}

void ThreadSafeDownloadWorker::saveChapterToFile(const Chapter &chapter, const QString &content)
{
    if (content.isEmpty() || m_config.chapterSaveDir.isEmpty()) {
        return;
    }

    // Create chapter save directory if it doesn't exist
    QDir dir;
    if (!dir.exists(m_config.chapterSaveDir)) {
        dir.mkpath(m_config.chapterSaveDir);
    }

    // Generate chapter filename: chapter_{order:03d}_{title}.txt
    QString safeTitle = chapter.title();
    safeTitle.replace(QRegularExpression("[<>:\"/\\\\|?*]"), "_"); // Replace invalid filename characters

    QString filename = QString("chapter_%1_%2.txt")
                      .arg(chapter.order(), 3, 10, QChar('0'))
                      .arg(safeTitle);

    QString filepath = QDir(m_config.chapterSaveDir).filePath(filename);

    // Save chapter content to file
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        // Write chapter header and content
        stream << "=== " << chapter.title() << " ===" << "\n\n";
        stream << content << "\n\n";

        file.close();
    }
}

void ChapterDownloader::onIntervalTimer()
{
    // Interval timer triggered, process next task
    processNextTask();
}

void ChapterDownloader::applyBookSourceConfig(const BookSource &bookSource)
{
    emitDebugMessage(QString("=== applyBookSourceConfig for source: %1 (ID: %2) ===").arg(bookSource.name()).arg(bookSource.id()));
    emitDebugMessage(QString("Current maxConcurrent before apply: %1").arg(m_config.maxConcurrent));

    if (!bookSource.crawlRule()) {
        emitDebugMessage("No crawl rule found, keeping current config");
        return;
    }

    const CrawlRule* crawlRule = bookSource.crawlRule();

    // Apply thread configuration
    if (crawlRule->threads() > 0) {
        m_config.maxConcurrent = crawlRule->threads();
        emitDebugMessage(QString("Applied book source thread config: %1").arg(crawlRule->threads()));
    }

    // Apply interval configuration
    int requestInterval = calculateRequestInterval(bookSource);
    if (requestInterval > 0) {
        m_config.requestInterval = requestInterval;
        m_currentInterval = requestInterval;
        emitDebugMessage(QString("Applied book source interval config: %1ms").arg(requestInterval));
    }

    // Apply retry configuration
    if (crawlRule->maxAttempts() > 0) {
        m_config.maxRetries = crawlRule->maxAttempts();
        emitDebugMessage(QString("Applied book source retry config: %1").arg(crawlRule->maxAttempts()));
    }

    // Special handling for high-risk book sources (only for truly problematic ones)
    if (isSpecialBookSource(bookSource.id())) {
        // Only apply special handling for sources that really need it
        // Increase timeout for problematic sources
        m_config.timeout = 30000; // 30 seconds

        // Enable smart interval adjustment
        m_config.enableSmartInterval = true;

        emitDebugMessage(QString("Applied special timeout handling for book source: %1").arg(bookSource.name()));
    }
}

int ChapterDownloader::calculateRequestInterval(const BookSource &bookSource)
{
    if (!bookSource.crawlRule()) {
        return m_config.requestInterval;
    }

    const CrawlRule* crawlRule = bookSource.crawlRule();

    // Use minInterval as base, with some randomization
    int minInterval = crawlRule->minInterval();
    int maxInterval = crawlRule->maxInterval();

    if (minInterval <= 0) {
        return m_config.requestInterval;
    }

    // Add randomization to avoid detection
    int range = maxInterval - minInterval;
    if (range > 0) {
        return minInterval + (qrand() % range);
    }

    return minInterval;
}

bool ChapterDownloader::isSpecialBookSource(int sourceId)
{
    // Only truly problematic book sources that need special timeout handling
    QList<int> specialSources = {
        16, // Dingdian Novel (JS encryption)
        2   // Shuhaige (cookies required)
    };

    return specialSources.contains(sourceId);
}

QString ChapterDownloader::downloadPaginatedChapterContent(const QString &firstPageHtml, const ChapterRule &rule, const QString &baseUrl)
{
    QString allContent;
    QString currentHtml = firstPageHtml;
    QString currentBaseUrl = baseUrl;
    int pageCount = 1;
    int maxPages = 20; // Safety limit for chapter content

    emitDebugMessage(QString("Starting paginated chapter content download from: %1").arg(baseUrl));

    while (!currentHtml.isEmpty() && pageCount <= maxPages) {
        emitDebugMessage(QString("Processing page %1 of chapter content").arg(pageCount));

        // Parse content from current page
        QString pageContent = m_contentParser->parseChapterContentSinglePage(currentHtml, rule);

        if (pageContent.isEmpty()) {
            emitDebugMessage(QString("No content found on page %1, stopping pagination").arg(pageCount));
            break;
        }

        // Append content with separator
        if (!allContent.isEmpty()) {
            allContent += "\n\n";
        }
        allContent += pageContent;

        emitDebugMessage(QString("Added content from page %1, total length: %2").arg(pageCount).arg(allContent.length()));

        // Find next page URL
        QString nextPageUrl = m_contentParser->parseNextPageUrl(currentHtml, rule.nextPage(), currentBaseUrl);

        if (nextPageUrl.isEmpty()) {
            emitDebugMessage("No next page URL found, pagination complete");
            break;
        }

        if (nextPageUrl == currentBaseUrl) {
            emitDebugMessage("Next page URL is same as current, stopping to prevent infinite loop");
            break;
        }

        emitDebugMessage(QString("Found next page URL: %1").arg(nextPageUrl));

        // Apply request interval for pagination
        if (m_config.requestInterval > 0) {
            QThread::msleep(m_config.requestInterval);
        }

        // Download next page
        bool success = false;
        QString error;
        QString nextPageHtml = m_httpClient->getSync(nextPageUrl, QJsonObject(), &success, &error);

        if (!success || nextPageHtml.isEmpty()) {
            emitDebugMessage(QString("Failed to download next page: %1 - %2").arg(nextPageUrl).arg(error));
            break;
        }

        currentHtml = nextPageHtml;
        currentBaseUrl = nextPageUrl;
        pageCount++;
    }

    if (pageCount > maxPages) {
        emitDebugMessage(QString("Reached maximum page limit (%1), stopping pagination").arg(maxPages));
    }

    emitDebugMessage(QString("Paginated chapter content download completed, total pages: %1, total length: %2").arg(pageCount).arg(allContent.length()));
    return allContent;
}

