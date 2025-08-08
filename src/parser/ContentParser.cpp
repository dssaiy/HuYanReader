#include "ContentParser.h"
#include "LexborHtmlParser.h"
#include <QDebug>
#include <QUrl>
#include <QRegularExpression>
// #include <QTextDocument>
#include <QStringList>

ContentParser::ContentParser(QObject *parent)
    : QObject(parent)
    , m_debugMode(false)
    , m_ruleManager(nullptr)
{

    m_htmlTagRegex = QRegularExpression("<[^>]*>", QRegularExpression::CaseInsensitiveOption);
    m_whitespaceRegex = QRegularExpression("\\s+");
    m_entityRegex = QRegularExpression("&[^;]+;");
    m_invisibleRegex = QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F\\x7F]");
}

ContentParser::~ContentParser()
{
}

QList<SearchResult> ContentParser::parseSearchResults(const QString &html, const BookSource &source, const QString &baseUrl)
{
    QList<SearchResult> results = parseSearchResults(html, *source.searchRule(), source.id(), baseUrl);

    // Set source name for all results
    for (SearchResult &result : results) {
        result.setSourceName(source.name());
    }

    return results;
}

QList<SearchResult> ContentParser::parseSearchResults(const QString &html, const SearchRule &rule, int sourceId, const QString &baseUrl)
{
    QList<SearchResult> results;
    
    if (html.isEmpty() || rule.result().isEmpty()) {
        setError("HTML content or search result selector is empty");
        return results;
    }
    
    debugLog(QString("Parsing search results, selector: %1, HTML length: %2").arg(rule.result()).arg(html.length()));

    // Try using Lexbor HTML parser first
    LexborHtmlParser lexborParser;
    if (lexborParser.parseHtml(html)) {
        // Use improved parsing method that works with complete HTML document
        QList<SearchResult> lexborResults = parseSearchResultsWithLexbor(lexborParser, rule, sourceId, baseUrl);
        if (!lexborResults.isEmpty()) {
            debugLog(QString("Lexbor parsing completed, found %1 results").arg(lexborResults.size()));
            return lexborResults;
        }
    }

    // Fallback to original regex-based parsing
    debugLog("Falling back to regex-based parsing");
    QString cleanHtml = preprocessHtml(html);
    QStringList resultItems = extractMultipleContent(cleanHtml, rule.result(), HTML);
    
    debugLog(QString("Found %1 search result items").arg(resultItems.size()));

    // Output detailed regex results for comparison
    debugLog("=== REGEX DETAILED RESULTS ===");
    for (int i = 0; i < resultItems.size() && i < 10; ++i) {
        QString content = resultItems[i];
        debugLog(QString("Regex Result %1: [%2] (length: %3)").arg(i+1).arg(content.left(200)).arg(content.length()));
    }
    debugLog("=== END REGEX RESULTS ===");

    for (int i = 0; i < resultItems.size(); ++i) {
        const QString &itemHtml = resultItems[i];

        debugLog(QString("parseSearchResults - Processing item %1/%2").arg(i + 1).arg(resultItems.size()));
        debugLog(QString("parseSearchResults - Item HTML (first 200 chars): %1").arg(itemHtml.left(200)));
        debugLog(QString("parseSearchResults - BookName selector: '%1'").arg(rule.bookName()));
        debugLog(QString("parseSearchResults - Author selector: '%1'").arg(rule.author()));

        QString bookName = extractSingleContent(itemHtml, rule.bookName(), TEXT);
        QString bookUrl = extractSingleContent(itemHtml, rule.bookName(), ATTR_HREF);

        debugLog(QString("parseSearchResults - Raw bookName: '%1'").arg(bookName));
        debugLog(QString("parseSearchResults - Raw bookUrl: '%1'").arg(bookUrl));

        if (bookName.isEmpty()) {
            debugLog(QString("Skip %1 result: book title is empty").arg(i + 1));
            debugLog(QString("parseSearchResults - Failed item HTML: %1").arg(itemHtml));
            continue;
        }


        SearchResult result;
        result.setSourceId(sourceId);
        result.setBookName(cleanText(bookName));
        result.setUrl(resolveUrl(bookUrl, baseUrl));


        if (!rule.author().isEmpty()) {
            QString author = extractSingleContent(itemHtml, rule.author(), TEXT);
            debugLog(QString("parseSearchResults - Raw author: '%1'").arg(author));
            result.setAuthor(cleanText(author));
        }
        
        if (!rule.category().isEmpty()) {
            QString category = extractSingleContent(itemHtml, rule.category(), TEXT);
            result.setCategory(cleanText(category));
        }
        
        if (!rule.latestChapter().isEmpty()) {
            QString latestChapter = extractSingleContent(itemHtml, rule.latestChapter(), TEXT);
            result.setLatestChapter(cleanText(latestChapter));
        }
        
        if (!rule.lastUpdateTime().isEmpty()) {
            QString lastUpdateTime = extractSingleContent(itemHtml, rule.lastUpdateTime(), TEXT);
            result.setLastUpdateTime(cleanText(lastUpdateTime));
        }
        
        if (!rule.status().isEmpty()) {
            QString status = extractSingleContent(itemHtml, rule.status(), TEXT);
            result.setStatus(cleanText(status));
        }
        
        if (!rule.wordCount().isEmpty()) {
            QString wordCount = extractSingleContent(itemHtml, rule.wordCount(), TEXT);
            result.setWordCount(cleanText(wordCount));
        }
        
        // Set source name if RuleManager is available
        if (m_ruleManager) {
            const BookSource* source = m_ruleManager->getSourceById(sourceId);
            if (source) {
                result.setSourceName(source->name());
            }
        }

        results.append(result);

        debugLog(QString("Parse result %1: %2 - %3").arg(i + 1).arg(result.bookName()).arg(result.author()));
    }

    debugLog(QString("Search result parsing completed, %1 valid results").arg(results.size()));
    return results;
}

