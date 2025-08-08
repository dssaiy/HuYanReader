#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QTimer>
#include <QJsonObject>
#include <QStringList>
#include <QMutex>
#include <QRandomGenerator>
#include <QThread>
#include <QSslError>
#include <QEventLoop>
#include <QMetaObject>

/**
 * @brief Custom Cookie manager providing public access interface
 */
class CustomCookieJar : public QNetworkCookieJar
{
public:
    explicit CustomCookieJar(QObject *parent = nullptr) : QNetworkCookieJar(parent) {}

    // Public interface
    void setAllCookiesPublic(const QList<QNetworkCookie> &cookieList) {
        setAllCookies(cookieList);
    }

    QList<QNetworkCookie> allCookiesPublic() const {
        return allCookies();
    }
};



/**
 * @brief HTTP client class, wrapping QNetworkAccessManager
 * Provides GET/POST requests, Cookie management, User-Agent rotation, error handling and retry mechanism
 * Uses simple synchronous requests in main thread for reliability
 */
class HttpClient : public QObject
{
    Q_OBJECT

public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient();

    // HTTP request methods (original async methods)
    QNetworkReply* get(const QString &url, const QJsonObject &headers = QJsonObject());
    QNetworkReply* post(const QString &url, const QByteArray &data, const QJsonObject &headers = QJsonObject());
    QNetworkReply* post(const QString &url, const QJsonObject &formData, const QJsonObject &headers = QJsonObject());

    // Thread-safe synchronous methods
    QString getSync(const QString &url, const QJsonObject &headers = QJsonObject(), bool *success = nullptr, QString *error = nullptr);
    QString postSync(const QString &url, const QByteArray &data, const QJsonObject &headers = QJsonObject(), bool *success = nullptr, QString *error = nullptr);

    // Get response cookies from last request
    QList<QNetworkCookie> getLastResponseCookies() const;

    void setTimeout(int timeoutMs);
    void setMaxRetries(int maxRetries);
    void setRetryDelay(int delayMs);
    void setUserAgents(const QStringList &userAgents);
    void addUserAgent(const QString &userAgent);
    void enableCookies(bool enable = true);
    void clearCookies();

    void setCookie(const QString &name, const QString &value, const QString &domain = QString());
    QString getCookie(const QString &name, const QString &domain = QString()) const;
    void loadCookiesFromFile(const QString &filePath);
    void saveCookiesToFile(const QString &filePath);

    bool isRequestInProgress() const;
    int getActiveRequestCount() const;

signals:
    void requestFinished(QNetworkReply *reply);
    void requestError(QNetworkReply::NetworkError error, const QString &errorString);
    void retryAttempt(int attempt, int maxAttempts);

private slots:
    void onRequestFinished();
    void onRequestError(QNetworkReply::NetworkError error);
    void onTimeout();

private:
    void initializeNetworkManager();
    QNetworkRequest createRequest(const QString &url, const QJsonObject &headers = QJsonObject());
    QString getRandomUserAgent() const;
    void setupRequest(QNetworkRequest &request, const QJsonObject &headers);
    void handleReply(QNetworkReply *reply);
    void retryRequest(QNetworkReply *reply);
    QByteArray jsonToFormData(const QJsonObject &json);

    struct RetryInfo {
        QString url;
        QByteArray data;
        QJsonObject headers;
        QString method;
        int attempts;
        QTimer *timer;

        RetryInfo() : attempts(0), timer(nullptr) {}
    };

    QNetworkAccessManager *m_networkManager;
    CustomCookieJar *m_cookieJar;

    int m_timeout;
    int m_maxRetries;
    int m_retryDelay;
    QStringList m_userAgents;
    bool m_cookiesEnabled;

    QList<QNetworkReply*> m_activeReplies;
    QHash<QNetworkReply*, RetryInfo*> m_retryMap;
    mutable QMutex m_mutex;

    // Store last response cookies for extraction
    QList<QNetworkCookie> m_lastResponseCookies;

    // Simple synchronous network operations
    QString performSyncRequest(const QString &method, const QString &url, const QByteArray &data, const QJsonObject &headers, bool *success, QString *error);

    static const QStringList DEFAULT_USER_AGENTS;
};

#endif // HTTPCLIENT_H
