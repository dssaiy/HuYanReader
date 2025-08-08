#include "NovelConfig.h"

NovelConfig::NovelConfig(QObject* parent) : QObject(parent)
{
    QString exeDir = QCoreApplication::applicationDirPath();

    m_configFilePath = exeDir + "/novelconfig.ini";

    m_settings = new QSettings(m_configFilePath, QSettings::IniFormat, this);

    if (!hasConfig()) {
        setDefaults();
    }
    else {
        loadConfig();
    }

    saveConfig();
}

NovelConfig::~NovelConfig()
{
    saveConfig();
}

bool NovelConfig::hasConfig() const
{
    QFileInfo checkFile(m_configFilePath);
    return checkFile.exists() && checkFile.isFile();
}

void NovelConfig::setDefaults()
{
    // 基础配置 - 参考原程序so-novel-1.8.5的config.ini
    m_language = "zh_CN";
    m_activeRules = "rules/https-test-rules.json";  // 使用HTTPS测试规则作为默认
    m_downloadPath = "downloads";
    m_extName = "txt";
    m_sourceId = -1;
    m_searchLimit = 20;
    m_autoUpdate = false;

    // 爬取限制配置
    m_crawlMinInterval = 200;
    m_crawlMaxInterval = 400;
    m_crawlThreads = -1;  // -1表示自动设置
    m_preserveChapterCache = false;

    // 重试配置
    m_maxRetryAttempts = 5;
    m_retryMinInterval = 2000;
    m_retryMaxInterval = 4000;

    // 代理配置
    m_proxyEnabled = false;
    m_proxyHost = "127.0.0.1";
    m_proxyPort = 8080;
}

void NovelConfig::loadConfig()
{
    m_settings->beginGroup("base");

    m_language = m_settings->value("language", "zh_CN").toString();
    m_activeRules = m_settings->value("active-rules", "rules/https-test-rules.json").toString();
    m_downloadPath = m_settings->value("download-path", "downloads").toString();
    m_extName = m_settings->value("extname", "txt").toString();
    m_sourceId = m_settings->value("source-id", -1).toInt();
    m_searchLimit = m_settings->value("search-limit", 20).toInt();
    m_autoUpdate = m_settings->value("auto-update", false).toBool();

    m_settings->endGroup();

    m_settings->beginGroup("crawl");

    m_crawlMinInterval = m_settings->value("min", 200).toInt();
    m_crawlMaxInterval = m_settings->value("max", 400).toInt();
    m_crawlThreads = m_settings->value("threads", -1).toInt();
    m_preserveChapterCache = m_settings->value("preserve_chapter_cache", false).toBool();

    m_settings->endGroup();

    m_settings->beginGroup("retry");

    m_maxRetryAttempts = m_settings->value("max-attempts", 5).toInt();
    m_retryMinInterval = m_settings->value("min", 2000).toInt();
    m_retryMaxInterval = m_settings->value("max", 4000).toInt();

    m_settings->endGroup();

    m_settings->beginGroup("proxy");

    m_proxyEnabled = m_settings->value("enabled", false).toBool();
    m_proxyHost = m_settings->value("host", "127.0.0.1").toString();
    m_proxyPort = m_settings->value("port", 8080).toInt();

    m_settings->endGroup();
}

void NovelConfig::saveConfig()
{
    m_settings->beginGroup("base");

    m_settings->setValue("language", m_language);
    m_settings->setValue("active-rules", m_activeRules);
    m_settings->setValue("download-path", m_downloadPath);
    m_settings->setValue("extname", m_extName);
    m_settings->setValue("source-id", m_sourceId);
    m_settings->setValue("search-limit", m_searchLimit);
    m_settings->setValue("auto-update", m_autoUpdate);

    m_settings->endGroup();

    m_settings->beginGroup("crawl");

    m_settings->setValue("min", m_crawlMinInterval);
    m_settings->setValue("max", m_crawlMaxInterval);
    m_settings->setValue("threads", m_crawlThreads);
    m_settings->setValue("preserve_chapter_cache", m_preserveChapterCache);

    m_settings->endGroup();

    m_settings->beginGroup("retry");

    m_settings->setValue("max-attempts", m_maxRetryAttempts);
    m_settings->setValue("min", m_retryMinInterval);
    m_settings->setValue("max", m_retryMaxInterval);

    m_settings->endGroup();

    m_settings->beginGroup("proxy");

    m_settings->setValue("enabled", m_proxyEnabled);
    m_settings->setValue("host", m_proxyHost);
    m_settings->setValue("port", m_proxyPort);

    m_settings->endGroup();

    m_settings->sync();
}

// 基础配置设置方法
void NovelConfig::setLanguage(const QString& language)
{
    if (m_language != language) {
        m_language = language;
        emit configChanged();
    }
}