Book ContentParser::parseBookDetails(const QString &html, const BookSource &source, const QString &bookUrl)
{
    return parseBookDetails(html, *source.bookRule(), bookUrl);
}

Book ContentParser::parseBookDetails(const QString &html, const BookRule &rule, const QString &bookUrl)
{
    Book book;
    
    if (html.isEmpty()) {
        setError("HTML content is empty");
        return book;
    }
    
    debugLog("Start parsing book details");
    

    QString cleanHtml = preprocessHtml(html);

    book.setUrl(bookUrl);

    if (!rule.bookName().isEmpty()) {
        QString bookName = extractSingleContent(cleanHtml, rule.bookName(), detectContentType(rule.bookName()));
        book.setBookName(cleanText(bookName));
    }

    if (!rule.author().isEmpty()) {
        QString author = extractSingleContent(cleanHtml, rule.author(), detectContentType(rule.author()));
        book.setAuthor(cleanText(author));
    }

    if (!rule.intro().isEmpty()) {
        QString intro = extractSingleContent(cleanHtml, rule.intro(), detectContentType(rule.intro()));
        book.setIntro(cleanText(intro));
    }

    if (!rule.category().isEmpty()) {
        QString category = extractSingleContent(cleanHtml, rule.category(), detectContentType(rule.category()));
        book.setCategory(cleanText(category));
    }
    
    if (!rule.coverUrl().isEmpty()) {
        ContentType coverType = rule.coverUrl().startsWith("meta[") ? ATTR_CONTENT : ATTR_SRC;
        QString coverUrl = extractSingleContent(cleanHtml, rule.coverUrl(), coverType);
        book.setCoverUrl(coverUrl);
    }
    
    if (!rule.latestChapter().isEmpty()) {
        QString latestChapter = extractSingleContent(cleanHtml, rule.latestChapter(), detectContentType(rule.latestChapter()));
        book.setLatestChapter(cleanText(latestChapter));
    }

    if (!rule.lastUpdateTime().isEmpty()) {
        QString lastUpdateTime = extractSingleContent(cleanHtml, rule.lastUpdateTime(), detectContentType(rule.lastUpdateTime()));
        book.setLastUpdateTime(cleanText(lastUpdateTime));
    }

    if (!rule.status().isEmpty()) {
        QString status = extractSingleContent(cleanHtml, rule.status(), detectContentType(rule.status()));
        book.setStatus(cleanText(status));
    }

    if (!rule.wordCount().isEmpty()) {
        QString wordCount = extractSingleContent(cleanHtml, rule.wordCount(), detectContentType(rule.wordCount()));
        book.setWordCount(cleanText(wordCount));
    }
    
    debugLog(QString("Book details analysis completed: %1 - %2").arg(book.bookName()).arg(book.author()));
    return book;
}

QList<Chapter> ContentParser::parseChapterList(const QString &html, const BookSource &source, const QString &baseUrl)
{
    return parseChapterList(html, *source.tocRule(), baseUrl);
}

QList<Chapter> ContentParser::parseChapterList(const QString &html, const TocRule &rule, const QString &baseUrl)
{
    QList<Chapter> chapters;

    if (html.isEmpty() || rule.item().isEmpty()) {
        setError("HTML content or chapter selector is empty");
        return chapters;
    }

    debugLog(QString("Start parsing chapter list, selector: %1, pagination: %2").arg(rule.item()).arg(rule.pagination()));

    // Check if pagination is enabled
    if (rule.pagination() && !rule.nextPage().isEmpty()) {
        return parseChapterListWithPagination(html, rule, baseUrl);
    }

    // Single page parsing
    return parseChapterListSinglePage(html, rule, baseUrl);
}

QString ContentParser::parseChapterContent(const QString &html, const BookSource &source)
{
    return parseChapterContent(html, *source.chapterRule());
}

QString ContentParser::parseChapterContent(const QString &html, const ChapterRule &rule)
{
    if (html.isEmpty() || rule.content().isEmpty()) {
        setError("HTML content or chapter content selector is empty");
        return QString();
    }

    debugLog(QString("Start parsing chapter content, selector: %1, pagination: %2").arg(rule.content()).arg(rule.pagination()));

    // Check if pagination is enabled
    if (rule.pagination() && !rule.nextPage().isEmpty()) {
        return parseChapterContentWithPagination(html, rule, "");
    }

    // Single page parsing
    return parseChapterContentSinglePage(html, rule);
}

