#include "RuleManager.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonParseError>
#include <QDebug>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QTimer>
#include <algorithm>

// SelectorConverter implementation

SelectorConverter::ParsedSelector SelectorConverter::parseSelector(const QString &cssSelector)
{
    ParsedSelector result;
    result.selector = cssSelector.trimmed();
    
    if (result.selector.isEmpty()) {
        return result;
    }
    

    if (isAttributeSelector(result.selector)) {
        result.type = ATTRIBUTE_SELECTOR;
        

        QRegularExpression attrRegex("^([^\\[]+)\\[([^=]+)=?\"?([^\"]*)\"?\\]$");
        QRegularExpressionMatch match = attrRegex.match(result.selector);
        
        if (match.hasMatch()) {
            result.element = match.captured(1).trimmed();
            result.attribute = match.captured(2).trimmed();
            result.attributeValue = match.captured(3).trimmed();
            result.isValid = true;
            

            QString pattern = QString(R"(<%1[^>]*%2\s*=\s*['"]*%3['"]*[^>]*>(.*?)</%1>)")
                             .arg(result.element, result.attribute, QRegularExpression::escape(result.attributeValue));
            result.regex = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        }
    }

    else if (isTextSelector(result.selector)) {
        result.type = TEXT_CONTENT;
        result.isValid = true;
        result.regex = toRegex(result.selector);
    }

    else {
        result.type = CSS_SELECTOR;
        result.isValid = true;
        result.regex = toRegex(result.selector);
    }
    
    return result;
}

QRegularExpression SelectorConverter::toRegex(const QString &cssSelector)
{
    QString selector = cssSelector.trimmed();

    // Handle hierarchical selectors (containing >)
    if (selector.contains('>')) {
        return parseHierarchicalSelector(selector);
    }

    // Handle simple ID selector
    if (selector.startsWith('#')) {
        QString id = selector.mid(1);
        QString pattern = QString(R"(<[^>]*id\s*=\s*['"]*%1['"]*[^>]*>(.*?)</[^>]+>)")
                         .arg(QRegularExpression::escape(id));
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    }

    // Handle simple class selector
    if (selector.startsWith('.')) {
        QString className = selector.mid(1);
        QString pattern = QString(R"(<[^>]*class\s*=\s*['"]*[^'"]*%1[^'"]*['"]*[^>]*>(.*?)</[^>]+>)")
                         .arg(QRegularExpression::escape(className));
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    }

    // Handle simple tag selector
    if (selector.contains(QRegularExpression("^[a-zA-Z][a-zA-Z0-9]*$"))) {
        QString pattern = QString(R"(<%1[^>]*>(.*?)</%1>)")
                         .arg(QRegularExpression::escape(selector));
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    }

    // Default fallback
    QString pattern = QString(R"(<[^>]*>(.*?)</[^>]+>)");
    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
}

QRegularExpression SelectorConverter::parseHierarchicalSelector(const QString &cssSelector)
{
    // For hierarchical selectors like "#checkform > table > tbody > tr"
    // We need to create a regex that respects the full hierarchy

    QStringList parts = cssSelector.split('>', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QRegularExpression();
    }

    // For complex selectors like ".novelslist2 > ul > li", we need a simpler approach
    // Just match the final element and capture its content
    QString finalElement = parts.last().trimmed();
    QString pattern;

    if (finalElement.startsWith('#')) {
        QString id = finalElement.mid(1);
        pattern = QString(R"(<[^>]*id\s*=\s*['"]*%1['"]*[^>]*>(.*?)</[^>]+>)")
                 .arg(QRegularExpression::escape(id));
    } else if (finalElement.startsWith('.')) {
        QString className = finalElement.mid(1);
        pattern = QString(R"(<[^>]*class\s*=\s*['"]*[^'"]*%1[^'"]*['"]*[^>]*>(.*?)</[^>]+>)")
                 .arg(QRegularExpression::escape(className));
    } else {
        // Tag name - match all instances of this tag
        pattern = QString(R"(<%1[^>]*>(.*?)</%1>)")
                 .arg(QRegularExpression::escape(finalElement));
    }

    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
}

bool SelectorConverter::isAttributeSelector(const QString &selector)
{
    return selector.contains('[') && selector.contains(']') && selector.contains('=');
}

bool SelectorConverter::isTextSelector(const QString &selector)
{

    return !selector.contains('<') && !selector.contains('[') && !selector.startsWith('#') && !selector.startsWith('.');
}

// ============================================================================
// RuleManager 
// ============================================================================

RuleManager::RuleManager(QObject *parent)
    : QObject(parent)
    , m_loaded(false)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_fileWatchingEnabled(false)
{

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &RuleManager::onFileChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &RuleManager::onDirectoryChanged);
}

