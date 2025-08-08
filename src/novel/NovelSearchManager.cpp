#include "NovelSearchManager.h"
#include "../config/NovelConfig.h"
#include "NovelSearcher.h"
#include "../network/HttpClient.h"
#include "../parser/ContentParser.h"
#include "../parser/RuleManager.h"
#include "ChapterDownloader.h"
#include "FileGenerator.h"
#include "../config/settings.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMetaType>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

NovelSearchManager::NovelSearchManager(Settings* settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_novelConfig(nullptr)
    , m_httpClient(nullptr)
    , m_ruleManager(nullptr)
    , m_parser(nullptr)
    , m_searcher(nullptr)
    , m_downloader(nullptr)
    , m_generator(nullptr)
    , m_isSearching(false)
    , m_isDownloading(false)
    , m_totalSourcesSearching(0)
    , m_totalChapters(0)
    , m_downloadedChapters(0)
    , m_searchTimeoutTimer(nullptr)
    , m_currentSearchIndex(0)
    , m_sequentialTimer(nullptr)
{
    qDebug() << "NovelSearchManager constructor started";

    // Register meta types for Qt signal-slot system
    qRegisterMetaType<QList<SearchResult>>("QList<SearchResult>");
    qRegisterMetaType<SearchResult>("SearchResult");

    setupComponents();
    setupConnections();
    // Note: Do not load book sources here to avoid duplicate loading
    // Book sources will be loaded when NovelConfig is set

    qDebug() << "NovelSearchManager constructor completed";
}

NovelSearchManager::~NovelSearchManager()
{
    // Clean up timeout timers
    for (auto timer : m_sourceTimeoutTimers) {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
    }

    if (m_searchTimeoutTimer) {
        m_searchTimeoutTimer->stop();
        m_searchTimeoutTimer->deleteLater();
    }

    // Clean up sequential search timer
    if (m_sequentialTimer) {
        m_sequentialTimer->stop();
        m_sequentialTimer->deleteLater();
    }

    qDebug() << "NovelSearchManager destroyed";
}

// Deprecated linkView methods removed - NovelSearchViewEnhanced is connected directly in MainWindow

void NovelSearchManager::setNovelConfig(NovelConfig* config)
{
    m_novelConfig = config;
    qDebug() << "NovelConfig linked to NovelSearchManager";

    // Connect to config changes to reload book sources automatically
    if (m_novelConfig) {
        connect(m_novelConfig, &NovelConfig::configChanged, this, [this]() {
            qDebug() << "NovelConfig changed, reloading book sources...";
            loadBookSources();
        });

        // Load book sources for the first time with current config
        qDebug() << "Loading book sources with NovelConfig for the first time...";
        loadBookSources();
    }
}

QList<BookSource> NovelSearchManager::getAvailableSources() const
{
    return m_availableSources;
}

void NovelSearchManager::setupComponents()
{
    qDebug() << "NovelSearchManager::setupComponents - Starting component initialization...";
    
    // Create HTTP client
    m_httpClient = new HttpClient(this);
    m_httpClient->setTimeout(15000); // 15 seconds timeout (longer than search manager timeout)
    qDebug() << "HttpClient created with 15s timeout";
    
    // Create rule manager
    m_ruleManager = new RuleManager(this);
    qDebug() << "RuleManager created";
    
    // Create content parser
    m_parser = new ContentParser(this);
    m_parser->setRuleManager(m_ruleManager);  // Set RuleManager for source name resolution
    qDebug() << "ContentParser created and linked to RuleManager";

    // Create novel searcher
    m_searcher = new NovelSearcher(this);
    qDebug() << "NovelSearcher created";

    // Create chapter downloader
    m_downloader = new ChapterDownloader(this);
    qDebug() << "ChapterDownloader created";

    // Create file generator
    m_generator = new FileGenerator(this);
    qDebug() << "FileGenerator created";
    
    // Set up component dependencies
    qDebug() << "Setting up component dependencies...";
    qDebug() << "Component initialization completed successfully";
}

void NovelSearchManager::setupConnections()
{
    qDebug() << "Setting up signal-slot connections...";
    
    // Simplified signal connections - using simulation mode
    qDebug() << "Using simulation mode for search and download";
    
    qDebug() << "Signal-slot connections setup completed";
}

void NovelSearchManager::loadBookSources()
{
    qDebug() << "NovelSearchManager::loadBookSources - Starting book source loading...";

    if (!m_ruleManager) {
        qDebug() << "RuleManager not available";
        return;
    }

    // Clear existing sources to avoid duplicates
    m_ruleManager->clearRules();
    m_availableSources.clear();

    // Try to get active rules path from NovelConfig first
    QString activeRulesPath;
    if (m_novelConfig) {
        activeRulesPath = m_novelConfig->getActiveRules();
        qDebug() << "Active rules path from NovelConfig:" << activeRulesPath;
    } else {
        qDebug() << "NovelConfig object is null, using fallback settings";
        // Only use settings as fallback if NovelConfig is not available
        if (m_settings) {
            QSettings* qsettings = m_settings->getpSettings();
            if (qsettings) {
                activeRulesPath = qsettings->value("novel/activeRules", "").toString();
                qDebug() << "Active rules path from settings (fallback):" << activeRulesPath;
            }
        }
    }

    // Load rules based on configuration
    bool rulesLoaded = false;

    if (!activeRulesPath.isEmpty()) {
        // Try to load the specific rules file configured by user
        QFileInfo fileInfo(activeRulesPath);
        qDebug() << "Checking configured rules file:" << activeRulesPath;
        qDebug() << "File exists:" << fileInfo.exists();
        qDebug() << "Is file:" << fileInfo.isFile();
        qDebug() << "Absolute path:" << fileInfo.absoluteFilePath();

        if (fileInfo.exists() && fileInfo.isFile()) {
            qDebug() << "Loading configured rules file:" << activeRulesPath;
            rulesLoaded = m_ruleManager->loadRulesFromFile(activeRulesPath);
            qDebug() << "Configured rules loading result:" << rulesLoaded;
        } else {
            qDebug() << "Configured rules file does not exist:" << activeRulesPath;
        }
    }

    // If no specific rules file is configured or loading failed, try default directory
    if (!rulesLoaded) {
        QString rulesDir = "rules/";
        qDebug() << "No valid rules file configured, trying default directory:" << rulesDir;

        QDir dir(rulesDir);
        if (dir.exists()) {
            QStringList ruleFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
            if (!ruleFiles.isEmpty()) {
                qDebug() << "Loading rules from default directory...";
                for (const QString& fileName : ruleFiles) {
                    QString filePath = dir.absoluteFilePath(fileName);
                    qDebug() << "Loading rules from:" << filePath;
                    if (m_ruleManager->loadRulesFromFile(filePath)) {
                        rulesLoaded = true;
                    }
                }
            } else {
                qDebug() << "No JSON rule files found in directory:" << rulesDir;
            }
        } else {
            qDebug() << "Rules directory does not exist:" << rulesDir;
        }
    }

    if (!rulesLoaded) {
        qWarning() << "Failed to load any book source rules. Search functionality will be limited.";
    }

    // Get searchable sources
    m_availableSources = m_ruleManager->getSearchableSources();

    qDebug() << "Found" << m_availableSources.size() << "searchable sources";
    for (const BookSource& source : m_availableSources) {
        qDebug() << "Available source:" << source.name() << "ID:" << source.id();
    }

    qDebug() << "Book source loading completed successfully";
}

void NovelSearchManager::startSearch(const QString &keyword, int sourceId)
{
    if (m_isSearching) {
        qDebug() << "Search already in progress";
        return;
    }

    qDebug() << "Starting search for keyword:" << keyword << "sourceId:" << sourceId;

    // Reload book sources to get latest configuration
    loadBookSources();

    // Check if we have any available sources
    if (m_availableSources.isEmpty()) {
        QString errorMsg = "No book sources available. Please configure book sources in Settings first.\n\nTo configure:\n1. Click Settings button\n2. Select Active Rules file (JSON format)\n3. Apply settings and try search again";
        qDebug() << errorMsg;
        emit searchFailed(errorMsg);
        return;
    }

    {
        QMutexLocker locker(&m_searchMutex);

        m_isSearching = true;
        m_currentKeyword = keyword;
        m_searchResultsBySource.clear();
        m_completedSources.clear();

        // Clean up previous timeout timers
        for (auto timer : m_sourceTimeoutTimers) {
            if (timer) {
                timer->stop();
                timer->deleteLater();
            }
        }
        m_sourceTimeoutTimers.clear();
    } // Release lock before starting search

    emit searchStarted(keyword);

    if (sourceId == -1) {
        // Search all sources sequentially
        startSequentialSearch(keyword);
    } else {
        // Search single source
        startSingleSourceSearch(keyword, sourceId);
    }
}

void NovelSearchManager::startSingleSourceSearch(const QString &keyword, int sourceId)
{
    const BookSource* source = nullptr;
    for (const BookSource& s : m_availableSources) {
        if (s.id() == sourceId) {
            source = &s;
            break;
        }
    }

    if (!source) {
        QString errorMsg = QString("Book source with ID %1 not found. Please check your book source configuration in Settings.").arg(sourceId);
        onSearchFailed(errorMsg, sourceId);
        return;
    }

    qDebug() << "Starting single source search on:" << source->name();

    // Only emit progress for standalone single source search, not during sequential search
    if (m_searchQueue.isEmpty()) {
        m_totalSourcesSearching = 1;
        emit searchProgress("Searching " + source->name() + "...", 0, 1);
    }

    // Set up timeout timer
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(12000); // 12 seconds timeout
    m_sourceTimeoutTimers[sourceId] = timer;

    connect(timer, &QTimer::timeout, this, [this, sourceId]() {
        onSearchTimeout(sourceId);
    });

    timer->start();

    // Start real search using NovelSearcher
    if (m_searcher) {
        qDebug() << "Using NovelSearcher to perform real search";

        // Check critical components before configuring searcher
        if (!m_httpClient) {
            QString errorMsg = "HTTP client not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, sourceId);
            return;
        }

        if (!m_parser) {
            QString errorMsg = "Content parser not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, sourceId);
            return;
        }

        if (!m_ruleManager) {
            QString errorMsg = "Rule manager not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, sourceId);
            return;
        }

        // Configure searcher with HTTP client and parser
        m_searcher->setHttpClient(m_httpClient);
        m_searcher->setContentParser(m_parser);
        m_searcher->setRuleManager(m_ruleManager);

        // Perform synchronous search
        QList<SearchResult> results = m_searcher->searchSingleSourceSync(keyword, sourceId);

        if (!results.isEmpty()) {
            qDebug() << "Search completed successfully, found" << results.size() << "results";
            onSearchCompleted(results, sourceId);
        } else {
            QString error = m_searcher->getLastError();
            if (error.isEmpty()) {
                error = "No results found";
            }
            qDebug() << "Search failed:" << error;

            // Check if we're in sequential search mode
            qDebug() << "DEBUG: Before calling failed handler - m_isSearching:" << m_isSearching;
            if (!m_searchQueue.isEmpty() && m_currentSearchIndex < m_searchQueue.size()) {
                qDebug() << "DEBUG: Calling onSequentialSearchFailed";
                onSequentialSearchFailed(error, sourceId);
            } else {
                qDebug() << "DEBUG: Calling onSearchFailed";
                onSearchFailed(error, sourceId);
            }
        }
    } else {
        qDebug() << "NovelSearcher not available, search failed";

        // Check if we're in sequential search mode
        if (!m_searchQueue.isEmpty() && m_currentSearchIndex < m_searchQueue.size()) {
            onSequentialSearchFailed("Search engine not available", sourceId);
        } else {
            onSearchFailed("Search engine not available", sourceId);
        }
    }
}

void NovelSearchManager::startMultiSourceSearch(const QString &keyword)
{
    // Redirect to sequential search for better user experience
    startSequentialSearch(keyword);
}

// Backup concurrent search implementation - kept for future use
void NovelSearchManager::startConcurrentSearch(const QString &keyword)
{
    if (m_availableSources.isEmpty()) {
        onSearchFailed("No available sources", -1);
        return;
    }

    qDebug() << "Starting concurrent search across" << m_availableSources.size() << "sources";

    m_totalSourcesSearching = qMin(3, m_availableSources.size()); // Limit to 3 sources for performance
    emit searchProgress("Starting multi-source search...", 0, m_totalSourcesSearching);

    // Use first 3 sources for multi-source search
    QList<BookSource> selectedSources;
    for (int i = 0; i < m_totalSourcesSearching; ++i) {
        selectedSources.append(m_availableSources[i]);

        // Set up timeout timer for each source
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(12000); // 12 seconds timeout
        m_sourceTimeoutTimers[m_availableSources[i].id()] = timer;

        int currentSourceId = m_availableSources[i].id();
        connect(timer, &QTimer::timeout, this, [this, currentSourceId]() {
            onSearchTimeout(currentSourceId);
        });

        timer->start();
    }

    // Start real multi-source search using NovelSearcher
    if (m_searcher) {
        qDebug() << "Using NovelSearcher for multi-source search";

        // Check critical components before configuring searcher
        if (!m_httpClient) {
            QString errorMsg = "HTTP client not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, -1);
            return;
        }

        if (!m_parser) {
            QString errorMsg = "Content parser not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, -1);
            return;
        }

        if (!m_ruleManager) {
            QString errorMsg = "Rule manager not available";
            qDebug() << errorMsg;
            onSearchFailed(errorMsg, -1);
            return;
        }

        // Configure searcher
        m_searcher->setHttpClient(m_httpClient);
        m_searcher->setContentParser(m_parser);
        m_searcher->setRuleManager(m_ruleManager);

        // Perform search on each source with delay to avoid overwhelming servers
        for (int i = 0; i < selectedSources.size(); ++i) {
            const BookSource& source = selectedSources[i];

            // Use timer to stagger requests
            QTimer::singleShot(i * 500, this, [this, keyword, source]() {
                qDebug() << "Searching source:" << source.name();

                QList<SearchResult> results = m_searcher->searchSingleSourceSync(keyword, source.id());

                if (!results.isEmpty()) {
                    qDebug() << "Found" << results.size() << "results from" << source.name();
                    onSearchCompleted(results, source.id());
                } else {
                    QString error = m_searcher->getLastError();
                    if (error.isEmpty()) {
                        error = "No results found";
                    }
                    qDebug() << "Search failed for" << source.name() << ":" << error;
                    onSearchCompleted(QList<SearchResult>(), source.id()); // Empty results
                }
            });
        }
    } else {
        qDebug() << "NovelSearcher not available for multi-source search";
        onSearchFailed("Search engine not available", -1);
    }
}

void NovelSearchManager::startSequentialSearch(const QString &keyword)
{
    if (m_availableSources.isEmpty()) {
        onSearchFailed("No available sources", -1);
        return;
    }

    qDebug() << "Starting sequential search across" << m_availableSources.size() << "sources";

    // Initialize sequential search state
    m_searchQueue = m_availableSources;
    m_currentSearchIndex = 0;
    m_accumulatedResults.clear();
    m_totalSourcesSearching = m_searchQueue.size();

    // Send initial progress
    emit searchProgress("Starting sequential search...", 0, m_totalSourcesSearching);

    // Start searching the first source
    searchNextSource();
}

void NovelSearchManager::searchNextSource()
{
    // Check if all sources have been searched
    if (m_currentSearchIndex >= m_searchQueue.size()) {
        qDebug() << "Sequential search completed with" << m_accumulatedResults.size() << "total results";
        resetSearchState();
        emit searchCompleted(m_accumulatedResults);
        return;
    }

    // Get current source
    const BookSource& currentSource = m_searchQueue[m_currentSearchIndex];

    qDebug() << "Sequential search: processing source" << (m_currentSearchIndex + 1)
             << "of" << m_searchQueue.size() << ":" << currentSource.name();

    // Update progress
    emit searchProgress(QString("Searching %1... (%2/%3)")
                       .arg(currentSource.name())
                       .arg(m_currentSearchIndex + 1)
                       .arg(m_searchQueue.size()),
                       m_currentSearchIndex + 1, m_searchQueue.size());

    // Set up timeout timer for current source
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(15000); // 15 seconds timeout
    m_sourceTimeoutTimers[currentSource.id()] = timer;

    int currentSourceId = currentSource.id();
    connect(timer, &QTimer::timeout, this, [this, currentSourceId]() {
        onSearchTimeout(currentSourceId);
    });

    timer->start();

    // Start single source search
    startSingleSourceSearch(m_currentKeyword, currentSource.id());
}

void NovelSearchManager::onSequentialSearchCompleted(const QList<SearchResult> &results, int sourceId)
{
    qDebug() << "Sequential search: source" << sourceId << "completed with" << results.size() << "results";
    qDebug() << "onSequentialSearchCompleted: m_currentSearchIndex before increment:" << m_currentSearchIndex;

    // Add results to accumulated results
    if (!results.isEmpty()) {
        m_accumulatedResults.append(results);

        // Emit real-time results update
        emit searchResultsUpdated(results, sourceId);
    }

    // Clean up timeout timer for this source
    if (m_sourceTimeoutTimers.contains(sourceId)) {
        QTimer* timer = m_sourceTimeoutTimers[sourceId];
        if (timer) {
            timer->stop();
            timer->disconnect();
            timer->deleteLater();
        }
        m_sourceTimeoutTimers.remove(sourceId);
    }

    // Move to next source
    m_currentSearchIndex++;
    qDebug() << "onSequentialSearchCompleted: m_currentSearchIndex after increment:" << m_currentSearchIndex;

    // Schedule next source search with a small delay to avoid overwhelming servers
    if (m_sequentialTimer) {
        m_sequentialTimer->stop();
        m_sequentialTimer->disconnect();
        m_sequentialTimer->deleteLater();
        m_sequentialTimer = nullptr;
    }

    m_sequentialTimer = new QTimer(this);
    m_sequentialTimer->setSingleShot(true);

    qDebug() << "onSequentialSearchCompleted: Scheduling next source search in 1 second";
    connect(m_sequentialTimer, &QTimer::timeout, this, &NovelSearchManager::searchNextSource);
    m_sequentialTimer->start(1000); // 1 second delay
}

void NovelSearchManager::cancelSearch()
{
    if (!m_isSearching) {
        return;
    }

    qDebug() << "Cancelling search";

    QMutexLocker locker(&m_searchMutex);
    resetSearchState();

    emit searchFailed("Search cancelled by user");
}

// Old 2-parameter startDownload method removed - using enhanced 5-parameter version only

void NovelSearchManager::startDownload(const SearchResult &result, int startChapter, int endChapter, int mode, const QString &customPath)
{
    if (m_isDownloading) {
        qDebug() << "Download already in progress";
        return;
    }

    qDebug() << "=== REAL DOWNLOAD START ==="
             << "\nBook:" << result.bookName() << "by" << result.author()
             << "\nMode:" << mode << "Range:" << startChapter << "-" << endChapter
             << "\nBook URL:" << result.bookUrl()
             << "\nSource ID:" << result.sourceId();

    // === STEP 1: Validate Components ===
    if (!m_downloader || !m_httpClient || !m_parser || !m_ruleManager) {
        QString errorMsg = "Critical components not available";
        qDebug() << errorMsg;
        emit downloadFailed(errorMsg);
        return;
    }

    // === STEP 2: Get Book Source Rules ===
    const BookSource* bookSource = m_ruleManager->getSourceById(result.sourceId());
    if (!bookSource) {
        QString errorMsg = QString("Book source not found for ID: %1").arg(result.sourceId());
        qDebug() << errorMsg;
        emit downloadFailed(errorMsg);
        return;
    }

    // Set current book source for error handling
    m_currentBookSource = bookSource;
    m_specialSourceRetryCount = 0;

    qDebug() << "Found book source:" << bookSource->name();

    // === STEP 3: Initialize Download State ===
    QMutexLocker locker(&m_downloadMutex);
    m_isDownloading = true;
    m_currentResult = result;
    m_downloadedChapters = 0;
    m_downloadedContent.clear();

    emit downloadStarted(result);
    emit downloadProgress("Getting chapter list...", 0, 100);

    // === STEP 4: Get Chapter List ===
    qDebug() << "=== STEP 4: Getting chapter list ===";

    QString tocUrl = result.bookUrl();
    QString tocHtml;

    // Check if toc has a separate URL
    if (bookSource->tocRule() && !bookSource->tocRule()->url().isEmpty()) {
        // Extract book ID from book URL for toc URL template
        QRegularExpression idRegex(R"(/(\d+)/)");
        QRegularExpressionMatch match = idRegex.match(result.bookUrl());

        if (match.hasMatch()) {
            QString bookId = match.captured(1);
            tocUrl = QString(bookSource->tocRule()->url()).arg(bookId);
            qDebug() << "Using separate toc URL:" << tocUrl;
        } else {
            qDebug() << "Failed to extract book ID from URL:" << result.bookUrl();
            resetDownloadState();
            emit downloadFailed("Failed to extract book ID for chapter list");
            return;
        }
    }

    // Get chapter list page HTML
    bool success = false;
    QString error;
    tocHtml = m_httpClient->getSync(tocUrl, QJsonObject(), &success, &error);

    if (!success || tocHtml.isEmpty()) {
        QString errorMsg = QString("Failed to get chapter list page: %1").arg(error);
        qDebug() << errorMsg;
        resetDownloadState();
        emit downloadFailed(errorMsg);
        return;
    }

    qDebug() << "Chapter list page downloaded, HTML length:" << tocHtml.length();

    // Parse chapter list using ContentParser and book source rules
    QList<Chapter> allChapters = m_parser->parseChapterList(tocHtml, *bookSource, tocUrl);

    if (allChapters.isEmpty()) {
        QString errorMsg = "No chapters found in book";
        qDebug() << errorMsg;
        resetDownloadState();
        emit downloadFailed(errorMsg);
        return;
    }

    qDebug() << "Found" << allChapters.size() << "chapters total";

    // === STEP 5: Filter Chapters Based on Download Mode ===
    QList<Chapter> chaptersToDownload;

    switch (mode) {
    case 0: // ChapterRange
        if (endChapter == -1) {
            // Download from startChapter to end
            for (int i = startChapter - 1; i < allChapters.size(); ++i) {
                chaptersToDownload.append(allChapters[i]);
            }
        } else {
            // Download specific range
            for (int i = startChapter - 1; i < qMin(endChapter, allChapters.size()); ++i) {
                chaptersToDownload.append(allChapters[i]);
            }
        }
        break;

    case 1: // FullNovel
        chaptersToDownload = allChapters;
        break;

    case 2: // CustomPath (download all to custom path)
        chaptersToDownload = allChapters;
        break;

    default:
        chaptersToDownload = allChapters.mid(0, 10); // Default: first 10 chapters
        break;
    }

    if (chaptersToDownload.isEmpty()) {
        QString errorMsg = "No chapters to download in specified range";
        qDebug() << errorMsg;
        resetDownloadState();
        emit downloadFailed(errorMsg);
        return;
    }

    m_totalChapters = chaptersToDownload.size();
    qDebug() << "Will download" << m_totalChapters << "chapters";

    emit downloadProgress("Starting chapter downloads...", 0, m_totalChapters);

    // === STEP 6: Setup ChapterDownloader and Start Real Download ===
    qDebug() << "=== STEP 6: Setting up real chapter downloader ===";

    // Configure ChapterDownloader
    m_downloader->setHttpClient(m_httpClient);
    m_downloader->setContentParser(m_parser);

    // Configure download settings (allow multi-threading with conservative settings)
    DownloadConfig config = m_downloader->getDownloadConfig();
    // Note: maxConcurrent now uses default value from DownloadConfig (2 threads)
    config.requestInterval = 2000;  // 2 seconds between requests for stability
    config.timeout = 30000;  // 30 seconds timeout
    config.maxRetries = 2;  // 2 retries max

    // Enable individual chapter file saving using user's download path
    config.saveIndividualChapters = true;

    // Get user's download path from NovelConfig
    QString userDownloadPath;
    if (m_novelConfig) {
        userDownloadPath = m_novelConfig->getDownloadPath();
    }

    // Use default path if user path is empty
    if (userDownloadPath.isEmpty()) {
        userDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/NovelDownloads";
    }

    // Set chapter save directory to downloadPath/novel_chapters
    config.chapterSaveDir = QDir(userDownloadPath).filePath("novel_chapters");
    config.enableAutoMerge = true;

    qDebug() << "Chapter save directory set to:" << config.chapterSaveDir;

    m_downloader->setDownloadConfig(config);

    // Clear any existing tasks
    m_downloader->clearAllTasks();

    // Disconnect any existing connections first to avoid duplicates
    disconnect(m_downloader, &ChapterDownloader::taskCompleted, this, &NovelSearchManager::onRealChapterDownloaded);
    disconnect(m_downloader, &ChapterDownloader::downloadFinished, this, &NovelSearchManager::onAllChaptersDownloaded);
    disconnect(m_downloader, &ChapterDownloader::downloadError, this, &NovelSearchManager::onDownloadError);

    // Connect ChapterDownloader signals for this download session
    connect(m_downloader, &ChapterDownloader::taskCompleted, this, &NovelSearchManager::onRealChapterDownloaded, Qt::DirectConnection);
    connect(m_downloader, &ChapterDownloader::downloadFinished, this, &NovelSearchManager::onAllChaptersDownloaded, Qt::DirectConnection);
    connect(m_downloader, &ChapterDownloader::downloadError, this, &NovelSearchManager::onDownloadError, Qt::DirectConnection);

    // Add all chapters as download tasks
    for (const Chapter& chapter : chaptersToDownload) {
        QString taskId = m_downloader->addDownloadTask(chapter, *bookSource);
        qDebug() << "Added download task:" << taskId << "-" << chapter.title();
    }

    // Start the real download process
    m_downloader->startDownload();

    qDebug() << "=== REAL DOWNLOAD STARTED ===";
}

void NovelSearchManager::cancelDownload()
{
    if (!m_isDownloading) {
        return;
    }

    qDebug() << "Cancelling download";

    QMutexLocker locker(&m_downloadMutex);

    // Cancel real downloader
    if (m_downloader) {
        m_downloader->stopDownload();
    }

    resetDownloadState();
    emit downloadFailed("Download cancelled by user");
}

// === NEW METHODS FOR REAL DOWNLOAD HANDLING ===

void NovelSearchManager::onRealChapterDownloaded(const QString &taskId, const DownloadTask &task)
{
    qDebug() << "Chapter downloaded:" << task.chapter.title() << "Content length:" << task.content.length();

    // AVOID DEADLOCK: Don't use mutex here since startDownload already holds it
    // QMutexLocker locker(&m_downloadMutex);

    // Store the downloaded content (thread-safe since we're in single-thread mode)
    m_downloadedContent[taskId] = task.content;
    m_downloadedChapters++;

    // Update progress
    QString progressMsg = QString("Downloaded: %1").arg(task.chapter.title());
    qDebug() << "Progress:" << progressMsg << m_downloadedChapters << "/" << m_totalChapters;

    qDebug() << "=== onRealChapterDownloaded completed ===";
}

void NovelSearchManager::onAllChaptersDownloaded(const DownloadStats &stats)
{
    qDebug() << "=== ALL CHAPTERS DOWNLOADED ===" << stats.completedTasks << "chapters";
    qDebug() << "=== NovelSearchManager::onAllChaptersDownloaded CALLED ===";
    qDebug() << "Stats - Completed:" << stats.completedTasks << "Total:" << stats.totalTasks;

    QMutexLocker locker(&m_downloadMutex);

    if (!m_isDownloading) {
        qDebug() << "Download was cancelled, ignoring completion";
        return;
    }

    emit downloadProgress("Generating file...", m_totalChapters, m_totalChapters);

    // === STEP 7: Generate Final Text File ===
    qDebug() << "=== STEP 7: Generating final text file ===";

    // Get all completed tasks from downloader
    QList<DownloadTask> completedTasks = m_downloader->getCompletedTasks();

    if (completedTasks.isEmpty()) {
        resetDownloadState();
        emit downloadFailed("No chapters were successfully downloaded");
        return;
    }

    // Sort tasks by chapter order
    std::sort(completedTasks.begin(), completedTasks.end(),
              [](const DownloadTask &a, const DownloadTask &b) {
                  return a.chapter.order() < b.chapter.order();
              });

    // Create output directory using user's download path
    QString outputDir;
    if (m_novelConfig) {
        outputDir = m_novelConfig->getDownloadPath();
    }

    // Use default path if user path is empty
    if (outputDir.isEmpty()) {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/NovelDownloads";
    }

    QDir().mkpath(outputDir);

    // Generate filename
    QString safeBookName = m_currentResult.bookName();
    safeBookName.replace(QRegularExpression("[<>:\"/\\\\|?*]"), "_");
    QString fileName = QString("%1_%2.txt").arg(safeBookName).arg(m_currentResult.author());
    QString filePath = outputDir + "/" + fileName;

    // Write content to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        resetDownloadState();
        emit downloadFailed(QString("Cannot create file: %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // Write book header
    out << "=== " << m_currentResult.bookName() << " ===" << "\n";
    out << "Author: " << m_currentResult.author() << "\n";
    out << "Source: " << m_currentResult.sourceName() << "\n";
    out << "Downloaded: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    out << "Total Chapters: " << completedTasks.size() << "\n\n";

    // Write all chapters
    for (const DownloadTask &task : completedTasks) {
        out << "=== " << task.chapter.title() << " ===" << "\n\n";
        out << task.content << "\n\n";
    }

    file.close();

    qDebug() << "File generated successfully:" << filePath;

    // Reset download state
    resetDownloadState();

    // Emit completion signal
    emit downloadCompleted(filePath);

    qDebug() << "=== DOWNLOAD COMPLETED SUCCESSFULLY ===";
}

void NovelSearchManager::onDownloadError(const QString &error)
{
    qDebug() << "Download error:" << error;

    QMutexLocker locker(&m_downloadMutex);
    resetDownloadState();
    emit downloadFailed(error);
}

void NovelSearchManager::onSearchCompleted(const QList<SearchResult> &results, int sourceId)
{
    qDebug() << "Search completed for source" << sourceId << "with" << results.size() << "results";

    QMutexLocker locker(&m_searchMutex);

    if (!m_isSearching) {
        return;
    }

    // Check if we're in sequential search mode
    if (!m_searchQueue.isEmpty() && m_currentSearchIndex < m_searchQueue.size()) {
        // Sequential search mode - delegate to sequential handler
        locker.unlock(); // Release lock before calling sequential handler
        onSequentialSearchCompleted(results, sourceId);
        return;
    }

    // Original concurrent search logic
    // Stop and cleanup timeout timer for this source IMMEDIATELY
    if (m_sourceTimeoutTimers.contains(sourceId)) {
        QTimer* timer = m_sourceTimeoutTimers[sourceId];
        if (timer) {
            timer->stop();
            timer->disconnect(); // Disconnect all signals to prevent timeout callback
            timer->deleteLater();
        }
        m_sourceTimeoutTimers.remove(sourceId);
    }

    // Store results
    m_searchResultsBySource[sourceId] = results;
    m_completedSources.insert(sourceId);

    // Update progress
    emit searchProgress(QString("Completed source %1").arg(sourceId),
                       m_completedSources.size(), m_totalSourcesSearching);

    // Check if all sources completed
    if (m_completedSources.size() >= m_totalSourcesSearching) {
        // Merge all results
        QList<SearchResult> allResults;
        for (auto it = m_searchResultsBySource.begin(); it != m_searchResultsBySource.end(); ++it) {
            allResults.append(it.value());
        }

        resetSearchState();
        emit searchCompleted(allResults);
    }
}

void NovelSearchManager::onSequentialSearchFailed(const QString &error, int sourceId)
{
    qDebug() << "Sequential search failed for source" << sourceId << "error:" << error;
    qDebug() << "onSequentialSearchFailed: ENTRY - m_isSearching:" << m_isSearching;

    QMutexLocker locker(&m_searchMutex);
    qDebug() << "onSequentialSearchFailed: AFTER LOCK - m_isSearching:" << m_isSearching;

    if (!m_isSearching) {
        qDebug() << "onSequentialSearchFailed: Not searching, ignoring";
        return;
    }

    qDebug() << "DEBUG: queue size:" << m_searchQueue.size() << "index:" << m_currentSearchIndex;

    // In sequential search mode, continue to next source
    // Don't propagate individual source failures as overall search failures
    qDebug() << "onSequentialSearchFailed: Continuing to next source (ignoring individual source failure)";
    locker.unlock(); // Release lock before calling sequential handler
    qDebug() << "onSequentialSearchFailed: BEFORE CALLING onSequentialSearchCompleted";
    onSequentialSearchCompleted(QList<SearchResult>(), sourceId);
    qDebug() << "onSequentialSearchFailed: AFTER CALLING onSequentialSearchCompleted";
}

void NovelSearchManager::onSearchFailed(const QString &error, int sourceId)
{
    qDebug() << "Search failed for source" << sourceId << "error:" << error;

    QMutexLocker locker(&m_searchMutex);

    if (!m_isSearching) {
        qDebug() << "onSearchFailed: Not searching, ignoring";
        return;
    }

    // Check if we're in sequential search mode
    if (!m_searchQueue.isEmpty() && m_currentSearchIndex < m_searchQueue.size()) {
        qDebug() << "onSearchFailed: In sequential search mode, delegating to onSequentialSearchFailed";
        locker.unlock(); // Release lock before calling sequential handler
        onSequentialSearchFailed(error, sourceId);
        return;
    }

    // Stop and cleanup timeout timer for this source
    if (sourceId != -1 && m_sourceTimeoutTimers.contains(sourceId)) {
        QTimer* timer = m_sourceTimeoutTimers[sourceId];
        if (timer) {
            timer->stop();
            timer->disconnect(); // Disconnect all signals to prevent timeout callback
            timer->deleteLater();
        }
        m_sourceTimeoutTimers.remove(sourceId);
    }

    if (sourceId == -1 || m_totalSourcesSearching == 1) {
        // Complete failure
        resetSearchState();
        emit searchFailed(error);
    } else {
        // Partial failure, mark source as completed
        m_completedSources.insert(sourceId);

        // Check if all sources completed
        if (m_completedSources.size() >= m_totalSourcesSearching) {
            // Merge all results
            QList<SearchResult> allResults;
            for (auto it = m_searchResultsBySource.begin(); it != m_searchResultsBySource.end(); ++it) {
                allResults.append(it.value());
            }

            resetSearchState();
            emit searchCompleted(allResults);
        }
    }
}

void NovelSearchManager::onSearchTimeout(int sourceId)
{
    qDebug() << "Search timeout occurred for source:" << sourceId;

    QMutexLocker locker(&m_searchMutex);

    if (!m_isSearching) {
        return;
    }

    // Check if we're in sequential search mode
    if (!m_searchQueue.isEmpty() && m_currentSearchIndex < m_searchQueue.size()) {
        // Check if this timeout is for the current source being searched
        const BookSource& currentSource = m_searchQueue[m_currentSearchIndex];
        if (sourceId == currentSource.id()) {
            qDebug() << "Sequential search timeout for current source" << sourceId << "- continuing to next source";
            // Sequential search mode - delegate to sequential handler with empty results
            locker.unlock(); // Release lock before calling sequential handler
            onSequentialSearchCompleted(QList<SearchResult>(), sourceId);
        } else {
            qDebug() << "Sequential search timeout for non-current source" << sourceId << "- ignoring (current source is" << currentSource.id() << ")";
        }
        return;
    }

    // Check if this source has already completed successfully
    if (m_completedSources.contains(sourceId)) {
        qDebug() << "Source" << sourceId << "already completed successfully, ignoring timeout";
        return;
    }

    // Clean up timeout timer for this specific source
    if (m_sourceTimeoutTimers.contains(sourceId)) {
        QTimer* timer = m_sourceTimeoutTimers[sourceId];
        if (timer) {
            timer->stop();
            timer->disconnect();
            timer->deleteLater();
        }
        m_sourceTimeoutTimers.remove(sourceId);
    }

    // Mark this source as completed (with empty results due to timeout)
    m_completedSources.insert(sourceId);

    // Check if this was a single source search or if all sources are now completed
    if (m_totalSourcesSearching == 1 || m_completedSources.size() >= m_totalSourcesSearching) {
        // Force completion with current results
        QList<SearchResult> allResults;

        // For sequential search, use accumulated results
        if (!m_searchQueue.isEmpty()) {
            allResults = m_accumulatedResults;
            qDebug() << "Sequential search timeout - using accumulated results:" << allResults.size();
        } else {
            // For concurrent search, use results by source
            for (auto it = m_searchResultsBySource.begin(); it != m_searchResultsBySource.end(); ++it) {
                allResults.append(it.value());
            }
            qDebug() << "Concurrent search timeout - using source results:" << allResults.size();
        }

        resetSearchState();

        if (allResults.isEmpty()) {
            emit searchFailed("Search timeout - no results found");
        } else {
            qDebug() << "Search completed with" << allResults.size() << "results despite some timeouts";
            emit searchCompleted(allResults);
        }
    } else {
        // Partial timeout, continue waiting for other sources
        emit searchProgress(QString("Source %1 timed out, continuing with other sources...").arg(sourceId),
                           m_completedSources.size(), m_totalSourcesSearching);
    }
}

// Keep the old method for backward compatibility
void NovelSearchManager::onSearchTimeout()
{
    onSearchTimeout(-1);
}

void NovelSearchManager::onDownloadProgress(int current, int total)
{
    emit downloadProgress(QString("Downloading chapters... %1/%2").arg(current).arg(total),
                         current, total);
}

void NovelSearchManager::onDownloadTaskCompleted(const QString &taskId, const QString &content)
{
    QMutexLocker locker(&m_downloadMutex);

    if (!m_isDownloading) {
        return;
    }

    m_downloadedContent[taskId] = content;
    m_downloadedChapters++;

    emit downloadProgress(QString("Downloaded chapter %1/%2").arg(m_downloadedChapters).arg(m_totalChapters),
                         m_downloadedChapters, m_totalChapters);

    // Check if all chapters downloaded
    if (m_downloadedChapters >= m_totalChapters) {
        generateFile();
    }
}

void NovelSearchManager::onDownloadTaskFailed(const QString &taskId, const QString &error)
{
    Q_UNUSED(taskId)

    qDebug() << "Download task failed:" << error;

    QMutexLocker locker(&m_downloadMutex);

    if (!m_isDownloading) {
        return;
    }

    // Check if this is a special book source that needs enhanced error handling
    if (m_currentBookSource && isSpecialBookSourceError(m_currentBookSource->id(), error)) {
        if (handleSpecialBookSourceError(m_currentBookSource->id(), error)) {
            // Error was handled, continue download
            return;
        }
    }

    resetDownloadState();
    emit downloadFailed("Download failed: " + error);
}

void NovelSearchManager::onFileGenerated(const QString &filePath)
{
    qDebug() << "File generated successfully:" << filePath;

    QMutexLocker locker(&m_downloadMutex);
    resetDownloadState();

    emit downloadCompleted(filePath);
}

void NovelSearchManager::onFileGenerationFailed(const QString &error)
{
    qDebug() << "File generation failed:" << error;

    QMutexLocker locker(&m_downloadMutex);
    resetDownloadState();

    emit downloadFailed("File generation failed: " + error);
}

void NovelSearchManager::generateFile()
{
    if (!m_generator) {
        onFileGenerationFailed("FileGenerator not available");
        return;
    }

    // Create output directory using user's download path
    QString outputDir;
    if (m_novelConfig) {
        outputDir = m_novelConfig->getDownloadPath();
    }

    // Use default path if user path is empty
    if (outputDir.isEmpty()) {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/NovelDownloads";
    }

    QDir().mkpath(outputDir);
    qDebug() << "Final file output directory:" << outputDir;

    // Generate filename
    QString fileName = m_currentResult.bookName().replace(QRegExp("[<>:\"/\\|?*]"), "_") + ".txt";
    QString filePath = outputDir + "/" + fileName;

    // Prepare content
    QString fullContent;
    fullContent += "Title: " + m_currentResult.bookName() + "\n";
    fullContent += "Author: " + m_currentResult.author() + "\n";
    fullContent += "Source: " + m_currentResult.sourceName() + "\n";
    fullContent += "Generated: " + QDateTime::currentDateTime().toString() + "\n\n";
    fullContent += QString("=").repeated(50) + "\n\n";

    // Add chapters
    for (int i = 1; i <= m_totalChapters; ++i) {
        QString chapterId = QString("chapter_%1").arg(i);
        if (m_downloadedContent.contains(chapterId)) {
            fullContent += QString("Chapter %1\n").arg(i);
            fullContent += QString("-").repeated(20) + "\n";
            fullContent += m_downloadedContent[chapterId] + "\n\n";
        }
    }

    // Generate file directly
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << fullContent;
        file.close();
        onFileGenerated(filePath);
    } else {
        onFileGenerationFailed("Failed to create file: " + filePath);
    }
}

void NovelSearchManager::resetSearchState()
{
    m_isSearching = false;
    m_currentKeyword.clear();
    m_searchResultsBySource.clear();
    m_completedSources.clear();
    m_totalSourcesSearching = 0;

    // Clean up sequential search state
    m_searchQueue.clear();
    m_currentSearchIndex = 0;
    m_accumulatedResults.clear();

    // Clean up timeout timers safely
    for (auto it = m_sourceTimeoutTimers.begin(); it != m_sourceTimeoutTimers.end(); ++it) {
        QTimer* timer = it.value();
        if (timer) {
            timer->stop();
            timer->disconnect(); // Disconnect all signals to prevent dangling connections
            timer->deleteLater();
        }
    }
    m_sourceTimeoutTimers.clear();

    // Clean up global search timeout timer
    if (m_searchTimeoutTimer) {
        m_searchTimeoutTimer->stop();
        m_searchTimeoutTimer->disconnect();
        m_searchTimeoutTimer->deleteLater();
        m_searchTimeoutTimer = nullptr;
    }

    // Clean up sequential timer
    if (m_sequentialTimer) {
        m_sequentialTimer->stop();
        m_sequentialTimer->disconnect();
        m_sequentialTimer->deleteLater();
        m_sequentialTimer = nullptr;
    }
}

void NovelSearchManager::resetDownloadState()
{
    m_isDownloading = false;
    m_currentResult = SearchResult();
    m_currentBookSource = nullptr;
    m_totalChapters = 0;
    m_downloadedChapters = 0;
    m_downloadedContent.clear();
    m_specialSourceRetryCount = 0;
}

bool NovelSearchManager::isSpecialBookSourceError(int sourceId, const QString &error)
{
    // Check if this is a known error pattern for special book sources
    QStringList rateLimitErrors = {
        "timeout", "timed out", "connection timeout",
        "rate limit", "too many requests", "blocked",
        "403", "429", "503", "502", "504"
    };

    QStringList encryptionErrors = {
        "empty content", "no content", "parsing failed",
        "invalid content", "content not found"
    };

    // High-risk sources with specific error patterns
    switch (sourceId) {
        case 16: // Dingdian Novel (JS encryption)
            return encryptionErrors.contains(error.toLower()) ||
                   error.contains("script") || error.contains("decrypt");

        case 9:  // Zero Point Novel (rate limiting)
        case 5:  // Tianxi Novel (IP restriction)
        case 2:  // Shuhaige (cookies required)
            for (const QString& pattern : rateLimitErrors) {
                if (error.toLower().contains(pattern)) {
                    return true;
                }
            }
            break;

        case 3:  // Mengshuzw (connection timeout)
        case 4:  // Niaoshuwang (read timeout)
            return error.toLower().contains("timeout") ||
                   error.toLower().contains("connection");
    }

    return false;
}

bool NovelSearchManager::handleSpecialBookSourceError(int sourceId, const QString &error)
{
    qDebug() << "Handling special book source error for source" << sourceId << ":" << error;

    // Limit retry attempts for special sources
    if (m_specialSourceRetryCount >= 3) {
        qDebug() << "Max retry attempts reached for special source";
        return false;
    }

    m_specialSourceRetryCount++;

    switch (sourceId) {
        case 16: // Dingdian Novel (JS encryption)
            if (error.contains("decrypt") || error.contains("empty content")) {
                qDebug() << "Retrying with enhanced JS processing for Dingdian Novel";
                adjustDownloadStrategyForSource(sourceId);
                return true;
            }
            break;

        case 9:  // Zero Point Novel (rate limiting)
        case 5:  // Tianxi Novel (IP restriction)
            if (error.contains("timeout") || error.contains("rate limit")) {
                qDebug() << "Applying rate limiting strategy for source" << sourceId;
                adjustDownloadStrategyForSource(sourceId);

                // Wait before retry
                QTimer::singleShot(5000, this, [this]() {
                    if (m_downloader && m_isDownloading) {
                        m_downloader->resumeDownload();
                    }
                });
                return true;
            }
            break;

        case 2:  // Shuhaige (cookies required)
            if (error.contains("403") || error.contains("blocked")) {
                qDebug() << "Retrying with enhanced cookie handling for Shuhaige";
                adjustDownloadStrategyForSource(sourceId);
                return true;
            }
            break;

        case 3:  // Mengshuzw (connection timeout)
        case 4:  // Niaoshuwang (read timeout)
            if (error.contains("timeout")) {
                qDebug() << "Retrying with extended timeout for source" << sourceId;
                adjustDownloadStrategyForSource(sourceId);
                return true;
            }
            break;
    }

    return false;
}

void NovelSearchManager::adjustDownloadStrategyForSource(int sourceId)
{
    if (!m_downloader) {
        return;
    }

    DownloadConfig config = m_downloader->getDownloadConfig();

    switch (sourceId) {
        case 16: // Dingdian Novel
            config.maxConcurrent = 1;
            config.requestInterval = 3000;
            config.timeout = 30000;
            config.maxRetries = 5;
            break;

        case 9:  // Zero Point Novel
            config.maxConcurrent = 1;
            config.requestInterval = 5000;
            config.timeout = 25000;
            config.maxRetries = 3;
            break;

        case 5:  // Tianxi Novel
            config.maxConcurrent = 1;
            config.requestInterval = 2000;
            config.timeout = 20000;
            config.maxRetries = 4;
            break;

        case 2:  // Shuhaige
            config.maxConcurrent = 1;
            config.requestInterval = 1500;
            config.timeout = 15000;
            config.maxRetries = 3;
            break;

        case 3:  // Mengshuzw
        case 4:  // Niaoshuwang
            config.maxConcurrent = 1;
            config.requestInterval = 2000;
            config.timeout = 30000;
            config.maxRetries = 4;
            break;

        default:
            // General conservative settings for unknown special sources
            config.maxConcurrent = 1;
            config.requestInterval = 2000;
            config.timeout = 20000;
            config.maxRetries = 3;
            break;
    }

    m_downloader->setDownloadConfig(config);
    qDebug() << "Adjusted download strategy for source" << sourceId
             << "- interval:" << config.requestInterval
             << "timeout:" << config.timeout;
}
