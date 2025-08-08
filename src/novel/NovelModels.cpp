#include "NovelModels.h"
#include <QJsonDocument>
#include <QDebug>

// Chapter implementation

Chapter::Chapter()
    : m_order(0)
{
}

Chapter::Chapter(const QString &url, const QString &title, int order)
    : m_url(url), m_title(title), m_order(order)
{
}

Chapter::Chapter(const Chapter &other)
    : m_url(other.m_url)
    , m_title(other.m_title)
    , m_content(other.m_content)
    , m_order(other.m_order)
{
}

Chapter& Chapter::operator=(const Chapter &other)
{
    if (this != &other) {
        m_url = other.m_url;
        m_title = other.m_title;
        m_content = other.m_content;
        m_order = other.m_order;
    }
    return *this;
}

bool Chapter::operator==(const Chapter &other) const
{
    return m_url == other.m_url && m_title == other.m_title && m_order == other.m_order;
}

bool Chapter::operator<(const Chapter &other) const
{
    return m_order < other.m_order;
}

QJsonObject Chapter::toJson() const
{
    QJsonObject json;
    json["url"] = m_url;
    json["title"] = m_title;
    json["content"] = m_content;
    json["order"] = m_order;
    return json;
}

void Chapter::fromJson(const QJsonObject &json)
{
    m_url = json["url"].toString();
    m_title = json["title"].toString();
    m_content = json["content"].toString();
    m_order = json["order"].toInt();
}

Chapter Chapter::fromJsonStatic(const QJsonObject &json)
{
    Chapter chapter;
    chapter.fromJson(json);
    return chapter;
}

QString Chapter::toString() const
{
    return QString("Chapter(url=%1, title=%2, order=%3)")
           .arg(m_url, m_title)
           .arg(m_order);
}

// Book implementation

Book::Book()
{
}

Book::Book(const Book &other)
    : m_url(other.m_url)
    , m_bookName(other.m_bookName)
    , m_author(other.m_author)
    , m_intro(other.m_intro)
    , m_category(other.m_category)
    , m_coverUrl(other.m_coverUrl)
    , m_latestChapter(other.m_latestChapter)
    , m_lastUpdateTime(other.m_lastUpdateTime)
    , m_status(other.m_status)
    , m_wordCount(other.m_wordCount)
{
}

Book& Book::operator=(const Book &other)
{
    if (this != &other) {
        m_url = other.m_url;
        m_bookName = other.m_bookName;
        m_author = other.m_author;
        m_intro = other.m_intro;
        m_category = other.m_category;
        m_coverUrl = other.m_coverUrl;
        m_latestChapter = other.m_latestChapter;
        m_lastUpdateTime = other.m_lastUpdateTime;
        m_status = other.m_status;
        m_wordCount = other.m_wordCount;
    }
    return *this;
}

bool Book::operator==(const Book &other) const
{
    return m_url == other.m_url && m_bookName == other.m_bookName && m_author == other.m_author;
}

QJsonObject Book::toJson() const
{
    QJsonObject json;
    json["url"] = m_url;
    json["bookName"] = m_bookName;
    json["author"] = m_author;
    json["intro"] = m_intro;
    json["category"] = m_category;
    json["coverUrl"] = m_coverUrl;
    json["latestChapter"] = m_latestChapter;
    json["lastUpdateTime"] = m_lastUpdateTime;
    json["status"] = m_status;
    json["wordCount"] = m_wordCount;
    return json;
}

void Book::fromJson(const QJsonObject &json)
{
    m_url = json["url"].toString();
    m_bookName = json["bookName"].toString();
    m_author = json["author"].toString();
    m_intro = json["intro"].toString();
    m_category = json["category"].toString();
    m_coverUrl = json["coverUrl"].toString();
    m_latestChapter = json["latestChapter"].toString();
    m_lastUpdateTime = json["lastUpdateTime"].toString();
    m_status = json["status"].toString();
    m_wordCount = json["wordCount"].toString();
}

Book Book::fromJsonStatic(const QJsonObject &json)
{
    Book book;
    book.fromJson(json);
    return book;
}

QString Book::toString() const
{
    return QString("Book(url=%1, bookName=%2, author=%3, category=%4)")
           .arg(m_url, m_bookName, m_author, m_category);
}

// SearchResult implementation

SearchResult::SearchResult()
    : m_sourceId(-1)
{
}