RuleManager::~RuleManager()
{
    clearRules();
}

bool RuleManager::loadRulesFromFile(const QString &filePath)
{
    qDebug() << "RuleManager::loadRulesFromFile - START:" << filePath;
    QMutexLocker locker(&m_mutex);
    qDebug() << "RuleManager::loadRulesFromFile - Mutex locked";

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        setError(QString("Rule file does not exist or is not readable: %1").arg(filePath));
        return false;
    }
    qDebug() << "RuleManager::loadRulesFromFile - File exists and readable";

    if (!isValidRuleFile(filePath)) {
        setError(QString("Invalid rule file format: %1").arg(filePath));
        return false;
    }
    qDebug() << "RuleManager::loadRulesFromFile - File format valid";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QString("Cannot open rule file: %1").arg(file.errorString()));
        return false;
    }
    qDebug() << "RuleManager::loadRulesFromFile - File opened successfully";

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    qDebug() << "RuleManager::loadRulesFromFile - JSON parsing completed";

    if (parseError.error != QJsonParseError::NoError) {
        setError(QString("JSON parse error: %1 (position: %2)")
                .arg(parseError.errorString())
                .arg(parseError.offset));
        return false;
    }
    qDebug() << "RuleManager::loadRulesFromFile - JSON parse successful, calling loadRulesFromJson";

    bool success = loadRulesFromJson(doc, filePath);
    qDebug() << "RuleManager::loadRulesFromFile - loadRulesFromJson returned:" << success;
    if (success) {
        qDebug() << "RuleManager::loadRulesFromFile - Processing success case";
        if (!m_loadedFiles.contains(filePath)) {
            m_loadedFiles.append(filePath);
        }
        qDebug() << "RuleManager::loadRulesFromFile - Added to loaded files";

        if (m_fileWatchingEnabled && !m_fileWatcher->files().contains(filePath)) {
            m_fileWatcher->addPath(filePath);
        }
        qDebug() << "RuleManager::loadRulesFromFile - File watching setup";

        m_loaded = true;
        qDebug() << "RuleManager::loadRulesFromFile - About to emit rulesLoaded signal";
        emit rulesLoaded(m_sources.size());
        qDebug() << "RuleManager::loadRulesFromFile - rulesLoaded signal emitted";
        qDebug() << "RuleManager: Successfully loaded rule file" << filePath << ", total" << m_sources.size() << "sources";
    }

    qDebug() << "RuleManager::loadRulesFromFile - FINISHED, returning:" << success;
    return success;
}

bool RuleManager::loadRulesFromDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        setError(QString("Rule directory does not exist: %1").arg(dirPath));
        return false;
    }
    
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    if (files.isEmpty()) {
        setError(QString("No JSON files found in rule directory: %1").arg(dirPath));
        return false;
    }
    
    bool hasSuccess = false;
    for (const QFileInfo &fileInfo : files) {
        if (loadRulesFromFile(fileInfo.absoluteFilePath())) {
            hasSuccess = true;
        }
    }
    
    if (hasSuccess) {

        if (m_fileWatchingEnabled && !m_fileWatcher->directories().contains(dirPath)) {
            m_fileWatcher->addPath(dirPath);
        }
    }
    
    return hasSuccess;
}

