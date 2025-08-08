#include "NovelSearcher.h"
#include <QDebug>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <algorithm>

// SearchTask implementation

SearchTask::SearchTask(const QString &keyword, const BookSource &source, HttpClient *httpClient, ContentParser *parser)
    : m_keyword(keyword), m_source(source), m_httpClient(httpClient), m_parser(parser)
{
    setAutoDelete(true);
}

void SearchTask::run()
{
    try {
        if (!m_source.hasSearch()) {
            emit searchFailed("Source does not support search", m_source.id());
            return;
        }
        const SearchRule *rule = m_source.searchRule();
        QString searchUrl = rule->url();
        QJsonObject headers;
        QString searchData = rule->data();
        if (!searchData.isEmpty()) {
            if (searchData.trimmed().startsWith("{")) {
                searchData = searchData.replace("{", "").replace("}", "").replace(":", "=").replace(" ", "");
            }
            searchData.replace("%s", m_keyword);
        }
        QNetworkReply *reply = nullptr;
        if (rule->method().toLower() == "post") {
            headers["Content-Type"] = "application/x-www-form-urlencoded";
            reply = m_httpClient->post(searchUrl, searchData.toUtf8(), headers);
        } else {
            if (searchUrl.contains("%s")) {
                searchUrl.replace("%s", m_keyword);
            }
            reply = m_httpClient->get(searchUrl, headers);
        }
        if (!reply) {
            emit searchFailed("Network request failed", m_source.id());
            return;
        }
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            emit searchFailed(reply->errorString(), m_source.id());
            reply->deleteLater();
            return;
        }
        QString html = QString::fromUtf8(reply->readAll());
        reply->deleteLater();
        QList<SearchResult> results = m_parser->parseSearchResults(html, m_source, searchUrl);
        emit searchCompleted(results, m_source.id());
    } catch (const std::exception &e) {
        emit searchFailed(QString("Search exception: %1").arg(e.what()), m_source.id());
    } catch (...) {
        emit searchFailed("Unknown search exception", m_source.id());
    }
}

// NovelSearcher implementation

NovelSearcher::NovelSearcher(QObject *parent)
    : QObject(parent)
    , m_ruleManager(nullptr)
    , m_httpClient(nullptr)
    , m_contentParser(nullptr)
    , m_threadPool(new QThreadPool(this))
    , m_isSearching(false)
    , m_activeSearches(0)
    , m_completedSources(0)
    , m_totalSources(0)
    , m_searchStartTime(0)
    , m_cacheCleanupTimer(new QTimer(this))
{
    m_threadPool->setMaxThreadCount(m_config.maxConcurrent);

    m_cacheCleanupTimer->setInterval(300000);
    m_cacheCleanupTimer->setSingleShot(false);
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &NovelSearcher::clearCache);
    
    if (m_config.enableCache) {
        m_cacheCleanupTimer->start();
    }
}

NovelSearcher::~NovelSearcher()
{
    cancelAllSearches();
}

void NovelSearcher::setRuleManager(RuleManager *ruleManager)
{
    m_ruleManager = ruleManager;
}

void NovelSearcher::setHttpClient(HttpClient *httpClient)
{
    m_httpClient = httpClient;
    
    if (m_httpClient) {
        connect(m_httpClient, &HttpClient::requestFinished,
                this, &NovelSearcher::onHttpRequestFinished);
    }
}

void NovelSearcher::setContentParser(ContentParser *parser)
{
    m_contentParser = parser;
}

void NovelSearcher::setSearchConfig(const SearchConfig &config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    
    m_threadPool->setMaxThreadCount(m_config.maxConcurrent);
    if (m_config.enableCache && !m_cacheCleanupTimer->isActive()) {
        m_cacheCleanupTimer->start();
    } else if (!m_config.enableCache && m_cacheCleanupTimer->isActive()) {
        m_cacheCleanupTimer->stop();
        clearCache();
    }
}

