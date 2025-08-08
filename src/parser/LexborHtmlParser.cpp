#include "LexborHtmlParser.h"
#include <QTextCodec>

// Simplified version: use basic HTML parsing without CSS selectors for now

LexborHtmlParser::LexborHtmlParser()
    : m_document(nullptr)
    , m_cssParser(nullptr)
    , m_selectors(nullptr)
{
    initializeLexbor();
}

LexborHtmlParser::~LexborHtmlParser()
{
    cleanupLexbor();
}

bool LexborHtmlParser::initializeLexbor()
{
    // Create HTML document
    m_document = lxb_html_document_create();
    if (!m_document) {
        m_lastError = "Failed to create HTML document";
        return false;
    }

    // Initialize CSS parser
    m_cssParser = lxb_css_parser_create();
    if (!m_cssParser) {
        m_lastError = "Failed to create CSS parser";
        return false;
    }

    lxb_status_t status = lxb_css_parser_init(m_cssParser, nullptr);
    if (status != LXB_STATUS_OK) {
        m_lastError = "Failed to initialize CSS parser";
        return false;
    }

    // Initialize selectors
    m_selectors = lxb_selectors_create();
    if (!m_selectors) {
        m_lastError = "Failed to create selectors";
        return false;
    }

    status = lxb_selectors_init(m_selectors);
    if (status != LXB_STATUS_OK) {
        m_lastError = "Failed to initialize selectors";
        return false;
    }

    return true;
}

void LexborHtmlParser::cleanupLexbor()
{
    if (m_selectors) {
        lxb_selectors_destroy(m_selectors, true);
        m_selectors = nullptr;
    }

    if (m_cssParser) {
        lxb_css_parser_destroy(m_cssParser, true);
        m_cssParser = nullptr;
    }

    if (m_document) {
        lxb_html_document_destroy(m_document);
        m_document = nullptr;
    }
}

bool LexborHtmlParser::parseHtml(const QString& html)
{
    if (!m_document) {
        m_lastError = "HTML document not initialized";
        return false;
    }

    // Clear previous content
    clear();

    // Recreate document
    m_document = lxb_html_document_create();
    if (!m_document) {
        m_lastError = "Failed to recreate HTML document";
        return false;
    }

    // Convert QString to UTF-8 byte array
    QByteArray htmlBytes = html.toUtf8();
    const lxb_char_t* htmlData = reinterpret_cast<const lxb_char_t*>(htmlBytes.constData());
    size_t htmlSize = htmlBytes.size();

    // Parse HTML
    lxb_status_t status = lxb_html_document_parse(m_document, htmlData, htmlSize);
    if (status != LXB_STATUS_OK) {
        m_lastError = QString("Failed to parse HTML, status: %1").arg(status);
        return false;
    }

    // Only output debug info for very small HTML fragments (likely problematic ones)
    if (htmlSize < 300) {
        qDebug() << "LexborHtmlParser: Parsed small HTML fragment, size:" << htmlSize;
    }

    return true;
}

// Static callback function for collecting results
static lxb_status_t selectElementsCallback(lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx)
{
    QList<QString>* results = static_cast<QList<QString>*>(ctx);

    if (node) {
        // Get text content
        size_t textLength = 0;
        lxb_char_t* textData = lxb_dom_node_text_content(node, &textLength);

        if (textData && textLength > 0) {
            QString text = QString::fromUtf8(reinterpret_cast<const char*>(textData), static_cast<int>(textLength));
            results->append(text.trimmed());
        }
    }

    return LXB_STATUS_OK;
}