bool RuleManager::loadRulesFromJson(const QJsonDocument &doc, const QString &filePath)
{
    qDebug() << "RuleManager::loadRulesFromJson - START";
    if (!doc.isArray()) {
        setError("JSON document must be array format");
        return false;
    }
    qDebug() << "RuleManager::loadRulesFromJson - Document is array format";

    QJsonArray array = doc.array();
    int successCount = 0;
    qDebug() << "RuleManager::loadRulesFromJson - Array size:" << array.size();

    for (int i = 0; i < array.size(); ++i) {
        qDebug() << "RuleManager::loadRulesFromJson - Processing item" << i;
        const QJsonValue &value = array[i];
        if (!value.isObject()) {
            qWarning() << "RuleManager: Skip invalid source item (not object)";
            continue;
        }

        qDebug() << "RuleManager::loadRulesFromJson - Item" << i << "is object, parsing...";
        BookSource source;
        if (parseSourceFromJson(value.toObject(), source)) {
            qDebug() << "RuleManager::loadRulesFromJson - Successfully parsed source:" << source.name();

            // Check for duplicate ID directly (avoid mutex deadlock)
            bool isDuplicate = false;
            for (const BookSource &existingSource : m_sources) {
                if (existingSource.id() == source.id()) {
                    isDuplicate = true;
                    break;
                }
            }

            if (isDuplicate) {
                qWarning() << "RuleManager: Duplicate source ID, skip:" << source.id() << source.name();
                continue;
            }

            m_sources.append(source);
            successCount++;
            qDebug() << "RuleManager::loadRulesFromJson - Added source, total count:" << successCount;
        } else {
            qDebug() << "RuleManager::loadRulesFromJson - Failed to parse source item" << i;
        }
    }
    
    if (successCount > 0) {
        qDebug() << "RuleManager::loadRulesFromJson - Calling updateSourceIndex()";
        updateSourceIndex();
        qDebug() << "RuleManager::loadRulesFromJson - updateSourceIndex() completed";
        qDebug() << "RuleManager: Successfully parsed" << successCount << "sources from" << filePath;
        qDebug() << "RuleManager::loadRulesFromJson - FINISHED successfully";
        return true;
    }

    qDebug() << "RuleManager::loadRulesFromJson - No sources parsed successfully";
    setError("No sources parsed successfully");
    return false;
}

bool RuleManager::parseSourceFromJson(const QJsonObject &json, BookSource &source)
{
    qDebug() << "RuleManager::parseSourceFromJson - START";
    try {
        qDebug() << "RuleManager::parseSourceFromJson - Calling source.fromJson()";
        source.fromJson(json);
        qDebug() << "RuleManager::parseSourceFromJson - source.fromJson() completed";

        if (source.id() < 0 || source.url().isEmpty() || source.name().isEmpty()) {
            qWarning() << "RuleManager: Source missing required fields:" << json;
            return false;
        }
        qDebug() << "RuleManager::parseSourceFromJson - Source validation passed";

        return true;
    } catch (const std::exception &e) {
        qWarning() << "RuleManager: Exception occurred while parsing source:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "RuleManager: Unknown exception occurred while parsing source";
        return false;
    }
}

bool RuleManager::saveRulesToFile(const QString &filePath) const
{
    QMutexLocker locker(&m_mutex);

    QJsonArray array;
    for (const BookSource &source : m_sources) {
        array.append(source.toJson());
    }

    QJsonDocument doc(array);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "RuleManager: Cannot write file:" << filePath << file.errorString();
        return false;
    }

    qint64 bytesWritten = file.write(doc.toJson());
    if (bytesWritten == -1) {
        qWarning() << "RuleManager: Write file failed:" << file.errorString();
        return false;
    }

    qDebug() << "RuleManager: Successfully saved" << m_sources.size() << "sources to" << filePath;
    return true;
}

bool RuleManager::reloadRules()
{
    QMutexLocker locker(&m_mutex);

    QStringList filesToReload = m_loadedFiles;
    clearRules();

    bool hasSuccess = false;
    for (const QString &filePath : filesToReload) {
        if (QFileInfo::exists(filePath)) {
            if (loadRulesFromFile(filePath)) {
                hasSuccess = true;
            }
        } else {
            qWarning() << "RuleManager: The file does not exist when reloading.:" << filePath;
        }
    }

    if (hasSuccess) {
        emit rulesReloaded();
    }

    return hasSuccess;
}

void RuleManager::clearRules()
{
    QMutexLocker locker(&m_mutex);

    m_sources.clear();
    m_sourceById.clear();
    m_sourceByName.clear();
    m_sourcesByUrl.clear();
    m_loadedFiles.clear();
    m_loaded = false;
    m_lastError.clear();


    if (!m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }
    if (!m_fileWatcher->directories().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->directories());
    }
}

QList<BookSource> RuleManager::getAllSources() const
{
    QMutexLocker locker(&m_mutex);
    return m_sources;
}