QFuture<QList<SearchResult>> NovelSearcher::searchSingleSource(const QString &keyword, int sourceId)
{
    if (!m_ruleManager) {
        return QtConcurrent::run([this]() {
            setError("RuleManager not set");
            return QList<SearchResult>();
        });
    }
    
    const BookSource *source = m_ruleManager->getSourceById(sourceId);
    if (!source) {
        return QtConcurrent::run([this, sourceId]() {
            setError(QString("Source not found: %1").arg(sourceId));
            return QList<SearchResult>();
        });
    }
    
    return searchSingleSource(keyword, *source);
}

QFuture<QList<SearchResult>> NovelSearcher::searchSingleSource(const QString &keyword, const BookSource &source)
{
    return QtConcurrent::run([this, keyword, source]() {
        return performSingleSearch(keyword, source);
    });
}

QFuture<NovelSearcher::SearchResultSet> NovelSearcher::searchMultiSource(const QString &keyword)
{
    if (!m_ruleManager) {
        return QtConcurrent::run([this]() {
            setError("RuleManager not set");
            return SearchResultSet();
        });
    }
    
    QList<BookSource> sources = m_ruleManager->getSearchableSources();
    return searchMultiSource(keyword, sources);
}

QFuture<NovelSearcher::SearchResultSet> NovelSearcher::searchMultiSource(const QString &keyword, const QList<int> &sourceIds)
{
    if (!m_ruleManager) {
        return QtConcurrent::run([this]() {
            setError("RuleManager not set");
            return SearchResultSet();
        });
    }
    
    QList<BookSource> sources;
    for (int id : sourceIds) {
        const BookSource *source = m_ruleManager->getSourceById(id);
        if (source && source->hasSearch() && !source->disabled()) {
            sources.append(*source);
        }
    }
    
    return searchMultiSource(keyword, sources);
}

QFuture<NovelSearcher::SearchResultSet> NovelSearcher::searchMultiSource(const QString &keyword, const QList<BookSource> &sources)
{
    return QtConcurrent::run([this, keyword, sources]() {
        return performMultiSearch(keyword, sources);
    });
}

QFuture<NovelSearcher::SearchResultSet> NovelSearcher::searchAggregated(const QString &keyword)
{
    return searchMultiSource(keyword);
}

QList<SearchResult> NovelSearcher::searchSingleSourceSync(const QString &keyword, int sourceId)
{
    QFuture<QList<SearchResult>> future = searchSingleSource(keyword, sourceId);
    future.waitForFinished();
    return future.result();
}

NovelSearcher::SearchResultSet NovelSearcher::searchMultiSourceSync(const QString &keyword)
{
    QFuture<SearchResultSet> future = searchMultiSource(keyword);
    future.waitForFinished();
    return future.result();
}

NovelSearcher::SearchResultSet NovelSearcher::searchAggregatedSync(const QString &keyword)
{
    QFuture<SearchResultSet> future = searchAggregated(keyword);
    future.waitForFinished();
    return future.result();
}