QList<QString> LexborHtmlParser::selectElements(const QString& selector)
{
    QList<QString> results;

    if (!m_document || !m_cssParser || !m_selectors) {
        m_lastError = "Parser not properly initialized";
        return results;
    }

    // Parse CSS selector
    QByteArray selectorBytes = selector.toUtf8();
    lxb_css_selector_list_t* selectorList = lxb_css_selectors_parse(m_cssParser,
        reinterpret_cast<const lxb_char_t*>(selectorBytes.constData()),
        selectorBytes.length());

    if (!selectorList) {
        qDebug() << "LexborHtmlParser: Failed to parse CSS selector:" << selector;
        m_lastError = "Failed to parse CSS selector";
        return results;
    }

    // Get body element as root for search
    lxb_dom_node_t* body = lxb_dom_interface_node(lxb_html_document_body_element(m_document));
    if (!body) {
        // If no body, use document root
        body = &lxb_dom_interface_document(m_document)->node;
    }

    // Find matching elements using callback
    lxb_status_t status = lxb_selectors_find(m_selectors, body, selectorList, selectElementsCallback, &results);

    if (status != LXB_STATUS_OK) {
        qDebug() << "LexborHtmlParser: Selector search failed with status:" << status;
        m_lastError = "Selector search failed";
    }

    // Cleanup
    lxb_css_selector_list_destroy_memory(selectorList);

    // Only output debug info if no results found and HTML is small (likely problematic)
    if (results.isEmpty()) {
        // Get the HTML content for debugging only if it's small
        lexbor_str_t str = {0};
        lxb_status_t serialize_status = lxb_html_serialize_str(body, &str);

        if (serialize_status == LXB_STATUS_OK && str.data && str.length < 500) {
            QString htmlContent = QString::fromUtf8(reinterpret_cast<const char*>(str.data), str.length);
            qDebug() << "LexborHtmlParser: Found 0 elements for selector:" << selector;
            qDebug() << "LexborHtmlParser: HTML content (first 500 chars):" << htmlContent.left(500);
        }

        if (str.data) {
            lexbor_str_destroy(&str, body->owner_document->text, false);
        }
    }

    return results;
}

QString LexborHtmlParser::selectText(const QString& selector)
{
    QList<QString> results = selectElements(selector);
    return results.isEmpty() ? QString() : results.first();
}

QList<LexborHtmlParser::ElementInfo> LexborHtmlParser::selectElementsWithInfo(const QString& selector)
{
    QList<ElementInfo> results;

    if (!m_document || !m_cssParser || !m_selectors) {
        m_lastError = "Parser not properly initialized";
        return results;
    }

    // Parse CSS selector
    QByteArray selectorBytes = selector.toUtf8();
    lxb_css_selector_list_t* selectorList = lxb_css_selectors_parse(m_cssParser,
        reinterpret_cast<const lxb_char_t*>(selectorBytes.constData()),
        selectorBytes.length());

    if (!selectorList) {
        m_lastError = "Failed to parse CSS selector";
        return results;
    }

    // Get body element as root for search
    lxb_dom_node_t* body = lxb_dom_interface_node(lxb_html_document_body_element(m_document));
    if (!body) {
        body = &lxb_dom_interface_document(m_document)->node;
    }

    // Find matching elements using callback
    lxb_status_t status = lxb_selectors_find(m_selectors, body, selectorList,
        [](lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx) -> lxb_status_t {
            QList<ElementInfo>* results = static_cast<QList<ElementInfo>*>(ctx);

            if (node) {
                // Get text content
                size_t textLength = 0;
                lxb_char_t* textData = lxb_dom_node_text_content(node, &textLength);

                QString text;
                if (textData && textLength > 0) {
                    text = QString::fromUtf8(reinterpret_cast<const char*>(textData), static_cast<int>(textLength));
                }

                results->append(ElementInfo(node, text.trimmed()));
            }

            return LXB_STATUS_OK;
        }, &results);

    // Cleanup
    lxb_css_selector_list_destroy_memory(selectorList);

    return results;
}

QString LexborHtmlParser::selectTextFromElement(lxb_dom_node_t* element, const QString& selector)
{
    if (!element || !m_cssParser || !m_selectors) {
        return QString();
    }

    // Check if selector contains JavaScript rule (@js:)
    QString cssSelector = selector;
    bool hasJsRule = false;

    if (selector.contains("@js:")) {
        QStringList parts = selector.split("@js:");
        if (parts.size() == 2) {
            cssSelector = parts[0].trimmed();
            hasJsRule = true;
        }
    }

    // Parse CSS selector
    QByteArray selectorBytes = cssSelector.toUtf8();
    lxb_css_selector_list_t* selectorList = lxb_css_selectors_parse(m_cssParser,
        reinterpret_cast<const lxb_char_t*>(selectorBytes.constData()),
        selectorBytes.length());

    if (!selectorList) {
        return QString();
    }

    QString result;

    // Find matching elements using callback
    lxb_selectors_find(m_selectors, element, selectorList,
        [](lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx) -> lxb_status_t {
            QString* result = static_cast<QString*>(ctx);

            if (node && result->isEmpty()) {
                // Get text content
                size_t textLength = 0;
                lxb_char_t* textData = lxb_dom_node_text_content(node, &textLength);

                if (textData && textLength > 0) {
                    *result = QString::fromUtf8(reinterpret_cast<const char*>(textData), static_cast<int>(textLength)).trimmed();
                }
            }

            return LXB_STATUS_OK;
        }, &result);

    // Cleanup
    lxb_css_selector_list_destroy_memory(selectorList);

    // Apply simple JavaScript rule processing for common patterns
    if (hasJsRule && !result.isEmpty()) {
        // Handle common pattern: r=r.replace('作者：', '');
        if (selector.contains("r.replace('作者：', '')")) {
            result = result.replace("作者：", "");
        }
        // Add more patterns as needed
    }

    return result;
}