void NovelConfig::setActiveRules(const QString& activeRules)
{
    if (m_activeRules != activeRules) {
        m_activeRules = activeRules;
        saveConfig(); // Must save immediately before emitting signal to ensure config file is updated
        emit configChanged();
    }
}

void NovelConfig::setDownloadPath(const QString& downloadPath)
{
    if (m_downloadPath != downloadPath) {
        m_downloadPath = downloadPath;
        emit configChanged();
    }
}

void NovelConfig::setExtName(const QString& extName)
{
    if (m_extName != extName) {
        m_extName = extName;
        emit configChanged();
    }
}

void NovelConfig::setSourceId(int sourceId)
{
    if (m_sourceId != sourceId) {
        m_sourceId = sourceId;
        emit configChanged();
    }
}

void NovelConfig::setSearchLimit(int searchLimit)
{
    if (m_searchLimit != searchLimit) {
        m_searchLimit = searchLimit;
        emit configChanged();
    }
}

void NovelConfig::setAutoUpdate(bool autoUpdate)
{
    if (m_autoUpdate != autoUpdate) {
        m_autoUpdate = autoUpdate;
        emit configChanged();
    }
}
// 爬取限制配置设置方法
void NovelConfig::setCrawlMinInterval(int minInterval)
{
    if (m_crawlMinInterval != minInterval) {
        m_crawlMinInterval = minInterval;
        emit configChanged();
    }
}

void NovelConfig::setCrawlMaxInterval(int maxInterval)
{
    if (m_crawlMaxInterval != maxInterval) {
        m_crawlMaxInterval = maxInterval;
        emit configChanged();
    }
}

void NovelConfig::setCrawlThreads(int threads)
{
    if (m_crawlThreads != threads) {
        m_crawlThreads = threads;
        emit configChanged();
    }
}

void NovelConfig::setPreserveChapterCache(bool preserve)
{
    if (m_preserveChapterCache != preserve) {
        m_preserveChapterCache = preserve;
        emit configChanged();
    }
}

// 重试配置设置方法
void NovelConfig::setMaxRetryAttempts(int maxAttempts)
{
    if (m_maxRetryAttempts != maxAttempts) {
        m_maxRetryAttempts = maxAttempts;
        emit configChanged();
    }
}

void NovelConfig::setRetryMinInterval(int minInterval)
{
    if (m_retryMinInterval != minInterval) {
        m_retryMinInterval = minInterval;
        emit configChanged();
    }
}

void NovelConfig::setRetryMaxInterval(int maxInterval)
{
    if (m_retryMaxInterval != maxInterval) {
        m_retryMaxInterval = maxInterval;
        emit configChanged();
    }
}

// 代理配置设置方法
void NovelConfig::setProxyEnabled(bool enabled)
{
    if (m_proxyEnabled != enabled) {
        m_proxyEnabled = enabled;
        emit configChanged();
    }
}

void NovelConfig::setProxyHost(const QString& host)
{
    if (m_proxyHost != host) {
        m_proxyHost = host;
        emit configChanged();
    }
}

void NovelConfig::setProxyPort(int port)
{
    if (m_proxyPort != port) {
        m_proxyPort = port;
        emit configChanged();
    }
}

// 基础配置获取方法
QString NovelConfig::getLanguage() const
{
    return m_language;
}

QString NovelConfig::getActiveRules() const
{
    return m_activeRules;
}

QString NovelConfig::getDownloadPath() const
{
    return m_downloadPath;
}

QString NovelConfig::getExtName() const
{
    return m_extName;
}

int NovelConfig::getSourceId() const
{
    return m_sourceId;
}

int NovelConfig::getSearchLimit() const
{
    return m_searchLimit;
}

bool NovelConfig::getAutoUpdate() const
{
    return m_autoUpdate;
}

// 爬取限制配置获取方法
int NovelConfig::getCrawlMinInterval() const
{
    return m_crawlMinInterval;
}

int NovelConfig::getCrawlMaxInterval() const
{
    return m_crawlMaxInterval;
}

int NovelConfig::getCrawlThreads() const
{
    return m_crawlThreads;
}

bool NovelConfig::getPreserveChapterCache() const
{
    return m_preserveChapterCache;
}

// 重试配置获取方法
int NovelConfig::getMaxRetryAttempts() const
{
    return m_maxRetryAttempts;
}

int NovelConfig::getRetryMinInterval() const
{
    return m_retryMinInterval;
}

int NovelConfig::getRetryMaxInterval() const
{
    return m_retryMaxInterval;
}

// 代理配置获取方法
bool NovelConfig::getProxyEnabled() const
{
    return m_proxyEnabled;
}

QString NovelConfig::getProxyHost() const
{
    return m_proxyHost;
}

int NovelConfig::getProxyPort() const
{
    return m_proxyPort;
}