QList<SearchResult> NovelSearcher::performSingleSearch(const QString &keyword, const BookSource &source)
{
    // Create thread-local HttpClient for thread safety
    HttpClient threadLocalHttpClient;

    // Set up cookies if specified in source configuration
    const SearchRule *rule = source.searchRule();
    if (rule && !rule->cookies().isEmpty()) {
        QString cookiesStr = rule->cookies();
        qDebug() << "NovelSearcher: Setting cookies for source" << source.id() << ":" << cookiesStr;

        // Parse cookies string like "waf_sc=''; HMACCOUNT=''"
        QStringList cookiePairs = cookiesStr.split(";", Qt::SkipEmptyParts);
        for (const QString &pair : cookiePairs) {
            QString trimmedPair = pair.trimmed();
            int equalPos = trimmedPair.indexOf('=');
            if (equalPos > 0) {
                QString name = trimmedPair.left(equalPos).trimmed();
                QString value = trimmedPair.mid(equalPos + 1).trimmed();
                // Remove quotes if present
                if (value.startsWith("'") && value.endsWith("'")) {
                    value = value.mid(1, value.length() - 2);
                }
                threadLocalHttpClient.setCookie(name, value, QUrl(source.url()).host());
                qDebug() << "NovelSearcher: Set cookie" << name << "=" << value;
            }
        }
    }

    if (!m_contentParser) {
        setError("ContentParser not set");
        return QList<SearchResult>();
    }

    if (!source.hasSearch()) {
        setError("Source does not support search");
        return QList<SearchResult>();
    }

    if (source.disabled()) {
        setError("Source is disabled");
        return QList<SearchResult>();
    }

    if (m_config.enableCache) {
        QString cacheKey = getCacheKey(keyword, source.id());
        QList<SearchResult> cached = getCachedResults(cacheKey);
        if (!cached.isEmpty()) {
            qDebug() << "NovelSearcher: Using cached results, source" << source.id();
            return cached;
        }
    }

    qDebug() << "NovelSearcher: Start searching, source" << source.id() << "keyword" << keyword;
    
    try {
        const SearchRule *rule = source.searchRule();
        
        QString searchUrl = rule->url();
        QString searchData = buildFormData(rule->data(), keyword);
        QNetworkReply *reply = nullptr;
        QJsonObject headers;

        QString html;
        bool requestSuccess = false;
        QString requestError;

        if (rule->method().toLower() == "post") {
            // 使用curl兼容的headers，因为curl测试成功了
            headers["Content-Type"] = "application/x-www-form-urlencoded";
            headers["User-Agent"] = "curl/7.68.0";
            headers["Accept"] = "*/*";
            headers["Connection"] = "keep-alive";
            qDebug() << "NovelSearcher: POST request to" << searchUrl << "with data:" << searchData;
            html = threadLocalHttpClient.postSync(searchUrl, searchData.toUtf8(), headers, &requestSuccess, &requestError);
        } else {
            if (searchUrl.contains("%s")) {
                searchUrl.replace("%s", keyword);
            }
            html = threadLocalHttpClient.getSync(searchUrl, headers, &requestSuccess, &requestError);
        }

        qDebug() << "NovelSearcher: Request result - Success:" << requestSuccess << "Error:" << requestError << "HTML length:" << html.length();

        if (!requestSuccess) {
            setError(QString("Network request failed: %1").arg(requestError));
            return QList<SearchResult>();
        }

        // Special handling for 书海阁小说网 (two-step search)
        if (source.id() == 2 && requestSuccess && html.length() < 5000) {
            qDebug() << "NovelSearcher: Detected ShuHaiGe short response, attempting two-step search";

            // For ShuHaiGe, any short response (especially empty or redirect) should trigger two-step search
            bool shouldTryTwoStep = false;

            if (html.isEmpty()) {
                qDebug() << "NovelSearcher: ShuHaiGe returned empty response (likely redirect)";
                shouldTryTwoStep = true;
            } else if (html.contains("找不到您要搜索的内容") || html.contains("错误提示") ||
                       html.contains("301 Moved Permanently") || html.contains("302 Found")) {
                qDebug() << "NovelSearcher: ShuHaiGe returned error/redirect page";
                shouldTryTwoStep = true;
            }

            if (shouldTryTwoStep) {
                qDebug() << "NovelSearcher: Trying to extract search ID";

                // Try to extract search ID from redirect or page content
                QString searchId = extractShuHaiGeSearchId(html, keyword, threadLocalHttpClient);
                if (!searchId.isEmpty()) {
                    // Use the same protocol as the original source URL
                    QString baseUrl = source.url();
                    QString protocol = baseUrl.startsWith("https") ? "https" : "http";
                    QString realSearchUrl = QString("%1://www.shuhaige.net/search/%2/1.html").arg(protocol).arg(searchId);
                    qDebug() << "NovelSearcher: ShuHaiGe second step - GET request to" << realSearchUrl;

                    html = threadLocalHttpClient.getSync(realSearchUrl, headers, &requestSuccess, &requestError);
                    if (requestSuccess) {
                        qDebug() << "NovelSearcher: ShuHaiGe two-step search successful, response length:" << html.length();
                    } else {
                        qDebug() << "NovelSearcher: ShuHaiGe second step failed:" << requestError;
                    }
                }
            }
        }

        QList<SearchResult> allResults = m_contentParser->parseSearchResults(html, source, searchUrl);

        // Filter results to ensure they contain the search keyword
        QList<SearchResult> filteredResults = filterSearchResults(allResults, keyword);

        qDebug() << "NovelSearcher: Total parsed results:" << allResults.size()
                 << "Filtered results:" << filteredResults.size();

        if (filteredResults.size() > m_config.maxResults) {
            filteredResults = filteredResults.mid(0, m_config.maxResults);
        }

        QList<SearchResult> results = filteredResults;

        if (m_config.enableCache && !results.isEmpty()) {
            QString cacheKey = getCacheKey(keyword, source.id());
            setCachedResults(cacheKey, results);
        }

        qDebug() << "NovelSearcher: Search completed, source" << source.id() << "results" << results.size();
        return results;

    } catch (const std::exception &e) {
        setError(QString("Search exception: %1").arg(e.what()));
        return QList<SearchResult>();
    } catch (...) {
        setError("Unknown search exception");
        return QList<SearchResult>();
    }
}