QString LexborHtmlParser::selectAttributeFromElement(lxb_dom_node_t* element, const QString& selector, const QString& attribute)
{
    if (!element || !m_cssParser || !m_selectors) {
        return QString();
    }

    // Parse CSS selector
    QByteArray selectorBytes = selector.toUtf8();
    lxb_css_selector_list_t* selectorList = lxb_css_selectors_parse(m_cssParser,
        reinterpret_cast<const lxb_char_t*>(selectorBytes.constData()),
        selectorBytes.length());

    if (!selectorList) {
        return QString();
    }

    QString result;
    QByteArray attrBytes = attribute.toUtf8();

    // Find matching elements using callback
    lxb_selectors_find(m_selectors, element, selectorList,
        [](lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx) -> lxb_status_t {
            auto* params = static_cast<QPair<QString*, QByteArray*>*>(ctx);
            QString* result = params->first;
            QByteArray* attrBytes = params->second;

            if (node && result->isEmpty() && node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                lxb_dom_element_t* elem = lxb_dom_interface_element(node);

                // Get attribute value
                size_t attrLength = 0;
                const lxb_char_t* attrValue = lxb_dom_element_get_attribute(elem,
                    reinterpret_cast<const lxb_char_t*>(attrBytes->constData()),
                    attrBytes->length(), &attrLength);

                if (attrValue && attrLength > 0) {
                    *result = QString::fromUtf8(reinterpret_cast<const char*>(attrValue), static_cast<int>(attrLength));
                }
            }

            return LXB_STATUS_OK;
        }, &QPair<QString*, QByteArray*>(&result, &attrBytes));

    // Cleanup
    lxb_css_selector_list_destroy_memory(selectorList);

    return result;
}

// Static callback function for collecting attribute values
static lxb_status_t selectAttributeCallback(lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx)
{
    struct AttributeContext {
        QString* result;
        QString attribute;
    };

    AttributeContext* attrCtx = static_cast<AttributeContext*>(ctx);

    if (node && node->type == LXB_DOM_NODE_TYPE_ELEMENT && attrCtx->result->isEmpty()) {
        lxb_dom_element_t* element = lxb_dom_interface_element(node);

        // Get attribute value
        QByteArray attrBytes = attrCtx->attribute.toUtf8();
        size_t attrLength = 0;
        const lxb_char_t* attrValue = lxb_dom_element_get_attribute(element,
            reinterpret_cast<const lxb_char_t*>(attrBytes.constData()),
            attrBytes.length(), &attrLength);

        if (attrValue && attrLength > 0) {
            *attrCtx->result = QString::fromUtf8(reinterpret_cast<const char*>(attrValue), static_cast<int>(attrLength));
        }
    }

    return LXB_STATUS_OK;
}

QString LexborHtmlParser::selectAttribute(const QString& selector, const QString& attribute)
{
    if (!m_document || !m_cssParser || !m_selectors) {
        m_lastError = "Parser not properly initialized";
        return QString();
    }

    // Parse CSS selector
    QByteArray selectorBytes = selector.toUtf8();
    lxb_css_selector_list_t* selectorList = lxb_css_selectors_parse(m_cssParser,
        reinterpret_cast<const lxb_char_t*>(selectorBytes.constData()),
        selectorBytes.length());

    if (!selectorList) {
        qDebug() << "LexborHtmlParser: Failed to parse CSS selector:" << selector;
        m_lastError = "Failed to parse CSS selector";
        return QString();
    }

    // Get body element as root for search
    lxb_dom_node_t* body = lxb_dom_interface_node(lxb_html_document_body_element(m_document));
    if (!body) {
        // If no body, use document root
        body = &lxb_dom_interface_document(m_document)->node;
    }

    QString result;
    struct {
        QString* result;
        QString attribute;
    } attrCtx = { &result, attribute };

    // Find matching elements using callback
    lxb_status_t status = lxb_selectors_find(m_selectors, body, selectorList, selectAttributeCallback, &attrCtx);

    if (status != LXB_STATUS_OK) {
        qDebug() << "LexborHtmlParser: Selector search failed with status:" << status;
        m_lastError = "Selector search failed";
    }

    // Cleanup
    lxb_css_selector_list_destroy_memory(selectorList);

    // Only output debug info if result is empty (indicating a problem)
    if (result.isEmpty()) {
        qDebug() << "LexborHtmlParser: Found empty attribute value for selector:" << selector << "attribute:" << attribute;
    }

    return result;
}