QStringList ContentParser::parseNextPageUrls(const QString &html, const QString &nextPageSelector, const QString &baseUrl)
{
    QStringList urls;

    if (html.isEmpty() || nextPageSelector.isEmpty()) {
        return urls;
    }


    QStringList nextPageItems = extractMultipleContent(html, nextPageSelector, ATTR_HREF);

    for (const QString &url : nextPageItems) {
        if (!url.isEmpty()) {
            urls.append(resolveUrl(url, baseUrl));
        }
    }

    return urls;
}

QString ContentParser::parseNextPageUrl(const QString &html, const QString &nextPageSelector, const QString &baseUrl)
{
    QStringList urls = parseNextPageUrls(html, nextPageSelector, baseUrl);
    return urls.isEmpty() ? QString() : urls.first();
}

ContentParser::ParseResult ContentParser::extractContent(const QString &html, const QString &selector, ContentType type)
{
    return parseWithSelector(html, selector, type);
}

QStringList ContentParser::extractMultipleContent(const QString &html, const QString &selector, ContentType type)
{
    if (html.isEmpty() || selector.isEmpty()) {
        debugLog("extractMultipleContent - Empty HTML or selector");
        return QStringList();
    }

    debugLog(QString("extractMultipleContent - Selector: '%1', Type: %2").arg(selector).arg(type));
    debugLog(QString("extractMultipleContent - HTML length: %1 chars").arg(html.length()));

    QRegularExpression regex;
    if (m_selectorCache.contains(selector)) {
        regex = m_selectorCache[selector];
        debugLog("extractMultipleContent - Using cached regex");
    } else {
        regex = selectorToRegex(selector);
        m_selectorCache[selector] = regex;
        debugLog(QString("extractMultipleContent - Generated regex pattern: '%1'").arg(regex.pattern()));
        debugLog(QString("extractMultipleContent - Regex is valid: %1").arg(regex.isValid() ? "true" : "false"));
    }

    QStringList results = extractMultipleByRegex(html, regex, type);
    debugLog(QString("extractMultipleContent - Found %1 matches").arg(results.size()));

    return results;
}

QList<SearchResult> ContentParser::parseSearchResultsWithLexbor(LexborHtmlParser& parser, const SearchRule& rule, int sourceId, const QString& baseUrl)
{
    QList<SearchResult> results;

    // Get all result elements using the improved method
    QList<LexborHtmlParser::ElementInfo> elements = parser.selectElementsWithInfo(rule.result());

    for (const auto& element : elements) {
        SearchResult result;
        result.setSourceId(sourceId);

        // Extract book name and URL
        QString bookName;
        QString bookUrl;

        if (rule.bookName().contains("@")) {
            QStringList parts = rule.bookName().split("@");
            if (parts.size() == 2) {
                bookName = parser.selectTextFromElement(element.node, parts[0]);
                bookUrl = parser.selectAttributeFromElement(element.node, parts[0], parts[1]);
            }
        } else {
            bookName = parser.selectTextFromElement(element.node, rule.bookName());
            bookUrl = parser.selectAttributeFromElement(element.node, rule.bookName(), "href");
        }

        if (bookName.isEmpty()) {
            continue;
        }

        result.setBookName(cleanText(bookName));
        result.setUrl(resolveUrl(bookUrl, baseUrl));

        // Extract author
        if (!rule.author().isEmpty()) {
            QString author = parser.selectTextFromElement(element.node, rule.author());
            result.setAuthor(cleanText(author));
        }

        // Extract category
        if (!rule.category().isEmpty()) {
            QString category = parser.selectTextFromElement(element.node, rule.category());
            result.setCategory(cleanText(category));
        }

        // Extract latest chapter
        if (!rule.latestChapter().isEmpty()) {
            QString latestChapter = parser.selectTextFromElement(element.node, rule.latestChapter());
            result.setLatestChapter(cleanText(latestChapter));
        }

        // Extract update time
        if (!rule.lastUpdateTime().isEmpty()) {
            QString updateTime = parser.selectTextFromElement(element.node, rule.lastUpdateTime());
            result.setLastUpdateTime(cleanText(updateTime));
        }

        // Set source name if RuleManager is available
        if (m_ruleManager) {
            const BookSource* source = m_ruleManager->getSourceById(sourceId);
            if (source) {
                result.setSourceName(source->name());
            }
        }

        results.append(result);
    }

    return results;
}

QString ContentParser::extractSingleContent(const QString &html, const QString &selector, ContentType type)
{
    QStringList results = extractMultipleContent(html, selector, type);
    return results.isEmpty() ? QString() : results.first();
}

QString ContentParser::cleanText(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }

    QString cleaned = text;


    cleaned = cleanInvisibleChars(cleaned);


    cleaned = unescapeHtml(cleaned);

    cleaned = normalizeWhitespace(cleaned);

    return cleaned.trimmed();
}

QString ContentParser::cleanHtml(const QString &html)
{
    if (html.isEmpty()) {
        return html;
    }

    QString cleaned = html;


    cleaned = cleanInvisibleChars(cleaned);

    cleaned.remove(QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption));

    cleaned.remove(QRegularExpression("<script[^>]*>.*?</script>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    cleaned.remove(QRegularExpression("<style[^>]*>.*?</style>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));

    return cleaned;
}