NovelSearcher::SearchResultSet NovelSearcher::performMultiSearch(const QString &keyword, const QList<BookSource> &sources)
{
    SearchResultSet resultSet;
    resultSet.totalSources = sources.size();

    if (sources.isEmpty()) {
        setError("No available sources");
        return resultSet;
    }

    qDebug() << "NovelSearcher: Start multi-source search, keyword" << keyword << "sources" << sources.size();

    QElapsedTimer timer;
    timer.start();

    {
        QMutexLocker locker(&m_mutex);
        m_isSearching = true;
        m_currentKeyword = keyword;
        m_currentMode = MULTI_SOURCE;
        m_pendingResults.clear();
        m_pendingErrors.clear();
        m_completedSources = 0;
        m_totalSources = sources.size();
        m_searchStartTime = timer.elapsed();
    }

    emit searchStarted(keyword, MULTI_SOURCE);

    QList<QFuture<QList<SearchResult>>> futures;

    for (const BookSource &source : sources) {
        if (m_config.excludeSources.contains(source.name())) {
            continue;
        }

        QFuture<QList<SearchResult>> future = QtConcurrent::run([this, keyword, source]() {
            return performSingleSearch(keyword, source);
        });

        futures.append(future);
    }
    for (int i = 0; i < futures.size(); ++i) {
        QFuture<QList<SearchResult>> &future = futures[i];
        future.waitForFinished();

        QList<SearchResult> results = future.result();
        int sourceId = sources[i].id();

        if (!results.isEmpty()) {
            resultSet.results.append(results);
            resultSet.sourceResultCounts[sourceId] = results.size();
            resultSet.successSources++;
        }

        {
            QMutexLocker locker(&m_mutex);
            m_completedSources++;
        }

        // Emit single source completed signal for each source
        emit singleSourceCompleted(results, sourceId);
        emit searchProgress(m_completedSources, m_totalSources);
    }

    resultSet.elapsedTime = timer.elapsed();

    if (m_config.enableDedup) {
        resultSet.results = deduplicateResults(resultSet.results);
    }

    if (m_config.sortType != NO_SORT) {
        resultSet.results = sortResults(resultSet.results, m_config.sortType, keyword);
    }

    if (resultSet.results.size() > m_config.maxResults) {
        resultSet.results = resultSet.results.mid(0, m_config.maxResults);
    }

    {
        QMutexLocker locker(&m_mutex);
        m_isSearching = false;
    }

    qDebug() << "NovelSearcher: Multi-source search completed, total results" << resultSet.results.size()
             << "successful sources" << resultSet.successSources << "/" << resultSet.totalSources
             << "elapsed" << resultSet.elapsedTime << "ms";

    emit multiSourceCompleted(resultSet);
    emit searchFinished(keyword, resultSet.results.size());

    return resultSet;
}

QList<SearchResult> NovelSearcher::sortResults(const QList<SearchResult> &results, SortType sortType, const QString &keyword)
{
    switch (sortType) {
    case BY_RELEVANCE:
        return sortByRelevance(results, keyword);
    case BY_AUTHOR:
        return sortByAuthor(results);
    case BY_UPDATE_TIME:
        return sortByUpdateTime(results);
    case BY_SOURCE_PRIORITY:
        return sortBySourcePriority(results);
    default:
        return results;
    }
}

