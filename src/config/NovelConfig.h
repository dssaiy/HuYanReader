#pragma once
#include <QObject>
#include <QString>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QMessageBox>

/**
 * @brief NovelConfig class - Manages novel search and download configuration
 *
 * Independent configuration management class for NovelSearch module
 * References Settings.h implementation pattern, uses QSettings to manage novelconfig.ini
 * Compatible with original program so-novel-1.8.5 configuration format
 */
class NovelConfig : public QObject {
    Q_OBJECT

public:
    explicit NovelConfig(QObject* parent = nullptr);
    ~NovelConfig();

    // Basic configuration setters
    void setLanguage(const QString& language);
    void setActiveRules(const QString& activeRules);
    void setDownloadPath(const QString& downloadPath);
    void setExtName(const QString& extName);
    void setSourceId(int sourceId);
    void setSearchLimit(int searchLimit);
    void setAutoUpdate(bool autoUpdate);

    // Crawl limit configuration setters
    void setCrawlMinInterval(int minInterval);
    void setCrawlMaxInterval(int maxInterval);
    void setCrawlThreads(int threads);
    void setPreserveChapterCache(bool preserve);

    // Retry configuration setters
    void setMaxRetryAttempts(int maxAttempts);
    void setRetryMinInterval(int minInterval);
    void setRetryMaxInterval(int maxInterval);

    // Proxy configuration setters
    void setProxyEnabled(bool enabled);
    void setProxyHost(const QString& host);
    void setProxyPort(int port);

    // Basic configuration getters
    QString getLanguage() const;
    QString getActiveRules() const;
    QString getDownloadPath() const;
    QString getExtName() const;
    int getSourceId() const;
    int getSearchLimit() const;
    bool getAutoUpdate() const;

    // Crawl limit configuration getters
    int getCrawlMinInterval() const;
    int getCrawlMaxInterval() const;
    int getCrawlThreads() const;
    bool getPreserveChapterCache() const;

    // Retry configuration getters
    int getMaxRetryAttempts() const;
    int getRetryMinInterval() const;
    int getRetryMaxInterval() const;

    // Proxy configuration getters
    bool getProxyEnabled() const;
    QString getProxyHost() const;
    int getProxyPort() const;

    // Configuration file management
    void saveConfig();
    void loadConfig();
    bool hasConfig() const;
    QSettings* getSettings() { return m_settings; }

signals:
    void configChanged();

private:
    void setDefaults();

    // Basic configuration items
    QString m_language;
    QString m_activeRules;
    QString m_downloadPath;
    QString m_extName;
    int m_sourceId;
    int m_searchLimit;
    bool m_autoUpdate;

    // Crawl limit configuration items
    int m_crawlMinInterval;
    int m_crawlMaxInterval;
    int m_crawlThreads;
    bool m_preserveChapterCache;

    // Retry configuration items
    int m_maxRetryAttempts;
    int m_retryMinInterval;
    int m_retryMaxInterval;

    // Proxy configuration items
    bool m_proxyEnabled;
    QString m_proxyHost;
    int m_proxyPort;

    // Configuration file management
    QSettings* m_settings;
    QString m_configFilePath;
};