SearchResult::SearchResult(const SearchResult &other)
    : m_sourceId(other.m_sourceId)
    , m_url(other.m_url)
    , m_bookName(other.m_bookName)
    , m_author(other.m_author)
    , m_intro(other.m_intro)
    , m_category(other.m_category)
    , m_latestChapter(other.m_latestChapter)
    , m_lastUpdateTime(other.m_lastUpdateTime)
    , m_status(other.m_status)
    , m_wordCount(other.m_wordCount)
    , m_sourceName(other.m_sourceName)
{
}

SearchResult& SearchResult::operator=(const SearchResult &other)
{
    if (this != &other) {
        m_sourceId = other.m_sourceId;
        m_url = other.m_url;
        m_bookName = other.m_bookName;
        m_author = other.m_author;
        m_intro = other.m_intro;
        m_category = other.m_category;
        m_latestChapter = other.m_latestChapter;
        m_lastUpdateTime = other.m_lastUpdateTime;
        m_status = other.m_status;
        m_wordCount = other.m_wordCount;
        m_sourceName = other.m_sourceName;
    }
    return *this;
}

bool SearchResult::operator==(const SearchResult &other) const
{
    return m_url == other.m_url && m_bookName == other.m_bookName && m_author == other.m_author;
}

Book SearchResult::toBook() const
{
    Book book;
    book.setUrl(m_url);
    book.setBookName(m_bookName);
    book.setAuthor(m_author);
    book.setIntro(m_intro);
    book.setCategory(m_category);
    book.setLatestChapter(m_latestChapter);
    book.setLastUpdateTime(m_lastUpdateTime);
    book.setStatus(m_status);
    book.setWordCount(m_wordCount);
    return book;
}

QJsonObject SearchResult::toJson() const
{
    QJsonObject json;
    json["sourceId"] = m_sourceId;
    json["url"] = m_url;
    json["bookName"] = m_bookName;
    json["author"] = m_author;
    json["intro"] = m_intro;
    json["category"] = m_category;
    json["latestChapter"] = m_latestChapter;
    json["lastUpdateTime"] = m_lastUpdateTime;
    json["status"] = m_status;
    json["wordCount"] = m_wordCount;
    json["sourceName"] = m_sourceName;
    return json;
}

void SearchResult::fromJson(const QJsonObject &json)
{
    m_sourceId = json["sourceId"].toInt(-1);
    m_url = json["url"].toString();
    m_bookName = json["bookName"].toString();
    m_author = json["author"].toString();
    m_intro = json["intro"].toString();
    m_category = json["category"].toString();
    m_latestChapter = json["latestChapter"].toString();
    m_lastUpdateTime = json["lastUpdateTime"].toString();
    m_status = json["status"].toString();
    m_wordCount = json["wordCount"].toString();
    m_sourceName = json["sourceName"].toString();
}

SearchResult SearchResult::fromJsonStatic(const QJsonObject &json)
{
    SearchResult result;
    result.fromJson(json);
    return result;
}

QString SearchResult::toString() const
{
    return QString("SearchResult(sourceId=%1, url=%2, bookName=%3, author=%4)")
           .arg(m_sourceId)
           .arg(m_url, m_bookName, m_author);
}

// SearchRule implementation

SearchRule::SearchRule()
    : m_disabled(false)
    , m_timeout(30)
    , m_method("GET")
    , m_pagination(false)
{
}

QJsonObject SearchRule::toJson() const
{
    QJsonObject json;
    json["disabled"] = m_disabled;
    json["baseUri"] = m_baseUri;
    json["timeout"] = m_timeout;
    json["url"] = m_url;
    json["method"] = m_method;
    json["data"] = m_data;
    json["cookies"] = m_cookies;
    json["result"] = m_result;
    json["bookName"] = m_bookName;
    json["author"] = m_author;
    json["category"] = m_category;
    json["latestChapter"] = m_latestChapter;
    json["lastUpdateTime"] = m_lastUpdateTime;
    json["status"] = m_status;
    json["wordCount"] = m_wordCount;
    json["pagination"] = m_pagination;
    json["nextPage"] = m_nextPage;
    return json;
}

void SearchRule::fromJson(const QJsonObject &json)
{
    m_disabled = json["disabled"].toBool(false);
    m_baseUri = json["baseUri"].toString();
    m_timeout = json["timeout"].toInt(30);
    m_url = json["url"].toString();
    m_method = json["method"].toString("GET");
    m_data = json["data"].toString();
    m_cookies = json["cookies"].toString();
    m_result = json["result"].toString();
    m_bookName = json["bookName"].toString();
    m_author = json["author"].toString();
    m_category = json["category"].toString();
    m_latestChapter = json["latestChapter"].toString();
    m_lastUpdateTime = json["lastUpdateTime"].toString();
    m_status = json["status"].toString();
    m_wordCount = json["wordCount"].toString();
    m_pagination = json["pagination"].toBool(false);
    m_nextPage = json["nextPage"].toString();
}

