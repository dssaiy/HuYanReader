#pragma once

#include <QString>
#include <QList>
#include <QDebug>
#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

/**
 * @brief Professional HTML parser based on Lexbor
 *
 * Uses Google Lexbor library for HTML parsing and CSS selector queries
 * Replaces the original regex approach with more accurate HTML parsing
 */
class LexborHtmlParser
{
public:
    /**
     * @brief Element information structure
     */
    struct ElementInfo {
        lxb_dom_node_t* node;
        QString textContent;

        ElementInfo() : node(nullptr) {}
        ElementInfo(lxb_dom_node_t* n, const QString& text) : node(n), textContent(text) {}
    };

    LexborHtmlParser();
    ~LexborHtmlParser();

    /**
     * @brief Parse HTML content
     * @param html HTML string
     * @return Whether parsing was successful
     */
    bool parseHtml(const QString& html);

    /**
     * @brief Query elements using CSS selector
     * @param selector CSS selector string
     * @return List of matching elements
     */
    QList<QString> selectElements(const QString& selector);

    /**
     * @brief Query single element text content using CSS selector
     * @param selector CSS selector string
     * @return Text content of first matching element
     */
    QString selectText(const QString& selector);

    /**
     * @brief Query single element attribute value using CSS selector
     * @param selector CSS selector string
     * @param attribute Attribute name
     * @return Attribute value of first matching element
     */
    QString selectAttribute(const QString& selector, const QString& attribute);

    /**
     * @brief Query elements with detailed information using CSS selector
     * @param selector CSS selector string
     * @return List of matching elements with node information
     */
    QList<ElementInfo> selectElementsWithInfo(const QString& selector);

    /**
     * @brief Query text content from a specific element using relative selector
     * @param element Parent element node
     * @param selector CSS selector relative to the element
     * @return Text content of first matching child element
     */
    QString selectTextFromElement(lxb_dom_node_t* element, const QString& selector);

    /**
     * @brief Query attribute value from a specific element using relative selector
     * @param element Parent element node
     * @param selector CSS selector relative to the element
     * @param attribute Attribute name
     * @return Attribute value of first matching child element
     */
    QString selectAttributeFromElement(lxb_dom_node_t* element, const QString& selector, const QString& attribute);

    /**
     * @brief Clear resources
     */
    void clear();

    /**
     * @brief Get last parsing error message
     */
    QString getLastError() const { return m_lastError; }

private:
    lxb_html_document_t* m_document;
    lxb_css_parser_t* m_cssParser;
    lxb_selectors_t* m_selectors;
    QString m_lastError;

    /**
     * @brief Initialize Lexbor components
     */
    bool initializeLexbor();

    /**
     * @brief Cleanup Lexbor resources
     */
    void cleanupLexbor();

    /**
     * @brief Extract content from DOM node
     */
    QString extractElementContent(lxb_dom_node_t* node);

    /**
     * @brief Get element text content
     */
    QString getElementText(lxb_dom_element_t* element);

    /**
     * @brief Get element attribute value
     */
    QString getElementAttribute(lxb_dom_element_t* element, const QString& attribute);

    /**
     * @brief Recursively find elements by tag name
     */
    void findElementsByTagName(lxb_dom_node_t* node, const QString& tagName, QList<QString>& results);
};