QString ContentParser::removeHtmlTags(const QString &html, const QStringList &tagsToRemove)
{
    QString result = html;

    if (tagsToRemove.isEmpty()) {

        result.remove(m_htmlTagRegex);
    } else {

        for (const QString &tag : tagsToRemove) {
            QString pattern = QString("<%1[^>]*>.*?</%1>").arg(tag);
            result.remove(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
        }
    }

    return result;
}

QString ContentParser::filterContent(const QString &content, const QString &filterText, const QString &filterTags)
{
    QString filtered = content;

    // Apply text filter using regex
    if (!filterText.isEmpty()) {
        try {
            QRegularExpression filterRegex(filterText);
            if (filterRegex.isValid()) {
                filtered.remove(filterRegex);
                debugLog(QString("Applied text filter, removed %1 matches").arg(filterText));
            } else {
                debugLog(QString("Invalid filter regex: %1 - %2").arg(filterText).arg(filterRegex.errorString()));
                setError(QString("Invalid filter regex: %1").arg(filterRegex.errorString()));
            }
        } catch (const std::exception& e) {
            debugLog(QString("Exception in text filter: %1").arg(e.what()));
            setError(QString("Exception in text filter: %1").arg(e.what()));
        }
    }

    if (!filterTags.isEmpty()) {
        QStringList tags = filterTags.split(' ', Qt::SkipEmptyParts);
        filtered = removeHtmlTags(filtered, tags);
    }

    return filtered;
}

QString ContentParser::formatChapterContent(const QString &content, const ChapterRule &rule)
{
    QString formatted = content;


    formatted = filterContent(formatted, rule.filterTxt(), rule.filterTag());

    if (!rule.paragraphTag().isEmpty()) {
        if (rule.paragraphTagClosed()) {

            formatted.replace(QRegularExpression(QString("<%1[^>]*>").arg(rule.paragraphTag())), "\n");
            formatted.replace(QRegularExpression(QString("</%1>").arg(rule.paragraphTag())), "");
        } else {

            formatted.replace(QRegularExpression(rule.paragraphTag()), "\n");
        }
    }


    formatted.remove(m_htmlTagRegex);

    formatted = unescapeHtml(formatted);

    formatted = normalizeWhitespace(formatted);

    formatted.remove(QRegularExpression("\\n\\s*\\n"));

    return formatted.trimmed();
}

QString ContentParser::resolveUrl(const QString &url, const QString &baseUrl)
{
    if (url.isEmpty()) {
        return url;
    }

    if (isAbsoluteUrl(url)) {
        return url;
    }

    if (baseUrl.isEmpty()) {
        return url;
    }

    QUrl base(baseUrl);
    QUrl resolved = base.resolved(QUrl(url));
    return resolved.toString();
}

QStringList ContentParser::resolveUrls(const QStringList &urls, const QString &baseUrl)
{
    QStringList resolved;
    for (const QString &url : urls) {
        resolved.append(resolveUrl(url, baseUrl));
    }
    return resolved;
}

bool ContentParser::validateSelector(const QString &selector)
{
    if (selector.isEmpty()) {
        return false;
    }

    try {
        QRegularExpression regex = selectorToRegex(selector);
        return regex.isValid();
    } catch (...) {
        return false;
    }
}

ContentParser::ParseResult ContentParser::parseWithSelector(const QString &html, const QString &selector, ContentType type)
{
    ParseResult result;

    if (html.isEmpty() || selector.isEmpty()) {
        result.error = "HTML content or selector is empty";
        return result;
    }

    try {
        QRegularExpression regex = selectorToRegex(selector);
        if (!regex.isValid()) {
            result.error = "Invalid selector";
            return result;
        }

        result.extractedTexts = extractMultipleByRegex(html, regex, type);
        result.success = true;

    } catch (const std::exception &e) {
        result.error = QString("Parse exception: %1").arg(e.what());
    } catch (...) {
        result.error = "Unknown parse exception";
    }

    return result;
}

QString ContentParser::extractByRegex(const QString &html, const QRegularExpression &regex, ContentType type)
{
    QStringList results = extractMultipleByRegex(html, regex, type);
    return results.isEmpty() ? QString() : results.first();
}

QStringList ContentParser::extractMultipleByRegex(const QString &html, const QRegularExpression &regex, ContentType type)
{
    QStringList results;

    debugLog(QString("extractMultipleByRegex - Starting regex matching with pattern: '%1'").arg(regex.pattern()));
    debugLog(QString("extractMultipleByRegex - Content type: %1").arg(type));
    debugLog(QString("extractMultipleByRegex - HTML sample (first 200 chars): %1").arg(html.left(200)));

    QRegularExpressionMatchIterator iterator = regex.globalMatch(html);
    int matchCount = 0;

    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        matchCount++;

        debugLog(QString("extractMultipleByRegex - Match %1 found at position %2-%3")
                .arg(matchCount).arg(match.capturedStart()).arg(match.capturedEnd()));
        debugLog(QString("extractMultipleByRegex - Full match: '%1'").arg(match.captured(0).left(100)));

        QString extracted;
        switch (type) {
        case TEXT:
            extracted = match.captured(1);
            debugLog(QString("extractMultipleByRegex - Extracted TEXT: '%1'").arg(extracted.left(50)));
            break;
        case HTML:
            extracted = match.captured(0);
            debugLog(QString("extractMultipleByRegex - Extracted HTML: '%1'").arg(extracted.left(50)));
            break;
        case ATTR_HREF:
            extracted = extractAttributeFromMatch(match, "href");
            debugLog(QString("extractMultipleByRegex - Extracted HREF: '%1'").arg(extracted));
            break;
        case ATTR_SRC:
            extracted = extractAttributeFromMatch(match, "src");
            debugLog(QString("extractMultipleByRegex - Extracted SRC: '%1'").arg(extracted));
            break;
        case ATTR_CONTENT:
            extracted = extractAttributeFromMatch(match, "content");
            debugLog(QString("extractMultipleByRegex - Extracted CONTENT: '%1'").arg(extracted));
            break;
        case ATTR_VALUE:
            extracted = extractAttributeFromMatch(match, "value");
            debugLog(QString("extractMultipleByRegex - Extracted VALUE: '%1'").arg(extracted));
            break;
        }

        if (!extracted.isEmpty()) {
            results.append(extracted);
            debugLog(QString("extractMultipleByRegex - Added result %1: '%2'").arg(results.size()).arg(extracted.left(30)));
        } else {
            debugLog("extractMultipleByRegex - Extracted content is empty, skipping");
        }
    }

    debugLog(QString("extractMultipleByRegex - Total matches found: %1, Valid results: %2").arg(matchCount).arg(results.size()));

    if (matchCount == 0) {
        debugLog("extractMultipleByRegex - No matches found! Checking HTML sample:");
        debugLog(QString("extractMultipleByRegex - HTML sample (chars 0-500): %1").arg(html.left(500)));
        debugLog(QString("extractMultipleByRegex - HTML sample (chars 500-1000): %1").arg(html.mid(500, 500)));
    }

    return results;
}