// BookRule implementation

BookRule::BookRule()
    : m_timeout(15)
{
}

QJsonObject BookRule::toJson() const
{
    QJsonObject json;
    json["baseUri"] = m_baseUri;
    json["timeout"] = m_timeout;
    json["url"] = m_url;
    json["bookName"] = m_bookName;
    json["author"] = m_author;
    json["intro"] = m_intro;
    json["category"] = m_category;
    json["coverUrl"] = m_coverUrl;
    json["latestChapter"] = m_latestChapter;
    json["lastUpdateTime"] = m_lastUpdateTime;
    json["status"] = m_status;
    json["wordCount"] = m_wordCount;
    return json;
}

void BookRule::fromJson(const QJsonObject &json)
{
    m_baseUri = json["baseUri"].toString();
    m_timeout = json["timeout"].toInt(15);
    m_url = json["url"].toString();
    m_bookName = json["bookName"].toString();
    m_author = json["author"].toString();
    m_intro = json["intro"].toString();
    m_category = json["category"].toString();
    m_coverUrl = json["coverUrl"].toString();
    m_latestChapter = json["latestChapter"].toString();
    m_lastUpdateTime = json["lastUpdateTime"].toString();
    m_status = json["status"].toString();
    m_wordCount = json["wordCount"].toString();
}

// TocRule implementation

TocRule::TocRule()
    : m_timeout(30)
    , m_isDesc(false)
    , m_pagination(false)
{
}

QJsonObject TocRule::toJson() const
{
    QJsonObject json;
    json["baseUri"] = m_baseUri;
    json["timeout"] = m_timeout;
    json["url"] = m_url;
    json["list"] = m_list;
    json["item"] = m_item;
    json["isDesc"] = m_isDesc;
    json["pagination"] = m_pagination;
    json["nextPage"] = m_nextPage;
    return json;
}

void TocRule::fromJson(const QJsonObject &json)
{
    m_baseUri = json["baseUri"].toString();
    m_timeout = json["timeout"].toInt(30);
    m_url = json["url"].toString();
    m_list = json["list"].toString();
    m_item = json["item"].toString();
    m_isDesc = json["isDesc"].toBool(false);
    m_pagination = json["pagination"].toBool(false);
    m_nextPage = json["nextPage"].toString();
}

// ChapterRule implementation

ChapterRule::ChapterRule()
    : m_timeout(10)
    , m_paragraphTagClosed(false)
    , m_pagination(false)
{
}

QJsonObject ChapterRule::toJson() const
{
    QJsonObject json;
    json["baseUri"] = m_baseUri;
    json["timeout"] = m_timeout;
    json["title"] = m_title;
    json["content"] = m_content;
    json["paragraphTagClosed"] = m_paragraphTagClosed;
    json["paragraphTag"] = m_paragraphTag;
    json["filterTxt"] = m_filterTxt;
    json["filterTag"] = m_filterTag;
    json["pagination"] = m_pagination;
    json["nextPage"] = m_nextPage;
    json["nextPageInJs"] = m_nextPageInJs;
    json["nextChapterLink"] = m_nextChapterLink;
    return json;
}

void ChapterRule::fromJson(const QJsonObject &json)
{
    m_baseUri = json["baseUri"].toString();
    m_timeout = json["timeout"].toInt(10);
    m_title = json["title"].toString();
    m_content = json["content"].toString();
    m_paragraphTagClosed = json["paragraphTagClosed"].toBool(false);
    m_paragraphTag = json["paragraphTag"].toString();
    m_filterTxt = json["filterTxt"].toString();
    m_filterTag = json["filterTag"].toString();
    m_pagination = json["pagination"].toBool(false);
    m_nextPage = json["nextPage"].toString();
    m_nextPageInJs = json["nextPageInJs"].toString();
    m_nextChapterLink = json["nextChapterLink"].toString();
}

// CrawlRule implementation

CrawlRule::CrawlRule()
    : m_threads(2)  // Enable 2 threads with new thread-safe architecture
    , m_minInterval(1000)
    , m_maxInterval(2000)
    , m_maxAttempts(3)
    , m_retryMinInterval(2000)
    , m_retryMaxInterval(4000)
{
}