QList<BookSource> RuleManager::getEnabledSources() const
{
    QMutexLocker locker(&m_mutex);

    QList<BookSource> enabled;
    for (const BookSource &source : m_sources) {
        if (!source.disabled()) {
            enabled.append(source);
        }
    }
    return enabled;
}

QList<BookSource> RuleManager::getSearchableSources() const
{
    QMutexLocker locker(&m_mutex);

    QList<BookSource> searchable;
    for (const BookSource &source : m_sources) {
        if (!source.disabled() && source.hasSearch()) {
            searchable.append(source);
        }
    }
    return searchable;
}

BookSource* RuleManager::getSourceById(int id)
{
    QMutexLocker locker(&m_mutex);
    return m_sourceById.value(id, nullptr);
}

const BookSource* RuleManager::getSourceById(int id) const
{
    QMutexLocker locker(&m_mutex);
    return m_sourceById.value(id, nullptr);
}

BookSource* RuleManager::getSourceByName(const QString &name)
{
    QMutexLocker locker(&m_mutex);
    return m_sourceByName.value(name, nullptr);
}

const BookSource* RuleManager::getSourceByName(const QString &name) const
{
    QMutexLocker locker(&m_mutex);
    return m_sourceByName.value(name, nullptr);
}

BookSource* RuleManager::matchSourceByUrl(const QString &url)
{
    QMutexLocker locker(&m_mutex);


    for (BookSource *source : m_sourcesByUrl) {
        if (source && url.startsWith(source->url())) {
            return source;
        }
    }
    return nullptr;
}

const BookSource* RuleManager::matchSourceByUrl(const QString &url) const
{
    QMutexLocker locker(&m_mutex);

    for (BookSource *source : m_sourcesByUrl) {
        if (source && url.startsWith(source->url())) {
            return source;
        }
    }
    return nullptr;
}

bool RuleManager::addSource(const BookSource &source)
{
    QMutexLocker locker(&m_mutex);

    if (!validateSource(source)) {
        setError("Source validation failed");
        return false;
    }

    if (hasSource(source.id())) {
        setError(QString("Source ID already exists: %1").arg(source.id()));
        return false;
    }

    m_sources.append(source);
    updateSourceIndex();

    emit sourceAdded(source.id());
    qDebug() << "RuleManager: Add a book source:" << source.name();
    return true;
}

bool RuleManager::updateSource(const BookSource &source)
{
    QMutexLocker locker(&m_mutex);

    if (!validateSource(source)) {
        setError("Book source verification failed");
        return false;
    }

    for (int i = 0; i < m_sources.size(); ++i) {
        if (m_sources[i].id() == source.id()) {
            m_sources[i] = source;
            updateSourceIndex();
            emit sourceUpdated(source.id());
            qDebug() << "RuleManager: Update book source:" << source.name();
            return true;
        }
    }

    setError(QString("No source found to update: %1").arg(source.id()));
    return false;
}

bool RuleManager::removeSource(int id)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_sources.size(); ++i) {
        if (m_sources[i].id() == id) {
            QString name = m_sources[i].name();
            m_sources.removeAt(i);
            updateSourceIndex();
            emit sourceRemoved(id);
            qDebug() << "RuleManager: Delete the book source:" << name;
            return true;
        }
    }

    setError(QString("The book source to be deleted was not found: %1").arg(id));
    return false;
}

bool RuleManager::enableSource(int id, bool enabled)
{
    QMutexLocker locker(&m_mutex);

    BookSource *source = m_sourceById.value(id, nullptr);
    if (!source) {
        setError(QString("No book source found: %1").arg(id));
        return false;
    }

    source->setDisabled(!enabled);
    emit sourceUpdated(id);
    qDebug() << "RuleManager:" << (enabled ? "enabled" : "disabled") << "book source:" << source->name();
    return true;
}

bool RuleManager::validateSource(const BookSource &source) const
{

    if (source.id() < 0) {
        return false;
    }

    if (source.url().isEmpty() || source.name().isEmpty()) {
        return false;
    }


    if (!source.url().startsWith("http://") && !source.url().startsWith("https://")) {
        return false;
    }


    if (source.hasSearch()) {
        const SearchRule *searchRule = source.searchRule();
        if (searchRule->url().isEmpty() || searchRule->result().isEmpty()) {
            return false;
        }
    }

    return true;
}