QList<SearchResult> NovelSearcher::deduplicateResults(const QList<SearchResult> &results)
{
    QList<SearchResult> deduplicated;

    for (const SearchResult &result : results) {
        bool duplicate = false;
        for (const SearchResult &existing : deduplicated) {
            if (isDuplicate(result, existing)) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            deduplicated.append(result);
        }
    }

    qDebug() << "NovelSearcher: Before dedup" << results.size() << "results, after dedup" << deduplicated.size() << "results";
    return deduplicated;
}

QList<SearchResult> NovelSearcher::filterResults(const QList<SearchResult> &results, const QString &filter)
{
    if (filter.isEmpty()) {
        return results;
    }

    QList<SearchResult> filtered;
    QRegularExpression regex(filter, QRegularExpression::CaseInsensitiveOption);

    for (const SearchResult &result : results) {
        if (result.bookName().contains(regex) || result.author().contains(regex)) {
            filtered.append(result);
        }
    }

    return filtered;
}

void NovelSearcher::enableCache(bool enable)
{
    QMutexLocker locker(&m_mutex);
    m_config.enableCache = enable;

    if (enable && !m_cacheCleanupTimer->isActive()) {
        m_cacheCleanupTimer->start();
    } else if (!enable && m_cacheCleanupTimer->isActive()) {
        m_cacheCleanupTimer->stop();
        clearCache();
    }
}

void NovelSearcher::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_searchCache.clear();
    qDebug() << "NovelSearcher: Search cache cleared";
}

void NovelSearcher::cancelSearch()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isSearching) {
        return;
    }

    for (QNetworkReply *reply : m_activeReplies) {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    }
    m_activeReplies.clear();

    m_threadPool->clear();

    m_isSearching = false;
    emit searchCancelled();

    qDebug() << "NovelSearcher: Search cancelled";
}

void NovelSearcher::cancelAllSearches()
{
    cancelSearch();
}

QList<SearchResult> NovelSearcher::sortByRelevance(const QList<SearchResult> &results, const QString &keyword)
{
    if (keyword.isEmpty()) {
        return results;
    }

    QList<SearchResult> sorted = results;

    std::sort(sorted.begin(), sorted.end(), [this, keyword](const SearchResult &a, const SearchResult &b) {
        double relevanceA = calculateRelevance(a, keyword);
        double relevanceB = calculateRelevance(b, keyword);
        return relevanceA > relevanceB;
    });

    return sorted;
}

QList<SearchResult> NovelSearcher::sortByAuthor(const QList<SearchResult> &results)
{
    QList<SearchResult> sorted = results;

    std::sort(sorted.begin(), sorted.end(), [](const SearchResult &a, const SearchResult &b) {
        if (a.author() == b.author()) {
            return a.bookName() < b.bookName();
        }
        return a.author() < b.author();
    });

    return sorted;
}

QList<SearchResult> NovelSearcher::sortByUpdateTime(const QList<SearchResult> &results)
{
    QList<SearchResult> sorted = results;

    std::sort(sorted.begin(), sorted.end(), [](const SearchResult &a, const SearchResult &b) {
        return a.lastUpdateTime() > b.lastUpdateTime();
    });

    return sorted;
}

QList<SearchResult> NovelSearcher::sortBySourcePriority(const QList<SearchResult> &results)
{
    QList<SearchResult> sorted = results;

    std::sort(sorted.begin(), sorted.end(), [](const SearchResult &a, const SearchResult &b) {
        return a.sourceId() < b.sourceId();
    });

    return sorted;
}

QString NovelSearcher::getCacheKey(const QString &keyword, int sourceId)
{
    return QString("%1_%2").arg(keyword).arg(sourceId);
}

QList<SearchResult> NovelSearcher::getCachedResults(const QString &cacheKey)
{
    QMutexLocker locker(&m_mutex);
    return m_searchCache.value(cacheKey);
}

void NovelSearcher::setCachedResults(const QString &cacheKey, const QList<SearchResult> &results)
{
    QMutexLocker locker(&m_mutex);
    m_searchCache[cacheKey] = results;
}