QString ContentParser::extractAttributeFromMatch(const QRegularExpressionMatch &match, const QString &attrName)
{
    QString fullMatch = match.captured(0);

    QString pattern = QString("%1\\s*=\\s*[\"']([^\"']*)[\"']").arg(attrName);
    QRegularExpression attrRegex(pattern, QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch attrMatch = attrRegex.match(fullMatch);
    return attrMatch.hasMatch() ? attrMatch.captured(1) : QString();
}

QRegularExpression ContentParser::selectorToRegex(const QString &selector)
{

    if (m_ruleManager) {
        return m_ruleManager->selectorToRegex(selector);
    }

    return SelectorConverter::toRegex(selector);
}

ContentParser::ContentType ContentParser::detectContentType(const QString &selector)
{
    if (selector.startsWith("meta[") && selector.contains("content")) {
        return ATTR_CONTENT;
    }
    if (selector.contains("href")) {
        return ATTR_HREF;
    }
    if (selector.contains("src")) {
        return ATTR_SRC;
    }
    return TEXT;
}

QString ContentParser::preprocessHtml(const QString &html)
{
    debugLog(QString("preprocessHtml - Original HTML length: %1 chars").arg(html.length()));
    debugLog(QString("preprocessHtml - Original HTML sample: %1").arg(html.left(200)));

    QString processed = html;

    // 清理不可见字符
    processed = cleanInvisibleChars(processed);
    debugLog(QString("preprocessHtml - After cleaning invisible chars: %1 chars").arg(processed.length()));

    // 移除HTML注释
    processed.remove(QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption));
    debugLog(QString("preprocessHtml - After removing comments: %1 chars").arg(processed.length()));

    // 标准化空白字符
    processed.replace(QRegularExpression("\\s+"), " ");
    debugLog(QString("preprocessHtml - After normalizing whitespace: %1 chars").arg(processed.length()));
    debugLog(QString("preprocessHtml - Processed HTML sample: %1").arg(processed.left(200)));

    return processed;
}

QString ContentParser::cleanInvisibleChars(const QString &text)
{
    QString cleaned = text;
    cleaned.remove(m_invisibleRegex);
    return cleaned;
}

QString ContentParser::unescapeHtml(const QString &html)
{

    QString result = html;
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    result.replace("&nbsp;", " ");
    return result;
}

QString ContentParser::normalizeWhitespace(const QString &text)
{
    QString normalized = text;
    normalized.replace(m_whitespaceRegex, " ");
    return normalized;
}

QString ContentParser::removeDuplicateTitle(const QString &content, const QString &title)
{
    if (title.isEmpty()) {
        return content;
    }

    QString result = content;

    if (result.startsWith(title)) {
        result = result.mid(title.length()).trimmed();
    }

    QString cleanTitle = cleanText(title);
    if (result.startsWith(cleanTitle)) {
        result = result.mid(cleanTitle.length()).trimmed();
    }

    return result;
}

QString ContentParser::normalizeUrl(const QString &url, const QString &baseUrl)
{
    return resolveUrl(url, baseUrl);
}

bool ContentParser::isAbsoluteUrl(const QString &url)
{
    return url.startsWith("http://") || url.startsWith("https://") || url.startsWith("//");
}

void ContentParser::setError(const QString &error)
{
    m_lastError = error;
    debugLog(QString("Error: %1").arg(error));
    emit parseError(error);
}

void ContentParser::debugLog(const QString &message)
{
    if (m_debugMode) {
        qDebug() << "ContentParser:" << message;
        emit debugMessage(message);
    }
}

