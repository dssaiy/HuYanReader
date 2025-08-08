#ifndef NOVELMODELS_H
#define NOVELMODELS_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QMetaType>

/**
 * @brief Chapter information class
 */
class Chapter
{
public:
    Chapter();
    Chapter(const QString &url, const QString &title = QString(), int order = 0);
    Chapter(const Chapter &other);
    Chapter& operator=(const Chapter &other);
    bool operator==(const Chapter &other) const;
    bool operator<(const Chapter &other) const;

    // Getters
    QString url() const { return m_url; }
    QString title() const { return m_title; }
    QString content() const { return m_content; }
    int order() const { return m_order; }
    bool isEmpty() const { return m_url.isEmpty(); }
    bool isValid() const { return !m_url.isEmpty() && !m_title.isEmpty(); }

    // Setters
    void setUrl(const QString &url) { m_url = url; }
    void setTitle(const QString &title) { m_title = title; }
    void setContent(const QString &content) { m_content = content; }
    void setOrder(int order) { m_order = order; }
    void setIndex(int index) { m_order = index; } // Alias for setOrder

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    static Chapter fromJsonStatic(const QJsonObject &json);

    // Debug output
    QString toString() const;

private:
    QString m_url;
    QString m_title;
    QString m_content;
    int m_order;
};

/**
 * @brief Novel information class
 */
class Book
{
public:
    Book();
    Book(const Book &other);
    Book& operator=(const Book &other);
    bool operator==(const Book &other) const;

