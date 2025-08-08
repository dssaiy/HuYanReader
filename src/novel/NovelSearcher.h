#ifndef NOVELSEARCHER_H
#define NOVELSEARCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QMutex>
#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QNetworkReply>
#include <QJsonObject>
#include <QFuture>
#include <QFutureWatcher>
#include "NovelModels.h"
#include "../parser/RuleManager.h"
#include "../parser/ContentParser.h"
#include "../network/HttpClient.h"


class SearchTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    SearchTask(const QString &keyword, const BookSource &source, HttpClient *httpClient, ContentParser *parser);
    void run() override;

signals:
    void searchCompleted(const QList<SearchResult> &results, int sourceId);
    void searchFailed(const QString &error, int sourceId);

private:
    QString m_keyword;
    BookSource m_source;
    HttpClient *m_httpClient;
    ContentParser *m_parser;
};

/**
 * @brief Novel searcher class providing single and multi-source search functionality
 */
class NovelSearcher : public QObject
{
    Q_OBJECT

public:
    enum SearchMode {
        SINGLE_SOURCE,
        MULTI_SOURCE,
        AGGREGATED_SEARCH
    };

    enum SortType {
        NO_SORT,
        BY_RELEVANCE,
        BY_AUTHOR,
        BY_UPDATE_TIME,
        BY_SOURCE_PRIORITY
    };

    struct SearchConfig {
        int maxResults;
        int timeout;
        int maxConcurrent;
        bool enableCache;
        bool enableDedup;
        SortType sortType;
        QStringList excludeSources;
        
        SearchConfig() 
            : maxResults(50), timeout(30000), maxConcurrent(5)
            , enableCache(true), enableDedup(true), sortType(BY_RELEVANCE) {}
    };

    struct SearchResultSet {
        QList<SearchResult> results;
        QHash<int, int> sourceResultCounts;
        QStringList errors;
        int totalSources;
        int successSources;
        qint64 elapsedTime;
        
        SearchResultSet() : totalSources(0), successSources(0), elapsedTime(0) {}
    };

    explicit NovelSearcher(QObject *parent = nullptr);
    ~NovelSearcher();


    void setRuleManager(RuleManager *ruleManager);
    void setHttpClient(HttpClient *httpClient);
    void setContentParser(ContentParser *parser);

    void setSearchConfig(const SearchConfig &config);
    SearchConfig getSearchConfig() const { return m_config; }

    QFuture<QList<SearchResult>> searchSingleSource(const QString &keyword, int sourceId);
    QFuture<QList<SearchResult>> searchSingleSource(const QString &keyword, const BookSource &source);

    QFuture<SearchResultSet> searchMultiSource(const QString &keyword);
    QFuture<SearchResultSet> searchMultiSource(const QString &keyword, const QList<int> &sourceIds);
    QFuture<SearchResultSet> searchMultiSource(const QString &keyword, const QList<BookSource> &sources);

    QFuture<SearchResultSet> searchAggregated(const QString &keyword);

    QList<SearchResult> searchSingleSourceSync(const QString &keyword, int sourceId);
    SearchResultSet searchMultiSourceSync(const QString &keyword);
    SearchResultSet searchAggregatedSync(const QString &keyword);

    QList<SearchResult> sortResults(const QList<SearchResult> &results, SortType sortType, const QString &keyword = QString());
    QList<SearchResult> deduplicateResults(const QList<SearchResult> &results);
    QList<SearchResult> filterResults(const QList<SearchResult> &results, const QString &filter);

    void enableCache(bool enable = true);
    void clearCache();
    bool isCacheEnabled() const { return m_config.enableCache; }

    bool isSearching() const { return m_isSearching; }
    int getActiveSearchCount() const { return m_activeSearches; }
    QString getLastError() const { return m_lastError; }

    void cancelSearch();
    void cancelAllSearches();

signals:
    void searchStarted(const QString &keyword, SearchMode mode);
    void searchProgress(int completed, int total);
    void singleSourceCompleted(const QList<SearchResult> &results, int sourceId);
    void multiSourceCompleted(const SearchResultSet &resultSet);
    void searchFinished(const QString &keyword, int totalResults);
    void searchCancelled();
    void searchError(const QString &error);

private slots:
    void onSingleSearchCompleted(const QList<SearchResult> &results, int sourceId);
    void onSingleSearchFailed(const QString &error, int sourceId);
    void onHttpRequestFinished(QNetworkReply *reply);

private:
    QList<SearchResult> performSingleSearch(const QString &keyword, const BookSource &source);
    SearchResultSet performMultiSearch(const QString &keyword, const QList<BookSource> &sources);

    QNetworkReply* sendSearchRequest(const QString &keyword, const BookSource &source);
    QList<SearchResult> parseSearchResponse(QNetworkReply *reply, const BookSource &source, const QString &keyword);
    QString buildFormData(const QString &dataTemplate, const QString &keyword);

    QList<SearchResult> sortByRelevance(const QList<SearchResult> &results, const QString &keyword);
    QList<SearchResult> sortByAuthor(const QList<SearchResult> &results);
    QList<SearchResult> sortByUpdateTime(const QList<SearchResult> &results);
    QList<SearchResult> sortBySourcePriority(const QList<SearchResult> &results);

    QString getCacheKey(const QString &keyword, int sourceId);
    QList<SearchResult> getCachedResults(const QString &cacheKey);
    void setCachedResults(const QString &cacheKey, const QList<SearchResult> &results);

    double calculateRelevance(const SearchResult &result, const QString &keyword);
    bool isDuplicate(const SearchResult &result1, const SearchResult &result2);
    QList<SearchResult> filterSearchResults(const QList<SearchResult> &results, const QString &keyword);
    bool containsKeyword(const QString &text, const QString &keyword);
    void setError(const QString &error);
    void updateProgress();

    // Special handling for 书海阁小说网 two-step search
    QString extractShuHaiGeSearchId(const QString &html, const QString &keyword, HttpClient &httpClient);

    RuleManager *m_ruleManager;
    HttpClient *m_httpClient;
    ContentParser *m_contentParser;
    QThreadPool *m_threadPool;

    SearchConfig m_config;
    bool m_isSearching;
    int m_activeSearches;
    QString m_lastError;

    QString m_currentKeyword;
    SearchMode m_currentMode;
    QHash<int, QList<SearchResult>> m_pendingResults;
    QStringList m_pendingErrors;
    int m_completedSources;
    int m_totalSources;
    qint64 m_searchStartTime;

    QHash<QString, QList<SearchResult>> m_searchCache;
    QTimer *m_cacheCleanupTimer;

    mutable QMutex m_mutex;

    QHash<QNetworkReply*, int> m_replySourceMap;
    QList<QNetworkReply*> m_activeReplies;
};

#endif // NOVELSEARCHER_H