QList<Chapter> ContentParser::parseChapterListWithRegex(const QString &html, const TocRule &rule, const QString &baseUrl)
{
    QList<Chapter> chapters;

    debugLog("Using fallback regex method for chapter list parsing");

    QString tocHtml = html;
    if (!rule.list().isEmpty()) {
        tocHtml = extractSingleContent(html, rule.list(), HTML);
        if (tocHtml.isEmpty()) {
            setError("Cannot find table of contents list area");
            return chapters;
        }
    }

    QStringList chapterItems = extractMultipleContent(tocHtml, rule.item(), HTML);

    debugLog(QString("Found %1 chapter items using regex").arg(chapterItems.size()));

    for (int i = 0; i < chapterItems.size(); ++i) {
        const QString &itemHtml = chapterItems[i];

        QString title = extractSingleContent(itemHtml, rule.item(), TEXT);
        QString url = extractSingleContent(itemHtml, rule.item(), ATTR_HREF);

        if (title.isEmpty() || url.isEmpty()) {
            debugLog(QString("Skip %1 section: title or link is empty").arg(i + 1));
            continue;
        }

        // Filter out non-chapter links
        if (isNonChapterLink(title, url)) {
            debugLog(QString("Skip %1 section: not a chapter link - %2").arg(i + 1).arg(title));
            continue;
        }

        Chapter chapter;
        chapter.setTitle(cleanText(title));
        chapter.setUrl(resolveUrl(url, baseUrl));
        chapter.setOrder(rule.isDesc() ? chapterItems.size() - i : i + 1);

        chapters.append(chapter);

        debugLog(QString("parsing chapter %1: %2").arg(chapter.order()).arg(chapter.title()));
    }

    if (rule.isDesc()) {
        std::reverse(chapters.begin(), chapters.end());

        for (int i = 0; i < chapters.size(); ++i) {
            chapters[i].setOrder(i + 1);
        }
    }

    return chapters;
}