    // Getters
    QString url() const { return m_url; }
    QString bookName() const { return m_bookName; }
    QString author() const { return m_author; }
    QString intro() const { return m_intro; }
    QString category() const { return m_category; }
    QString coverUrl() const { return m_coverUrl; }
    QString latestChapter() const { return m_latestChapter; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    QString status() const { return m_status; }
    QString wordCount() const { return m_wordCount; }
    bool isEmpty() const { return m_url.isEmpty() && m_bookName.isEmpty(); }
    bool isValid() const { return !m_url.isEmpty() && !m_bookName.isEmpty() && !m_author.isEmpty(); }

    // Setters
    void setUrl(const QString &url) { m_url = url; }
    void setBookName(const QString &bookName) { m_bookName = bookName; }
    void setAuthor(const QString &author) { m_author = author; }
    void setIntro(const QString &intro) { m_intro = intro; }
    void setCategory(const QString &category) { m_category = category; }
    void setCoverUrl(const QString &coverUrl) { m_coverUrl = coverUrl; }
    void setLatestChapter(const QString &latestChapter) { m_latestChapter = latestChapter; }
    void setLastUpdateTime(const QString &lastUpdateTime) { m_lastUpdateTime = lastUpdateTime; }
    void setStatus(const QString &status) { m_status = status; }
    void setWordCount(const QString &wordCount) { m_wordCount = wordCount; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    static Book fromJsonStatic(const QJsonObject &json);

    // Debug output
    QString toString() const;

private:
    QString m_url;
    QString m_bookName;
    QString m_author;
    QString m_intro;
    QString m_category;
    QString m_coverUrl;
    QString m_latestChapter;
    QString m_lastUpdateTime;
    QString m_status;
    QString m_wordCount;
};

/**
 * @brief Search result class
 */
class SearchResult
{
public:
    SearchResult();
    SearchResult(const SearchResult &other);
    SearchResult& operator=(const SearchResult &other);
    bool operator==(const SearchResult &other) const;

    // Getters
    int sourceId() const { return m_sourceId; }
    QString url() const { return m_url; }
    QString bookName() const { return m_bookName; }
    QString author() const { return m_author; }
    QString intro() const { return m_intro; }
    QString category() const { return m_category; }
    QString latestChapter() const { return m_latestChapter; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    QString status() const { return m_status; }
    QString wordCount() const { return m_wordCount; }
    QString sourceName() const { return m_sourceName; }
    QString bookUrl() const { return m_url; } // Alias for url()
    bool isEmpty() const { return m_url.isEmpty() && m_bookName.isEmpty(); }
    bool isValid() const { return !m_url.isEmpty() && !m_bookName.isEmpty() && !m_author.isEmpty(); }

    // Setters
    void setSourceId(int sourceId) { m_sourceId = sourceId; }
    void setUrl(const QString &url) { m_url = url; }
    void setBookName(const QString &bookName) { m_bookName = bookName; }
    void setAuthor(const QString &author) { m_author = author; }
    void setIntro(const QString &intro) { m_intro = intro; }
    void setCategory(const QString &category) { m_category = category; }
    void setLatestChapter(const QString &latestChapter) { m_latestChapter = latestChapter; }
    void setLastUpdateTime(const QString &lastUpdateTime) { m_lastUpdateTime = lastUpdateTime; }
    void setStatus(const QString &status) { m_status = status; }
    void setWordCount(const QString &wordCount) { m_wordCount = wordCount; }
    void setSourceName(const QString &sourceName) { m_sourceName = sourceName; }

    // Convert to Book object
    Book toBook() const;

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    static SearchResult fromJsonStatic(const QJsonObject &json);

    // Debug output
    QString toString() const;

private:
    int m_sourceId;
    QString m_url;
    QString m_bookName;
    QString m_author;
    QString m_intro;
    QString m_category;
    QString m_latestChapter;
    QString m_lastUpdateTime;
    QString m_status;
    QString m_wordCount;
    QString m_sourceName;
};

/**
 * @brief Book source rule class - Search rule
 */
class SearchRule
{
public:
    SearchRule();

    // Getters
    bool disabled() const { return m_disabled; }
    QString baseUri() const { return m_baseUri; }
    int timeout() const { return m_timeout; }
    QString url() const { return m_url; }
    QString method() const { return m_method; }
    QString data() const { return m_data; }
    QString cookies() const { return m_cookies; }
    QString result() const { return m_result; }
    QString bookName() const { return m_bookName; }
    QString author() const { return m_author; }
    QString category() const { return m_category; }
    QString latestChapter() const { return m_latestChapter; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    QString status() const { return m_status; }
    QString wordCount() const { return m_wordCount; }
    bool pagination() const { return m_pagination; }
    QString nextPage() const { return m_nextPage; }

    // Setters
    void setDisabled(bool disabled) { m_disabled = disabled; }
    void setBaseUri(const QString &baseUri) { m_baseUri = baseUri; }
    void setTimeout(int timeout) { m_timeout = timeout; }
    void setUrl(const QString &url) { m_url = url; }
    void setMethod(const QString &method) { m_method = method; }
    void setData(const QString &data) { m_data = data; }
    void setCookies(const QString &cookies) { m_cookies = cookies; }
    void setResult(const QString &result) { m_result = result; }
    void setBookName(const QString &bookName) { m_bookName = bookName; }
    void setAuthor(const QString &author) { m_author = author; }
    void setCategory(const QString &category) { m_category = category; }
    void setLatestChapter(const QString &latestChapter) { m_latestChapter = latestChapter; }
    void setLastUpdateTime(const QString &lastUpdateTime) { m_lastUpdateTime = lastUpdateTime; }
    void setStatus(const QString &status) { m_status = status; }
    void setWordCount(const QString &wordCount) { m_wordCount = wordCount; }
    void setPagination(bool pagination) { m_pagination = pagination; }
    void setNextPage(const QString &nextPage) { m_nextPage = nextPage; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    bool m_disabled;
    QString m_baseUri;
    int m_timeout;
    QString m_url;
    QString m_method;
    QString m_data;
    QString m_cookies;
    QString m_result;
    QString m_bookName;
    QString m_author;
    QString m_category;
    QString m_latestChapter;
    QString m_lastUpdateTime;
    QString m_status;
    QString m_wordCount;
    bool m_pagination;
    QString m_nextPage;
};

/**
 * @brief Book source rule class - Book details rule
 */
class BookRule
{
public:
    BookRule();

    // Getters
    QString baseUri() const { return m_baseUri; }
    int timeout() const { return m_timeout; }
    QString url() const { return m_url; }
    QString bookName() const { return m_bookName; }
    QString author() const { return m_author; }
    QString intro() const { return m_intro; }
    QString category() const { return m_category; }
    QString coverUrl() const { return m_coverUrl; }
    QString latestChapter() const { return m_latestChapter; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    QString status() const { return m_status; }
    QString wordCount() const { return m_wordCount; }

    // Setters
    void setBaseUri(const QString &baseUri) { m_baseUri = baseUri; }
    void setTimeout(int timeout) { m_timeout = timeout; }
    void setUrl(const QString &url) { m_url = url; }
    void setBookName(const QString &bookName) { m_bookName = bookName; }
    void setAuthor(const QString &author) { m_author = author; }
    void setIntro(const QString &intro) { m_intro = intro; }
    void setCategory(const QString &category) { m_category = category; }
    void setCoverUrl(const QString &coverUrl) { m_coverUrl = coverUrl; }
    void setLatestChapter(const QString &latestChapter) { m_latestChapter = latestChapter; }
    void setLastUpdateTime(const QString &lastUpdateTime) { m_lastUpdateTime = lastUpdateTime; }
    void setStatus(const QString &status) { m_status = status; }
    void setWordCount(const QString &wordCount) { m_wordCount = wordCount; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    QString m_baseUri;
    int m_timeout;
    QString m_url;
    QString m_bookName;
    QString m_author;
    QString m_intro;
    QString m_category;
    QString m_coverUrl;
    QString m_latestChapter;
    QString m_lastUpdateTime;
    QString m_status;
    QString m_wordCount;
};

/**
 * @brief Book source rule class - Table of contents rule
 */
class TocRule
{
public:
    TocRule();

    // Getters
    QString baseUri() const { return m_baseUri; }
    int timeout() const { return m_timeout; }
    QString url() const { return m_url; }
    QString list() const { return m_list; }
    QString item() const { return m_item; }
    bool isDesc() const { return m_isDesc; }
    bool pagination() const { return m_pagination; }
    QString nextPage() const { return m_nextPage; }

    // Setters
    void setBaseUri(const QString &baseUri) { m_baseUri = baseUri; }
    void setTimeout(int timeout) { m_timeout = timeout; }
    void setUrl(const QString &url) { m_url = url; }
    void setList(const QString &list) { m_list = list; }
    void setItem(const QString &item) { m_item = item; }
    void setIsDesc(bool isDesc) { m_isDesc = isDesc; }
    void setPagination(bool pagination) { m_pagination = pagination; }
    void setNextPage(const QString &nextPage) { m_nextPage = nextPage; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    QString m_baseUri;
    int m_timeout;
    QString m_url;
    QString m_list;
    QString m_item;
    bool m_isDesc;
    bool m_pagination;
    QString m_nextPage;
};

/**
 * @brief Book source rule class - Chapter rule
 */
class ChapterRule
{
public:
    ChapterRule();

    // Getters
    QString baseUri() const { return m_baseUri; }
    int timeout() const { return m_timeout; }
    QString title() const { return m_title; }
    QString content() const { return m_content; }
    bool paragraphTagClosed() const { return m_paragraphTagClosed; }
    QString paragraphTag() const { return m_paragraphTag; }
    QString filterTxt() const { return m_filterTxt; }
    QString filterTag() const { return m_filterTag; }
    bool pagination() const { return m_pagination; }
    QString nextPage() const { return m_nextPage; }
    QString nextPageInJs() const { return m_nextPageInJs; }
    QString nextChapterLink() const { return m_nextChapterLink; }

    // Setters
    void setBaseUri(const QString &baseUri) { m_baseUri = baseUri; }
    void setTimeout(int timeout) { m_timeout = timeout; }
    void setTitle(const QString &title) { m_title = title; }
    void setContent(const QString &content) { m_content = content; }
    void setParagraphTagClosed(bool closed) { m_paragraphTagClosed = closed; }
    void setParagraphTag(const QString &tag) { m_paragraphTag = tag; }
    void setFilterTxt(const QString &filter) { m_filterTxt = filter; }
    void setFilterTag(const QString &filter) { m_filterTag = filter; }
    void setPagination(bool pagination) { m_pagination = pagination; }
    void setNextPage(const QString &nextPage) { m_nextPage = nextPage; }
    void setNextPageInJs(const QString &nextPageInJs) { m_nextPageInJs = nextPageInJs; }
    void setNextChapterLink(const QString &link) { m_nextChapterLink = link; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    QString m_baseUri;
    int m_timeout;
    QString m_title;
    QString m_content;
    bool m_paragraphTagClosed;
    QString m_paragraphTag;
    QString m_filterTxt;
    QString m_filterTag;
    bool m_pagination;
    QString m_nextPage;
    QString m_nextPageInJs;
    QString m_nextChapterLink;
};

/**
 * @brief Crawl configuration rule
 */
class CrawlRule
{
public:
    CrawlRule();

    // Getters
    int threads() const { return m_threads; }
    int minInterval() const { return m_minInterval; }
    int maxInterval() const { return m_maxInterval; }
    int maxAttempts() const { return m_maxAttempts; }
    int retryMinInterval() const { return m_retryMinInterval; }
    int retryMaxInterval() const { return m_retryMaxInterval; }

    // Setters
    void setThreads(int threads) { m_threads = threads; }
    void setMinInterval(int interval) { m_minInterval = interval; }
    void setMaxInterval(int interval) { m_maxInterval = interval; }
    void setMaxAttempts(int attempts) { m_maxAttempts = attempts; }
    void setRetryMinInterval(int interval) { m_retryMinInterval = interval; }
    void setRetryMaxInterval(int interval) { m_retryMaxInterval = interval; }

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    int m_threads;
    int m_minInterval;
    int m_maxInterval;
    int m_maxAttempts;
    int m_retryMinInterval;
    int m_retryMaxInterval;
};

/**
 * @brief Complete book source class
 */
class BookSource
{
public:
    BookSource();
    BookSource(const BookSource &other);
    BookSource& operator=(const BookSource &other);
    bool operator==(const BookSource &other) const;

    // Basic information
    int id() const { return m_id; }
    QString url() const { return m_url; }
    QString name() const { return m_name; }
    QString comment() const { return m_comment; }
    QString language() const { return m_language; }
    bool needProxy() const { return m_needProxy; }
    bool disabled() const { return m_disabled; }

    void setId(int id) { m_id = id; }
    void setUrl(const QString &url) { m_url = url; }
    void setName(const QString &name) { m_name = name; }
    void setComment(const QString &comment) { m_comment = comment; }
    void setLanguage(const QString &language) { m_language = language; }
    void setNeedProxy(bool needProxy) { m_needProxy = needProxy; }
    void setDisabled(bool disabled) { m_disabled = disabled; }

    // Rule access
    SearchRule* searchRule() { return &m_searchRule; }
    const SearchRule* searchRule() const { return &m_searchRule; }
    BookRule* bookRule() { return &m_bookRule; }
    const BookRule* bookRule() const { return &m_bookRule; }
    TocRule* tocRule() { return &m_tocRule; }
    const TocRule* tocRule() const { return &m_tocRule; }
    ChapterRule* chapterRule() { return &m_chapterRule; }
    const ChapterRule* chapterRule() const { return &m_chapterRule; }
    CrawlRule* crawlRule() { return &m_crawlRule; }
    const CrawlRule* crawlRule() const { return &m_crawlRule; }

    // Convenience methods
    bool hasSearch() const;
    bool isValid() const;
    bool matchesUrl(const QString &url) const;

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    static BookSource fromJsonStatic(const QJsonObject &json);

    // Debug output
    QString toString() const;

private:
    // Basic information
    int m_id;
    QString m_url;
    QString m_name;
    QString m_comment;
    QString m_language;
    bool m_needProxy;
    bool m_disabled;

    // Rules
    SearchRule m_searchRule;
    BookRule m_bookRule;
    TocRule m_tocRule;
    ChapterRule m_chapterRule;
    CrawlRule m_crawlRule;
};

// Register meta types for QVariant
Q_DECLARE_METATYPE(Chapter)
Q_DECLARE_METATYPE(Book)
Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(SearchRule)
Q_DECLARE_METATYPE(BookRule)
Q_DECLARE_METATYPE(TocRule)
Q_DECLARE_METATYPE(ChapterRule)
Q_DECLARE_METATYPE(CrawlRule)
Q_DECLARE_METATYPE(BookSource)

#endif // NOVELMODELS_H