void LexborHtmlParser::clear()
{
    if (m_document) {
        lxb_html_document_destroy(m_document);
        m_document = nullptr;
    }
}

QString LexborHtmlParser::extractElementContent(lxb_dom_node_t* node)
{
    if (!node) return QString();

    // Get text content
    size_t textLength = 0;
    lxb_char_t* textData = lxb_dom_node_text_content(node, &textLength);

    if (!textData || textLength == 0) {
        return QString();
    }

    QString text = QString::fromUtf8(reinterpret_cast<const char*>(textData), static_cast<int>(textLength));

    // Clean up memory - Lexbor manages memory internally, so we don't need to free textData
    return text.trimmed();
}

QString LexborHtmlParser::getElementText(lxb_dom_element_t* element)
{
    if (!element) return QString();

    // Get element text content
    size_t textLength = 0;
    lxb_char_t* textData = lxb_dom_node_text_content(lxb_dom_interface_node(element), &textLength);

    if (!textData || textLength == 0) {
        return QString();
    }

    QString text = QString::fromUtf8(reinterpret_cast<const char*>(textData), static_cast<int>(textLength));

    // Clean up memory
    if (textData) {
        // Note: lxb_dom_document_destroy_text might not be the correct cleanup function
        // For now, skip manual cleanup as Lexbor manages memory internally
        // lxb_dom_document_destroy_text(lxb_dom_interface_document(element), textData);
    }

    return text.trimmed();
}

QString LexborHtmlParser::getElementAttribute(lxb_dom_element_t* element, const QString& attribute)
{
    if (!element) return QString();

    QByteArray attrBytes = attribute.toUtf8();
    const lxb_char_t* attrName = reinterpret_cast<const lxb_char_t*>(attrBytes.constData());
    size_t attrNameLength = attrBytes.size();

    // Get attribute value
    size_t attrValueLength = 0;
    const lxb_char_t* attrValue = lxb_dom_element_get_attribute(element, attrName, attrNameLength, &attrValueLength);

    if (!attrValue || attrValueLength == 0) {
        return QString();
    }

    return QString::fromUtf8(reinterpret_cast<const char*>(attrValue), static_cast<int>(attrValueLength));
}

void LexborHtmlParser::findElementsByTagName(lxb_dom_node_t* node, const QString& tagName, QList<QString>& results)
{
    if (!node) return;

    // Limit recursion depth to prevent stack overflow
    static int recursionDepth = 0;
    if (recursionDepth > 100) {
        return;
    }
    recursionDepth++;

    // If it's an element node, check tag name
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        lxb_dom_element_t* element = lxb_dom_interface_element(node);
        if (!element) {
            recursionDepth--;
            return;
        }

        // Get element tag name
        size_t tagNameLength = 0;
        const lxb_char_t* elementTagName = lxb_dom_element_qualified_name(element, &tagNameLength);

        if (elementTagName && tagNameLength > 0) {
            QString currentTagName = QString::fromUtf8(reinterpret_cast<const char*>(elementTagName), static_cast<int>(tagNameLength));

            // Check if it matches (* matches all elements)
            if (tagName == "*" || currentTagName.compare(tagName, Qt::CaseInsensitive) == 0) {
                QString text = getElementText(element);
                if (!text.isEmpty() && text.length() < 1000) { // Limit text length
                    results.append(text);
                    // Limit number of results to prevent memory issues
                    if (results.size() >= 100) {
                        recursionDepth--;
                        return;
                    }
                }
            }
        }
    }

    // Recursively traverse child nodes
    lxb_dom_node_t* child = lxb_dom_node_first_child(node);
    while (child) {
        findElementsByTagName(child, tagName, results);
        child = lxb_dom_node_next(child);
    }

    recursionDepth--;
}