QJsonObject CrawlRule::toJson() const
{
    QJsonObject json;
    json["threads"] = m_threads;
    json["minInterval"] = m_minInterval;
    json["maxInterval"] = m_maxInterval;
    json["maxAttempts"] = m_maxAttempts;
    json["retryMinInterval"] = m_retryMinInterval;
    json["retryMaxInterval"] = m_retryMaxInterval;
    return json;
}

void CrawlRule::fromJson(const QJsonObject &json)
{
    m_threads = json["threads"].toInt(2);  // Default to 2 threads with new thread-safe architecture
    m_minInterval = json["minInterval"].toInt(1000);
    m_maxInterval = json["maxInterval"].toInt(2000);
    m_maxAttempts = json["maxAttempts"].toInt(3);
    m_retryMinInterval = json["retryMinInterval"].toInt(2000);
    m_retryMaxInterval = json["retryMaxInterval"].toInt(4000);
}

// BookSource implementation

BookSource::BookSource()
    : m_id(-1)
    , m_needProxy(false)
    , m_disabled(false)
{
}

BookSource::BookSource(const BookSource &other)
    : m_id(other.m_id)
    , m_url(other.m_url)
    , m_name(other.m_name)
    , m_comment(other.m_comment)
    , m_language(other.m_language)
    , m_needProxy(other.m_needProxy)
    , m_disabled(other.m_disabled)
    , m_searchRule(other.m_searchRule)
    , m_bookRule(other.m_bookRule)
    , m_tocRule(other.m_tocRule)
    , m_chapterRule(other.m_chapterRule)
    , m_crawlRule(other.m_crawlRule)
{
}

BookSource& BookSource::operator=(const BookSource &other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_url = other.m_url;
        m_name = other.m_name;
        m_comment = other.m_comment;
        m_language = other.m_language;
        m_needProxy = other.m_needProxy;
        m_disabled = other.m_disabled;
        m_searchRule = other.m_searchRule;
        m_bookRule = other.m_bookRule;
        m_tocRule = other.m_tocRule;
        m_chapterRule = other.m_chapterRule;
        m_crawlRule = other.m_crawlRule;
    }
    return *this;
}

bool BookSource::operator==(const BookSource &other) const
{
    return m_id == other.m_id && m_url == other.m_url && m_name == other.m_name;
}

bool BookSource::hasSearch() const
{
    return !m_searchRule.url().isEmpty() && !m_searchRule.result().isEmpty();
}

bool BookSource::isValid() const
{
    return m_id >= 0 && !m_url.isEmpty() && !m_name.isEmpty();
}

bool BookSource::matchesUrl(const QString &url) const
{
    return url.startsWith(m_url);
}

QJsonObject BookSource::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["url"] = m_url;
    json["name"] = m_name;
    json["comment"] = m_comment;
    json["language"] = m_language;
    json["needProxy"] = m_needProxy;
    json["disabled"] = m_disabled;


    if (hasSearch()) {
        json["search"] = m_searchRule.toJson();
    }

    json["book"] = m_bookRule.toJson();
    json["toc"] = m_tocRule.toJson();
    json["chapter"] = m_chapterRule.toJson();
    json["crawl"] = m_crawlRule.toJson();

    return json;
}

void BookSource::fromJson(const QJsonObject &json)
{
    m_id = json["id"].toInt(-1);
    m_url = json["url"].toString();
    m_name = json["name"].toString();
    m_comment = json["comment"].toString();
    m_language = json["language"].toString();
    m_needProxy = json["needProxy"].toBool(false);
    m_disabled = json["disabled"].toBool(false);


    if (json.contains("search")) {
        m_searchRule.fromJson(json["search"].toObject());
    }

    if (json.contains("book")) {
        m_bookRule.fromJson(json["book"].toObject());
    }

    if (json.contains("toc")) {
        m_tocRule.fromJson(json["toc"].toObject());
    }

    if (json.contains("chapter")) {
        m_chapterRule.fromJson(json["chapter"].toObject());
    }

    if (json.contains("crawl")) {
        m_crawlRule.fromJson(json["crawl"].toObject());
    }
}

BookSource BookSource::fromJsonStatic(const QJsonObject &json)
{
    BookSource source;
    source.fromJson(json);
    return source;
}

QString BookSource::toString() const
{
    return QString("BookSource(id=%1, name=%2, url=%3, hasSearch=%4)")
           .arg(m_id)
           .arg(m_name, m_url)
           .arg(hasSearch() ? "true" : "false");
}