double NovelSearcher::calculateRelevance(const SearchResult &result, const QString &keyword)
{
    double relevance = 0.0;

    QString bookName = result.bookName().toLower();
    QString key = keyword.toLower();

    if (bookName == key) {
        relevance += 0.6;
    } else if (bookName.contains(key)) {
        relevance += 0.4;
    } else {
        int distance = 0;
        int maxLen = qMax(bookName.length(), key.length());
        if (maxLen > 0) {
            for (int i = 0; i < qMin(bookName.length(), key.length()); ++i) {
                if (bookName[i] != key[i]) {
                    distance++;
                }
            }
            distance += qAbs(bookName.length() - key.length());
            relevance += 0.3 * (1.0 - (double)distance / maxLen);
        }
    }

    QString author = result.author().toLower();
    if (author.contains(key)) {
        relevance += 0.3;
    }

    QString category = result.category().toLower();
    if (category.contains(key)) {
        relevance += 0.1;
    }

    return relevance;
}

bool NovelSearcher::isDuplicate(const SearchResult &result1, const SearchResult &result2)
{

    return result1.bookName().trimmed().toLower() == result2.bookName().trimmed().toLower() &&
           result1.author().trimmed().toLower() == result2.author().trimmed().toLower();
}

QList<SearchResult> NovelSearcher::filterSearchResults(const QList<SearchResult> &results, const QString &keyword)
{
    QList<SearchResult> filteredResults;
    QString lowerKeyword = keyword.simplified().toLower();

    qDebug() << "NovelSearcher::filterSearchResults - Filtering" << results.size()
             << "results with keyword:" << keyword;

    for (const SearchResult &result : results) {
        bool matches = false;
        QString reason;

        // Check if book name contains the keyword
        if (containsKeyword(result.bookName(), lowerKeyword)) {
            matches = true;
            reason = "book name";
        }
        // Check if author contains the keyword
        else if (containsKeyword(result.author(), lowerKeyword)) {
            matches = true;
            reason = "author";
        }
        // Check if category contains the keyword (optional)
        else if (containsKeyword(result.category(), lowerKeyword)) {
            matches = true;
            reason = "category";
        }

        if (matches) {
            filteredResults.append(result);
            qDebug() << "NovelSearcher::filterSearchResults - MATCH (" << reason << "):"
                     << "Book:" << result.bookName()
                     << "Author:" << result.author();
        } else {
            qDebug() << "NovelSearcher::filterSearchResults - SKIP:"
                     << "Book:" << result.bookName()
                     << "Author:" << result.author()
                     << "- No keyword match";
        }
    }

    qDebug() << "NovelSearcher::filterSearchResults - Filtered from" << results.size()
             << "to" << filteredResults.size() << "results";

    return filteredResults;
}

bool NovelSearcher::containsKeyword(const QString &text, const QString &keyword)
{
    if (text.isEmpty() || keyword.isEmpty()) {
        return false;
    }

    QString lowerText = text.simplified().toLower();
    QString lowerKeyword = keyword.simplified().toLower();

    // Direct substring match
    if (lowerText.contains(lowerKeyword)) {
        return true;
    }

    // Split keyword into words and check if all words are present
    QStringList keywordWords = lowerKeyword.split(' ', Qt::SkipEmptyParts);
    if (keywordWords.size() > 1) {
        bool allWordsFound = true;
        for (const QString &word : keywordWords) {
            if (!lowerText.contains(word)) {
                allWordsFound = false;
                break;
            }
        }
        if (allWordsFound) {
            return true;
        }
    }

    return false;
}