QStringList RuleManager::validateAllSources() const
{
    QMutexLocker locker(&m_mutex);

    QStringList errors;
    for (const BookSource &source : m_sources) {
        if (!validateSource(source)) {
            errors.append(QString("Book source verification failed: %1 (%2)").arg(source.name()).arg(source.id()));
        }
    }
    return errors;
}

SelectorConverter::ParsedSelector RuleManager::parseSelector(const QString &cssSelector) const
{
    return m_selectorConverter.parseSelector(cssSelector);
}

QRegularExpression RuleManager::selectorToRegex(const QString &cssSelector) const
{
    return m_selectorConverter.toRegex(cssSelector);
}

int RuleManager::getSourceCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_sources.size();
}

int RuleManager::getEnabledSourceCount() const
{
    QMutexLocker locker(&m_mutex);

    int count = 0;
    for (const BookSource &source : m_sources) {
        if (!source.disabled()) {
            count++;
        }
    }
    return count;
}

bool RuleManager::hasSource(int id) const
{
    QMutexLocker locker(&m_mutex);
    return m_sourceById.contains(id);
}

void RuleManager::enableFileWatching(bool enable)
{
    m_fileWatchingEnabled = enable;

    if (enable) {

        for (const QString &filePath : m_loadedFiles) {
            if (QFileInfo::exists(filePath) && !m_fileWatcher->files().contains(filePath)) {
                m_fileWatcher->addPath(filePath);
            }
        }
    } else {

        if (!m_fileWatcher->files().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->files());
        }
        if (!m_fileWatcher->directories().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->directories());
        }
    }

    qDebug() << "RuleManager: file monitoring" << (enable ? "enable" : "disable");
}

void RuleManager::onFileChanged(const QString &path)
{
    qDebug() << "RuleManager: File change detected:" << path;


    QTimer::singleShot(500, this, [this, path]() {
        if (QFileInfo::exists(path)) {

            QMutexLocker locker(&m_mutex);


            QList<int> idsToRemove;
            for (const BookSource &source : m_sources) {

                idsToRemove.append(source.id());
            }

            for (int id : idsToRemove) {
                removeSource(id);
            }


            if (loadRulesFromFile(path)) {
                emit fileChanged(path);
            }
        } else {
            qWarning() << "RuleManager: File has been deleted:" << path;
        }
    });
}

void RuleManager::onDirectoryChanged(const QString &path)
{
    qDebug() << "RuleManager: Catalog change detected:" << path;


    QTimer::singleShot(1000, this, [this, path]() {
        loadRulesFromDirectory(path);
    });
}

void RuleManager::setError(const QString &error)
{
    m_lastError = error;
    qWarning() << "RuleManager:" << error;
    emit errorOccurred(error);
}

void RuleManager::updateSourceIndex()
{
    qDebug() << "RuleManager::updateSourceIndex - START";
    m_sourceById.clear();
    m_sourceByName.clear();
    m_sourcesByUrl.clear();
    qDebug() << "RuleManager::updateSourceIndex - Cleared indexes";

    qDebug() << "RuleManager::updateSourceIndex - Processing" << m_sources.size() << "sources";
    for (BookSource &source : m_sources) {
        qDebug() << "RuleManager::updateSourceIndex - Processing source:" << source.name() << "ID:" << source.id();
        m_sourceById[source.id()] = &source;
        m_sourceByName[source.name()] = &source;
        m_sourcesByUrl.append(&source);
    }
    qDebug() << "RuleManager::updateSourceIndex - Finished processing sources";

    qDebug() << "RuleManager::updateSourceIndex - Starting sort";
    std::sort(m_sourcesByUrl.begin(), m_sourcesByUrl.end(),
              [](const BookSource *a, const BookSource *b) {
                  return a->url().length() > b->url().length();
              });
    qDebug() << "RuleManager::updateSourceIndex - Sort completed";
    qDebug() << "RuleManager::updateSourceIndex - FINISHED";
}

bool RuleManager::isValidRuleFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);

    if (fileInfo.suffix().toLower() != "json") {
        return false;
    }


    if (fileInfo.size() > 50 * 1024 * 1024) { 
        return false;
    }

    return true;
}