bool ContentParser::isNonChapterLink(const QString &title, const QString &url)
{
    // Filter out common non-chapter links by title patterns
    QStringList nonChapterPatterns = {
        "home", "index", "bookmark", "collect", "vote", "recommend",
        "prev", "next", "return", "toc", "setting", "config",
        "login", "register", "search", "rank", "category", "complete"
    };

    for (const QString &pattern : nonChapterPatterns) {
        if (title.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Filter out Chinese common non-chapter titles using Unicode ranges
    if (title.contains(QRegularExpression("[\u9996\u9875]")) ||      // "首页"
        title.contains(QRegularExpression("[\u4e66\u67b6]")) ||      // "书架"
        title.contains(QRegularExpression("[\u52a0\u5165]")) ||      // "加入"
        title.contains(QRegularExpression("[\u6536\u85cf]")) ||      // "收藏"
        title.contains(QRegularExpression("[\u767b\u5f55]")) ||      // "登录"
        title.contains(QRegularExpression("[\u6ce8\u518c]")) ||      // "注册"
        title.contains(QRegularExpression("[\u641c\u7d22]")) ||      // "搜索"
        title.contains(QRegularExpression("[\u5c0f\u8bf4\u7f51]"))) { // "小说网"
        return true;
    }

    // Filter out common non-chapter links by URL patterns
    QStringList nonChapterUrlPatterns = {
        "javascript:", "mailto:", "#",
        "/index", "/search", "/rank", "/category",
        "/login", "/register", "/bookmark", "/vote"
    };

    for (const QString &pattern : nonChapterUrlPatterns) {
        if (url.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Filter out URLs that don't look like chapter URLs
    // Chapter URLs usually contain numbers or specific patterns
    if (!url.contains(QRegularExpression("\\d+")) && !url.contains("chapter", Qt::CaseInsensitive)) {
        // If URL doesn't contain numbers or "chapter", it's likely not a chapter
        // But allow if title looks like a chapter (contains Chinese chapter markers)
        if (!title.contains(QRegularExpression("[\u7b2c\u5377].*[\u7ae0\u8282\u56de]"))) { // "第卷" + "章节回"
            return true;
        }
    }

    return false;
}

QString ContentParser::applySpecialProcessing(const QString &content, const ChapterRule &rule)
{
    QString processed = content;

    // Check if content selector contains special JS processing
    if (rule.content().contains("@js:")) {
        processed = processJavaScriptRule(processed, rule.content());
    }

    return processed;
}

QString ContentParser::processJavaScriptRule(const QString &content, const QString &selector)
{
    QString processed = content;

    // Extract JS rule from selector
    if (!selector.contains("@js:")) {
        return processed;
    }

    QStringList parts = selector.split("@js:");
    if (parts.size() < 2) {
        return processed;
    }

    QString jsRule = parts[1];
    debugLog(QString("Processing JS rule: %1").arg(jsRule));

    // Handle Base64 decryption for specific novel sites
    if (jsRule.contains("qsbs.bb(")) {
        processed = processBase64Decryption(processed);
    }

    // Handle simple text replacements
    if (jsRule.contains("r.replace")) {
        // Remove common patterns
        processed = processed.replace(QRegularExpression("\\([0-9]+/[0-9]+\\)"), "");
        processed = processed.replace(QRegularExpression("\\([0-9]+ / [0-9]+\\)"), "");
    }

    return processed;
}

QString ContentParser::processBase64Decryption(const QString &content)
{
    QString decrypted = content;

    // Find Base64 encoded script tags
    QRegularExpression scriptRegex("<script>\\s*document\\.writeln\\(qsbs\\.bb\\('([^']+)'\\)\\);\\s*</script>");
    QRegularExpressionMatchIterator matches = scriptRegex.globalMatch(content);

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString encodedContent = match.captured(1);
        QString decodedContent = decodeBase64Content(encodedContent);

        // Replace the script tag with decoded content
        decrypted.replace(match.captured(0), decodedContent);

        debugLog(QString("Decoded Base64 content, length: %1 -> %2").arg(encodedContent.length()).arg(decodedContent.length()));
    }

    return decrypted;
}

QString ContentParser::decodeBase64Content(const QString &encodedContent)
{
    // Custom Base64 decoder for 顶点小说
    const QString keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    QString output;

    QString input = encodedContent;
    input.remove(QRegularExpression("[^A-Za-z0-9+/=]"));

    int i = 0;
    while (i < input.length()) {
        int enc1 = keyStr.indexOf(input.at(i++));
        int enc2 = keyStr.indexOf(input.at(i++));
        int enc3 = keyStr.indexOf(input.at(i++));
        int enc4 = keyStr.indexOf(input.at(i++));

        int chr1 = (enc1 << 2) | (enc2 >> 4);
        int chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
        int chr3 = ((enc3 & 3) << 6) | enc4;

        output += QChar(chr1);
        if (enc3 != 64) output += QChar(chr2);
        if (enc4 != 64) output += QChar(chr3);
    }

    // UTF-8 decode
    return QString::fromUtf8(output.toUtf8());
}

QList<Chapter> ContentParser::parseChapterListWithPagination(const QString &html, const TocRule &rule, const QString &baseUrl)
{
    QList<Chapter> allChapters;

    debugLog("Starting paginated chapter list parsing");

    QString currentHtml = html;
    QString currentBaseUrl = baseUrl;
    int pageCount = 1;
    int maxPages = 50; // Safety limit to prevent infinite loops

    while (!currentHtml.isEmpty() && pageCount <= maxPages) {
        debugLog(QString("Processing page %1 of chapter list").arg(pageCount));

        // Parse chapters from current page
        QList<Chapter> pageChapters = parseChapterListSinglePage(currentHtml, rule, currentBaseUrl);

        if (pageChapters.isEmpty()) {
            debugLog(QString("No chapters found on page %1, stopping pagination").arg(pageCount));
            break;
        }

        // Adjust chapter order for pagination
        for (Chapter &chapter : pageChapters) {
            chapter.setOrder(allChapters.size() + chapter.order());
        }

        allChapters.append(pageChapters);
        debugLog(QString("Added %1 chapters from page %2, total: %3").arg(pageChapters.size()).arg(pageCount).arg(allChapters.size()));

        // Find next page URL
        QString nextPageUrl = parseNextPageUrl(currentHtml, rule.nextPage(), currentBaseUrl);

        if (nextPageUrl.isEmpty()) {
            debugLog("No next page URL found, pagination complete");
            break;
        }

        if (nextPageUrl == currentBaseUrl) {
            debugLog("Next page URL is same as current, stopping to prevent infinite loop");
            break;
        }

        debugLog(QString("Found next page URL: %1").arg(nextPageUrl));

        // TODO: Here we would need to fetch the next page HTML
        // For now, we'll break since we don't have the HTTP client in ContentParser
        debugLog("Pagination requires HTTP client integration - stopping at first page for now");
        break;

        pageCount++;
    }

    if (pageCount > maxPages) {
        debugLog(QString("Reached maximum page limit (%1), stopping pagination").arg(maxPages));
    }

    debugLog(QString("Paginated chapter list parsing completed, total chapters: %1").arg(allChapters.size()));
    return allChapters;
}

QString ContentParser::parseChapterContentWithPagination(const QString &html, const ChapterRule &rule, const QString &baseUrl)
{
    QString allContent;

    if (!rule.pagination() || rule.nextPage().isEmpty()) {
        // No pagination, use single page parsing
        return parseChapterContent(html, rule);
    }

    debugLog("Starting paginated chapter content parsing");

    QString currentHtml = html;
    QString currentBaseUrl = baseUrl;
    int pageCount = 1;
    int maxPages = 20; // Safety limit for chapter content

    while (!currentHtml.isEmpty() && pageCount <= maxPages) {
        debugLog(QString("Processing page %1 of chapter content").arg(pageCount));

        // Parse content from current page
        QString pageContent = parseChapterContentSinglePage(currentHtml, rule);

        if (pageContent.isEmpty()) {
            debugLog(QString("No content found on page %1, stopping pagination").arg(pageCount));
            break;
        }

        // Append content with separator
        if (!allContent.isEmpty()) {
            allContent += "\n\n";
        }
        allContent += pageContent;

        debugLog(QString("Added content from page %1, total length: %2").arg(pageCount).arg(allContent.length()));

        // Find next page URL
        QString nextPageUrl = parseNextPageUrl(currentHtml, rule.nextPage(), currentBaseUrl);

        if (nextPageUrl.isEmpty()) {
            debugLog("No next page URL found, pagination complete");
            break;
        }

        if (nextPageUrl == currentBaseUrl) {
            debugLog("Next page URL is same as current, stopping to prevent infinite loop");
            break;
        }

        debugLog(QString("Found next page URL: %1").arg(nextPageUrl));

        // TODO: Here we would need to fetch the next page HTML
        // For now, we'll break since we don't have the HTTP client in ContentParser
        debugLog("Pagination requires HTTP client integration - stopping at first page for now");
        break;

        pageCount++;
    }

    if (pageCount > maxPages) {
        debugLog(QString("Reached maximum page limit (%1), stopping pagination").arg(maxPages));
    }

    debugLog(QString("Paginated chapter content parsing completed, total length: %1").arg(allContent.length()));
    return allContent;
}

QString ContentParser::parseChapterContentSinglePage(const QString &html, const ChapterRule &rule)
{
    if (html.isEmpty() || rule.content().isEmpty()) {
        setError("HTML content or chapter content selector is empty");
        return QString();
    }

    debugLog(QString("Start parsing single page chapter content, selector: %1").arg(rule.content()));

    QString cleanHtml = preprocessHtml(html);

    // === USE LEXBOR HTML PARSER FOR REAL CSS SELECTOR SUPPORT ===
    LexborHtmlParser lexborParser;
    QString content;

    if (lexborParser.parseHtml(cleanHtml)) {
        // Use real CSS selector to get chapter content
        QStringList contentElements = lexborParser.selectElements(rule.content());

        if (!contentElements.isEmpty()) {
            content = contentElements.first(); // Get the first matching element
            debugLog(QString("Lexbor extracted content length: %1").arg(content.length()));
        } else {
            debugLog(QString("Lexbor found no elements for selector: %1").arg(rule.content()));
        }
    }

    // Fallback to regex method if Lexbor fails
    if (content.isEmpty()) {
        debugLog("Falling back to regex method for chapter content");
        content = extractSingleContent(cleanHtml, rule.content(), HTML);
        debugLog(QString("Regex extracted content length: %1").arg(content.length()));
    }

    if (content.isEmpty()) {
        setError(QString("Cannot extract chapter content with selector: %1").arg(rule.content()));
        return QString();
    }

    // Apply special processing for specific book sources
    content = applySpecialProcessing(content, rule);

    content = formatChapterContent(content, rule);

    debugLog(QString("Single page chapter content parsing completed, final length: %1 characters").arg(content.length()));
    return content;
}

QList<Chapter> ContentParser::parseChapterListSinglePage(const QString &html, const TocRule &rule, const QString &baseUrl)
{
    QList<Chapter> chapters;

    QString cleanHtml = preprocessHtml(html);

    // === USE LEXBOR HTML PARSER FOR REAL CSS SELECTOR SUPPORT ===
    LexborHtmlParser lexborParser;
    if (!lexborParser.parseHtml(cleanHtml)) {
        debugLog("Failed to parse HTML with Lexbor, falling back to regex method");
        // Fallback to original regex method
        return parseChapterListWithRegex(cleanHtml, rule, baseUrl);
    }

    // Use real CSS selector to get chapter elements
    QList<LexborHtmlParser::ElementInfo> chapterElements = lexborParser.selectElementsWithInfo(rule.item());

    debugLog(QString("Found %1 chapter elements using Lexbor").arg(chapterElements.size()));

    for (int i = 0; i < chapterElements.size(); ++i) {
        const LexborHtmlParser::ElementInfo &element = chapterElements[i];

        QString title = element.textContent.trimmed();

        // Get href attribute from the element node
        QString url;
        if (element.node) {
            // Get href attribute directly from the node
            lxb_dom_element_t* elem = lxb_dom_interface_element(element.node);
            if (elem) {
                size_t attrLen = 0;
                const lxb_char_t* attrValue = lxb_dom_element_get_attribute(elem,
                    reinterpret_cast<const lxb_char_t*>("href"), 4, &attrLen);
                if (attrValue && attrLen > 0) {
                    url = QString::fromUtf8(reinterpret_cast<const char*>(attrValue), static_cast<int>(attrLen));
                }
            }
        }

        if (title.isEmpty() || url.isEmpty()) {
            debugLog(QString("Skip %1 section: title or link is empty - title:'%2' url:'%3'").arg(i + 1).arg(title).arg(url));
            continue;
        }

        // Filter out non-chapter links
        if (isNonChapterLink(title, url)) {
            debugLog(QString("Skip %1 section: not a chapter link - %2").arg(i + 1).arg(title));
            continue;
        }

        Chapter chapter;
        chapter.setTitle(cleanText(title));
        chapter.setUrl(resolveUrl(url, baseUrl));
        chapter.setOrder(rule.isDesc() ? chapterElements.size() - i : i + 1);

        chapters.append(chapter);

        debugLog(QString("parsing chapter %1: %2").arg(chapter.order()).arg(chapter.title()));
    }

    if (rule.isDesc()) {
        std::reverse(chapters.begin(), chapters.end());

        for (int i = 0; i < chapters.size(); ++i) {
            chapters[i].setOrder(i + 1);
        }
    }

    debugLog(QString("Single page chapter list parsing completed, %1 chapters").arg(chapters.size()));
    return chapters;
}