// 静态工具方法，可以被SearchTask使用
static QString buildFormDataFromTemplate(const QString &dataTemplate, const QString &keyword)
{
    QString result;
    if (dataTemplate.isEmpty()) {
        return QString();
    }

    // Removed verbose debug output

    // 如果数据模板看起来像JSON格式 (以 { 开头)
    if (dataTemplate.trimmed().startsWith("{")) {
        // 解析JSON格式的数据模板，类似原项目的buildData方法
        QString jsonStr = dataTemplate;
        jsonStr.replace("%s", keyword);

        // 简单的JSON解析，将 {key: value} 转换为 key=value&key2=value2

        // 移除首尾的大括号
        if (jsonStr.startsWith("{") && jsonStr.endsWith("}")) {
            jsonStr = jsonStr.mid(1, jsonStr.length() - 2).trimmed();
        }

        // 分割键值对（处理单个键值对的情况）
        QStringList pairs;
        if (jsonStr.contains(",")) {
            pairs = jsonStr.split(",");
        } else {
            pairs.append(jsonStr); // 单个键值对
        }
        QStringList formPairs;

        for (const QString &pair : pairs) {
            QString trimmedPair = pair.trimmed();
            QStringList keyValue = trimmedPair.split(":");
            if (keyValue.size() == 2) {
                QString key = keyValue[0].trimmed();
                QString value = keyValue[1].trimmed();

                // 移除引号
                if (key.startsWith("\"") && key.endsWith("\"")) {
                    key = key.mid(1, key.length() - 2);
                }
                if (value.startsWith("\"") && value.endsWith("\"")) {
                    value = value.mid(1, value.length() - 2);
                }

                // URL编码中文字符
                QString encodedValue = QUrl::toPercentEncoding(value);
                formPairs.append(QString("%1=%2").arg(key, encodedValue));
            }
        }

        result = formPairs.join("&");
        return result;
    } else {
        // 直接的表单数据格式
        result = dataTemplate;
        result.replace("%s", keyword);
        return result;
    }
}

QString NovelSearcher::buildFormData(const QString &dataTemplate, const QString &keyword)
{
    return buildFormDataFromTemplate(dataTemplate, keyword);
}

void NovelSearcher::setError(const QString &error)
{
    QMutexLocker locker(&m_mutex);
    m_lastError = error;
    qWarning() << "NovelSearcher:" << error;
    emit searchError(error);
}

void NovelSearcher::updateProgress()
{
    emit searchProgress(m_completedSources, m_totalSources);
}

void NovelSearcher::onSingleSearchCompleted(const QList<SearchResult> &results, int sourceId)
{
    QMutexLocker locker(&m_mutex);

    if (!results.isEmpty()) {
        m_pendingResults[sourceId] = results;
    }

    m_completedSources++;
    updateProgress();

    emit singleSourceCompleted(results, sourceId);
}

void NovelSearcher::onSingleSearchFailed(const QString &error, int sourceId)
{
    QMutexLocker locker(&m_mutex);

    m_pendingErrors.append(QString("Source %1: %2").arg(sourceId).arg(error));
    m_completedSources++;
    updateProgress();
}

void NovelSearcher::onHttpRequestFinished(QNetworkReply *reply)
{
    QMutexLocker locker(&m_mutex);

    m_activeReplies.removeAll(reply);

    if (m_replySourceMap.contains(reply)) {
        int sourceId = m_replySourceMap.take(reply);

        if (reply->error() == QNetworkReply::NoError) {
        } else {
            onSingleSearchFailed(reply->errorString(), sourceId);
        }
    }
}

