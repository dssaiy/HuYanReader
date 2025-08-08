#ifndef CONTENTPARSER_H
#define CONTENTPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QRegularExpression>
#include <QJsonObject>
#include <QUrl>
#include "../novel/NovelModels.h"
#include "RuleManager.h"
#include "LexborHtmlParser.h"

/**
 * @brief HTML Content Parser Class
 * Responsible for extracting search results, chapter lists, chapter content, etc. from HTML pages based on book source rules
 */
class ContentParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Content type enumeration
     */
    enum ContentType {
        TEXT,           // Text content
        HTML,           // HTML content
        ATTR_HREF,      // href attribute
        ATTR_SRC,       // src attribute
        ATTR_CONTENT,   // content attribute
        ATTR_VALUE      // value attribute
    };

    /**
     * @brief Parse result structure
     */
    struct ParseResult {
        bool success;
        QString error;
        QStringList extractedTexts;
        
        ParseResult() : success(false) {}
        ParseResult(bool s, const QString &e = QString()) : success(s), error(e) {}
    };

    explicit ContentParser(QObject *parent = nullptr);
    ~ContentParser();

    // Search result parsing
    QList<SearchResult> parseSearchResults(const QString &html, const BookSource &source, const QString &baseUrl = QString());
    QList<SearchResult> parseSearchResults(const QString &html, const SearchRule &rule, int sourceId, const QString &baseUrl = QString());

    // Book details parsing
    Book parseBookDetails(const QString &html, const BookSource &source, const QString &bookUrl = QString());
    Book parseBookDetails(const QString &html, const BookRule &rule, const QString &bookUrl = QString());

    // Chapter list parsing
    QList<Chapter> parseChapterList(const QString &html, const BookSource &source, const QString &baseUrl = QString());
    QList<Chapter> parseChapterList(const QString &html, const TocRule &rule, const QString &baseUrl = QString());

    // Chapter content parsing
    QString parseChapterContent(const QString &html, const BookSource &source);
    QString parseChapterContent(const QString &html, const ChapterRule &rule);

    // Pagination handling
    QStringList parseNextPageUrls(const QString &html, const QString &nextPageSelector, const QString &baseUrl = QString());
    QString parseNextPageUrl(const QString &html, const QString &nextPageSelector, const QString &baseUrl = QString());
    QList<Chapter> parseChapterListWithPagination(const QString &html, const TocRule &rule, const QString &baseUrl);
    QList<Chapter> parseChapterListSinglePage(const QString &html, const TocRule &rule, const QString &baseUrl);
    QString parseChapterContentWithPagination(const QString &html, const ChapterRule &rule, const QString &baseUrl);
    QString parseChapterContentSinglePage(const QString &html, const ChapterRule &rule);

    // Generic content extraction
    ParseResult extractContent(const QString &html, const QString &selector, ContentType type = TEXT);
    QStringList extractMultipleContent(const QString &html, const QString &selector, ContentType type = TEXT);
    QString extractSingleContent(const QString &html, const QString &selector, ContentType type = TEXT);

    // Text cleaning and formatting
    QString cleanText(const QString &text);
    QString cleanHtml(const QString &html);
    QString removeHtmlTags(const QString &html, const QStringList &tagsToRemove = QStringList());
    QString filterContent(const QString &content, const QString &filterText, const QString &filterTags);
    QString formatChapterContent(const QString &content, const ChapterRule &rule);

    // URL handling
    QString resolveUrl(const QString &url, const QString &baseUrl);
    QStringList resolveUrls(const QStringList &urls, const QString &baseUrl);

    // Validation and debugging
    bool validateSelector(const QString &selector);
    QString getLastError() const { return m_lastError; }
    void setDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }

    // Rule manager
    void setRuleManager(RuleManager *ruleManager) { m_ruleManager = ruleManager; }
    RuleManager* getRuleManager() const { return m_ruleManager; }

signals:
    void parseProgress(int current, int total);
    void parseError(const QString &error);
    void debugMessage(const QString &message);

private:
    // Internal parsing methods
    ParseResult parseWithSelector(const QString &html, const QString &selector, ContentType type);
    QString extractByRegex(const QString &html, const QRegularExpression &regex, ContentType type);

    // Improved Lexbor parsing method
    QList<SearchResult> parseSearchResultsWithLexbor(LexborHtmlParser& parser, const SearchRule& rule, int sourceId, const QString& baseUrl);
    QStringList extractMultipleByRegex(const QString &html, const QRegularExpression &regex, ContentType type);
    QString extractAttributeFromMatch(const QRegularExpressionMatch &match, const QString &attrName);
    
    // Selector handling
    QRegularExpression selectorToRegex(const QString &selector);
    ContentType detectContentType(const QString &selector);

    // Chapter filtering
    bool isNonChapterLink(const QString &title, const QString &url);

    // Fallback chapter parsing method
    QList<Chapter> parseChapterListWithRegex(const QString &html, const TocRule &rule, const QString &baseUrl);
    QString preprocessHtml(const QString &html);

    // Special processing for specific book sources
    QString applySpecialProcessing(const QString &content, const ChapterRule &rule);
    QString processJavaScriptRule(const QString &content, const QString &selector);
    QString processBase64Decryption(const QString &content);
    QString decodeBase64Content(const QString &encodedContent);
    
    // Text processing
    QString cleanInvisibleChars(const QString &text);
    QString unescapeHtml(const QString &html);
    QString normalizeWhitespace(const QString &text);
    QString removeDuplicateTitle(const QString &content, const QString &title);
    
    // URL processing
    QString normalizeUrl(const QString &url, const QString &baseUrl);
    bool isAbsoluteUrl(const QString &url);

    // Error handling
    void setError(const QString &error);
    void debugLog(const QString &message);

    // Member variables
    QString m_lastError;                    // Last error message
    bool m_debugMode;                       // Debug mode
    RuleManager *m_ruleManager;             // Rule manager (optional)

    // Precompiled regular expressions
    QRegularExpression m_htmlTagRegex;      // HTML tag regex
    QRegularExpression m_whitespaceRegex;   // Whitespace regex
    QRegularExpression m_entityRegex;       // HTML entity regex
    QRegularExpression m_invisibleRegex;    // Invisible character regex

    // Common selector cache
    QHash<QString, QRegularExpression> m_selectorCache;
};

#endif // CONTENTPARSER_H
