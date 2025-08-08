#ifndef RULEMANAGER_H
#define RULEMANAGER_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QRegularExpression>
#include "../novel/NovelModels.h"

/**
 * @brief CSS Selector Converter
 * Converts CSS selectors to QRegularExpression or other usable parsing methods
 */
class SelectorConverter
{
public:
    /**
     * @brief Selector type enumeration
     */
    enum SelectorType {
        CSS_SELECTOR,       // Standard CSS selector
        ATTRIBUTE_SELECTOR, // Attribute selector [attr=value]
        TEXT_CONTENT,       // Text content
        REGEX_PATTERN       // Regular expression pattern
    };

    /**
     * @brief Parsed selector information
     */
    struct ParsedSelector {
        SelectorType type;
        QString selector;           // Original selector
        QString element;            // Element name
        QString attribute;          // Attribute name
        QString attributeValue;     // Attribute value
        QRegularExpression regex;   // Converted regular expression
        bool isValid;
        
        ParsedSelector() : type(CSS_SELECTOR), isValid(false) {}
    };

    static ParsedSelector parseSelector(const QString &cssSelector);
    static QRegularExpression toRegex(const QString &cssSelector);
    static QRegularExpression parseHierarchicalSelector(const QString &cssSelector);
    static bool isAttributeSelector(const QString &selector);
    static bool isTextSelector(const QString &selector);
};

/**
 * @brief Rule Manager Class
 * Responsible for loading, parsing, and managing book source rule files
 */
class RuleManager : public QObject
{
    Q_OBJECT

public:
    explicit RuleManager(QObject *parent = nullptr);
    ~RuleManager();

    // Rule file management
    bool loadRulesFromFile(const QString &filePath);
    bool loadRulesFromDirectory(const QString &dirPath);
    bool saveRulesToFile(const QString &filePath) const;
    bool reloadRules();
    void clearRules();

    // Book source management
    QList<BookSource> getAllSources() const;
    QList<BookSource> getEnabledSources() const;
    QList<BookSource> getSearchableSources() const;
    BookSource* getSourceById(int id);
    const BookSource* getSourceById(int id) const;
    BookSource* getSourceByName(const QString &name);
    const BookSource* getSourceByName(const QString &name) const;
    BookSource* matchSourceByUrl(const QString &url);
    const BookSource* matchSourceByUrl(const QString &url) const;

    // Rule operations
    bool addSource(const BookSource &source);
    bool updateSource(const BookSource &source);
    bool removeSource(int id);
    bool enableSource(int id, bool enabled = true);
    bool disableSource(int id) { return enableSource(id, false); }

    // Rule validation
    bool validateSource(const BookSource &source) const;
    QStringList validateAllSources() const;

    // Selector conversion
    SelectorConverter::ParsedSelector parseSelector(const QString &cssSelector) const;
    QRegularExpression selectorToRegex(const QString &cssSelector) const;

    // Status query
    int getSourceCount() const;
    int getEnabledSourceCount() const;
    bool hasSource(int id) const;
    bool isLoaded() const { return m_loaded; }
    QString getLastError() const { return m_lastError; }
    QStringList getLoadedFiles() const { return m_loadedFiles; }

    // File monitoring
    void enableFileWatching(bool enable = true);
    bool isFileWatchingEnabled() const { return m_fileWatchingEnabled; }

signals:
    void rulesLoaded(int count);
    void rulesReloaded();
    void sourceAdded(int id);
    void sourceUpdated(int id);
    void sourceRemoved(int id);
    void fileChanged(const QString &filePath);
    void errorOccurred(const QString &error);

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

private:
    // Internal methods
    bool loadRulesFromJson(const QJsonDocument &doc, const QString &filePath);
    bool parseSourceFromJson(const QJsonObject &json, BookSource &source);
    void setError(const QString &error);
    void updateSourceIndex();
    bool isValidRuleFile(const QString &filePath) const;

    // Member variables
    QList<BookSource> m_sources;                    // All book sources
    QHash<int, BookSource*> m_sourceById;           // ID index
    QHash<QString, BookSource*> m_sourceByName;     // Name index
    QList<BookSource*> m_sourcesByUrl;              // URL matching list (sorted by URL length)

    // Status management
    bool m_loaded;                                  // Whether loaded
    QString m_lastError;                            // Last error message
    QStringList m_loadedFiles;                      // List of loaded files

    // File monitoring
    QFileSystemWatcher *m_fileWatcher;              // File system watcher
    bool m_fileWatchingEnabled;                     // Whether file watching is enabled

    // Thread safety
    mutable QMutex m_mutex;                         // Mutex lock

    // Selector converter
    SelectorConverter m_selectorConverter;
};

#endif // RULEMANAGER_H