QString NovelSearcher::extractShuHaiGeSearchId(const QString &html, const QString &keyword, HttpClient &httpClient)
{
    qDebug() << "NovelSearcher: Attempting to extract search ID for keyword:" << keyword;

    // Method 1: Try to find redirect URL in HTML meta refresh
    QRegularExpression metaRefreshRegex(R"(<meta[^>]*http-equiv\s*=\s*['"]\s*refresh\s*['"][^>]*content\s*=\s*['"][^'";]*url\s*=\s*([^'";\s]+)['"])", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = metaRefreshRegex.match(html);
    if (match.hasMatch()) {
        QString redirectUrl = match.captured(1);
        qDebug() << "NovelSearcher: Found meta refresh redirect:" << redirectUrl;

        QRegularExpression idRegex(R"(/search/(\d+)/\d+\.html)");
        QRegularExpressionMatch idMatch = idRegex.match(redirectUrl);
        if (idMatch.hasMatch()) {
            QString searchId = idMatch.captured(1);
            qDebug() << "NovelSearcher: Extracted search ID from meta refresh:" << searchId;
            return searchId;
        }
    }

    // Method 2: Try to find search ID in JavaScript location.href
    QRegularExpression jsRegex(R"(location\.href\s*=\s*['"]([^'"]*)/search/(\d+)/\d+\.html['"])");
    match = jsRegex.match(html);
    if (match.hasMatch()) {
        QString searchId = match.captured(2);
        qDebug() << "NovelSearcher: Found search ID in JavaScript:" << searchId;
        return searchId;
    }

    // Method 3: Try to find jump link with search ID
    QRegularExpression jumpRegex(R"(href\s*=\s*['"]([^'"]*)/search/(\d+)/\d+\.html['"])");
    match = jumpRegex.match(html);
    if (match.hasMatch()) {
        QString searchId = match.captured(2);
        qDebug() << "NovelSearcher: Found search ID in jump link:" << searchId;
        return searchId;
    }

    // Method 4: Try to extract search ID from response cookies
    qDebug() << "NovelSearcher: Trying to extract search ID from POST response cookies";

    QJsonObject headers;
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
    headers["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
    headers["Connection"] = "keep-alive";
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36";
    headers["Referer"] = "https://www.shuhaige.net/";
    headers["Origin"] = "https://www.shuhaige.net";

    QString postData = QString("searchkey=%1&searchtype=all").arg(QString(QUrl::toPercentEncoding(keyword)));

    bool success = false;
    QString error;
    QString response = httpClient.postSync("https://www.shuhaige.net/search.html", postData.toUtf8(), headers, &success, &error);

    if (success) {
        qDebug() << "NovelSearcher: POST request successful, checking response cookies";

        // Get response cookies from HttpClient
        QList<QNetworkCookie> responseCookies = httpClient.getLastResponseCookies();
        qDebug() << "NovelSearcher: Got" << responseCookies.size() << "response cookies";

        for (const QNetworkCookie &cookie : responseCookies) {
            // Check if this is the waf_sc cookie which might contain the search ID
            if (cookie.name() == "waf_sc") {
                QString cookieValue = QString::fromUtf8(cookie.value());
                qDebug() << "NovelSearcher: Found waf_sc cookie, but it's not the search ID";
                // Note: waf_sc cookie is not the search ID, it's likely a WAF token
                break;
            }
        }

        // If cookie extraction failed, try to extract from HTML response
        if (response.length() > 1000) {
            qDebug() << "NovelSearcher: Cookie extraction failed, trying HTML patterns";

            QRegularExpression patterns[] = {
                QRegularExpression(R"(/search/(\d+)/\d+\.html)"),
                QRegularExpression(R"(search/(\d+)/1\.html)"),
                QRegularExpression(R"(searchid['":\s]*(\d+))"),
                QRegularExpression(R"(id['":\s]*(\d+))")
            };

            for (const auto &pattern : patterns) {
                QRegularExpressionMatch patternMatch = pattern.match(response);
                if (patternMatch.hasMatch()) {
                    QString searchId = patternMatch.captured(1);
                    qDebug() << "NovelSearcher: Found search ID in HTML response:" << searchId;
                    return searchId;
                }
            }
        }
    }

    // Method 5: If all else fails, try to construct a search URL based on keyword analysis
    qDebug() << "NovelSearcher: All extraction methods failed, using fallback approach";

    // Try to make a direct search request to see if we can find any pattern
    QString directSearchUrl = QString("https://www.shuhaige.net/search.html?searchkey=%1&searchtype=all")
                                .arg(QString(QUrl::toPercentEncoding(keyword)));

    QString directResponse = httpClient.getSync(directSearchUrl, headers, &success, &error);
    if (success && directResponse.length() > 1000) {
        qDebug() << "NovelSearcher: Direct search got response, trying to extract search ID";

        QRegularExpression directPattern(R"(/search/(\d+)/\d+\.html)");
        QRegularExpressionMatch directMatch = directPattern.match(directResponse);
        if (directMatch.hasMatch()) {
            QString searchId = directMatch.captured(1);
            qDebug() << "NovelSearcher: Found search ID in direct search:" << searchId;
            return searchId;
        }
    }

    qDebug() << "NovelSearcher: Failed to extract search ID dynamically";
    return QString();
}
