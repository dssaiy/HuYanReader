#include "HttpClient.h"
#include <QNetworkCookie>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslError>
#include <QSslKey>
#include <QNetworkConfiguration>
#include <QCoreApplication>


const QStringList HttpClient::DEFAULT_USER_AGENTS = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:127.0) Gecko/20100101 Firefox/127.0",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:126.0) Gecko/20100101 Firefox/126.0",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64; rv:127.0) Gecko/20100101 Firefox/127.0"
};







HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_cookieJar(nullptr)
    , m_timeout(15000)  // 15秒超时
    , m_maxRetries(3)
    , m_retryDelay(2000)
    , m_userAgents(DEFAULT_USER_AGENTS)
    , m_cookiesEnabled(true)
{
    qDebug() << "HttpClient::HttpClient - Constructor START";
    initializeNetworkManager();
    qDebug() << "HttpClient::HttpClient - Constructor FINISHED";
}

void HttpClient::initializeNetworkManager()
{
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
        m_cookieJar = new CustomCookieJar(this);
        m_networkManager->setCookieJar(m_cookieJar);

        connect(m_networkManager, &QNetworkAccessManager::finished,
                this, &HttpClient::onRequestFinished);

        // Enhanced SSL error handling for novel websites
        connect(m_networkManager, &QNetworkAccessManager::sslErrors,
                this, [](QNetworkReply *reply, const QList<QSslError> &errors) {
                    qDebug() << "HttpClient::SSL errors detected for" << reply->url().toString();
                    for (const QSslError &error : errors) {
                        qDebug() << "  SSL Error:" << error.errorString();
                    }
                    qDebug() << "HttpClient::Ignoring ALL SSL errors for novel website compatibility";
                    reply->ignoreSslErrors();  // Ignore all SSL errors
                });

        // Configure SSL settings for better HTTPS compatibility
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();

        // Completely disable SSL verification for novel websites
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig.setPeerVerifyDepth(0);
        sslConfig.setProtocol(QSsl::AnyProtocol);

        // Clear all certificate requirements
        sslConfig.setCaCertificates(QList<QSslCertificate>());
        sslConfig.setLocalCertificateChain(QList<QSslCertificate>());

        // Set global SSL configuration
        QSslConfiguration::setDefaultConfiguration(sslConfig);

        qDebug() << "HttpClient: Maximum permissive SSL configuration applied";
        qDebug() << "HttpClient: SSL support available:" << QSslSocket::supportsSsl();
        qDebug() << "HttpClient: SSL library build version:" << QSslSocket::sslLibraryBuildVersionString();
        qDebug() << "HttpClient: SSL library runtime version:" << QSslSocket::sslLibraryVersionString();
    }
}

HttpClient::~HttpClient()
{
    for (auto reply : m_activeReplies) {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    }


    for (auto retryInfo : m_retryMap.values()) {
        if (retryInfo->timer) {
            retryInfo->timer->stop();
            delete retryInfo->timer;
        }
        delete retryInfo;
    }
}

QNetworkReply* HttpClient::get(const QString &url, const QJsonObject &headers)
{
    initializeNetworkManager();
    QNetworkRequest request = createRequest(url, headers);
    QNetworkReply *reply = m_networkManager->get(request);
    handleReply(reply);


    if (m_maxRetries > 0) {
        RetryInfo *retryInfo = new RetryInfo();
        retryInfo->url = url;
        retryInfo->headers = headers;
        retryInfo->method = "GET";
        retryInfo->attempts = 0;
        m_retryMap[reply] = retryInfo;
    }

    return reply;
}

QNetworkReply* HttpClient::post(const QString &url, const QByteArray &data, const QJsonObject &headers)
{
    initializeNetworkManager();
    QNetworkRequest request = createRequest(url, headers);
    QNetworkReply *reply = m_networkManager->post(request, data);
    handleReply(reply);
    

    if (m_maxRetries > 0) {
        RetryInfo *retryInfo = new RetryInfo();
        retryInfo->url = url;
        retryInfo->data = data;
        retryInfo->headers = headers;
        retryInfo->method = "POST";
        retryInfo->attempts = 0;
        m_retryMap[reply] = retryInfo;
    }
    
    return reply;
}

QNetworkReply* HttpClient::post(const QString &url, const QJsonObject &formData, const QJsonObject &headers)
{
    initializeNetworkManager();
    QByteArray data = jsonToFormData(formData);
    QJsonObject postHeaders = headers;
    postHeaders["Content-Type"] = "application/x-www-form-urlencoded";
    return post(url, data, postHeaders);
}

void HttpClient::setTimeout(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    m_timeout = timeoutMs;
}

void HttpClient::setMaxRetries(int maxRetries)
{
    QMutexLocker locker(&m_mutex);
    m_maxRetries = maxRetries;
}

void HttpClient::setRetryDelay(int delayMs)
{
    QMutexLocker locker(&m_mutex);
    m_retryDelay = delayMs;
}

void HttpClient::setUserAgents(const QStringList &userAgents)
{
    QMutexLocker locker(&m_mutex);
    m_userAgents = userAgents.isEmpty() ? DEFAULT_USER_AGENTS : userAgents;
}

void HttpClient::addUserAgent(const QString &userAgent)
{
    QMutexLocker locker(&m_mutex);
    if (!userAgent.isEmpty() && !m_userAgents.contains(userAgent)) {
        m_userAgents.append(userAgent);
    }
}

void HttpClient::enableCookies(bool enable)
{
    QMutexLocker locker(&m_mutex);
    m_cookiesEnabled = enable;
}

void HttpClient::clearCookies()
{
    if (m_cookieJar) {
        m_cookieJar->setAllCookiesPublic(QList<QNetworkCookie>());
    }
}

void HttpClient::setCookie(const QString &name, const QString &value, const QString &domain)
{
    if (!m_cookiesEnabled || !m_cookieJar) return;
    
    QNetworkCookie cookie(name.toUtf8(), value.toUtf8());
    if (!domain.isEmpty()) {
        cookie.setDomain(domain);
    }
    
    QList<QNetworkCookie> cookies = m_cookieJar->allCookiesPublic();
    cookies.append(cookie);
    m_cookieJar->setAllCookiesPublic(cookies);
}

QString HttpClient::getCookie(const QString &name, const QString &domain) const
{
    if (!m_cookiesEnabled || !m_cookieJar) return QString();
    
    QList<QNetworkCookie> cookies = m_cookieJar->allCookiesPublic();
    for (const auto &cookie : cookies) {
        if (cookie.name() == name.toUtf8()) {
            if (domain.isEmpty() || cookie.domain() == domain) {
                return QString::fromUtf8(cookie.value());
            }
        }
    }
    return QString();
}

bool HttpClient::isRequestInProgress() const
{
    QMutexLocker locker(&m_mutex);
    return !m_activeReplies.isEmpty();
}

int HttpClient::getActiveRequestCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeReplies.size();
}

QNetworkRequest HttpClient::createRequest(const QString &url, const QJsonObject &headers)
{
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    setupRequest(request, headers);
    return request;
}

QString HttpClient::getRandomUserAgent() const
{
    if (m_userAgents.isEmpty()) return QString();
    
    int index = QRandomGenerator::global()->bounded(m_userAgents.size());
    return m_userAgents.at(index);
}

void HttpClient::setupRequest(QNetworkRequest &request, const QJsonObject &headers)
{
    // 使用curl的User-Agent，因为curl测试成功了
    request.setRawHeader("User-Agent", "curl/7.68.0");

    // 使用curl的默认Headers
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "keep-alive");

    qDebug() << "HttpClient: Using curl-compatible headers";

    // Enable automatic redirect following for all redirects
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);
    qDebug() << "HttpClient: Automatic redirect following enabled (UserVerified)";

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }


#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(m_timeout);
#endif
}

void HttpClient::handleReply(QNetworkReply *reply)
{
    if (!reply) return;
    
    QMutexLocker locker(&m_mutex);
    m_activeReplies.append(reply);
    

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &HttpClient::onRequestError);
#else
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &HttpClient::onRequestError);
#endif
}

QByteArray HttpClient::jsonToFormData(const QJsonObject &json)
{
    QUrlQuery query;
    for (auto it = json.begin(); it != json.end(); ++it) {
        query.addQueryItem(it.key(), it.value().toString());
    }
    return query.toString(QUrl::FullyEncoded).toUtf8();
}

void HttpClient::onRequestFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    {
        QMutexLocker locker(&m_mutex);
        m_activeReplies.removeAll(reply);


        if (m_retryMap.contains(reply)) {
            RetryInfo *retryInfo = m_retryMap.take(reply);
            if (retryInfo->timer) {
                retryInfo->timer->stop();
                delete retryInfo->timer;
            }
            delete retryInfo;
        }
    }

    emit requestFinished(reply);
    reply->deleteLater();
}

void HttpClient::onRequestError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    qDebug() << "HttpClient: Request error:" << error << reply->errorString();


    if (m_retryMap.contains(reply)) {
        RetryInfo *retryInfo = m_retryMap[reply];
        if (retryInfo->attempts < m_maxRetries) {
            retryRequest(reply);
            return;
        }
    }

    emit requestError(error, reply->errorString());
}

void HttpClient::onTimeout()
{
    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;


    for (auto it = m_retryMap.begin(); it != m_retryMap.end(); ++it) {
        if (it.value()->timer == timer) {
            QNetworkReply *originalReply = it.key();
            RetryInfo *retryInfo = it.value();


            retryInfo->attempts++;
            emit retryAttempt(retryInfo->attempts, m_maxRetries);

            QNetworkReply *newReply = nullptr;
            if (retryInfo->method == "GET") {
                newReply = get(retryInfo->url, retryInfo->headers);
            } else if (retryInfo->method == "POST") {
                newReply = post(retryInfo->url, retryInfo->data, retryInfo->headers);
            }

            if (newReply) {

                m_retryMap.remove(originalReply);
                m_retryMap[newReply] = retryInfo;
                retryInfo->timer = nullptr;
            }


            originalReply->abort();
            originalReply->deleteLater();

            timer->deleteLater();
            break;
        }
    }
}

void HttpClient::retryRequest(QNetworkReply *reply)
{
    if (!m_retryMap.contains(reply)) return;

    RetryInfo *retryInfo = m_retryMap[reply];
    retryInfo->attempts++;

    qDebug() << "HttpClient: Retrying request, attempt" << retryInfo->attempts << "of" << m_maxRetries;
    emit retryAttempt(retryInfo->attempts, m_maxRetries);


    if (!retryInfo->timer) {
        retryInfo->timer = new QTimer();
        retryInfo->timer->setSingleShot(true);
        connect(retryInfo->timer, &QTimer::timeout, this, &HttpClient::onTimeout);
    }

    retryInfo->timer->start(m_retryDelay);
}

void HttpClient::loadCookiesFromFile(const QString &filePath)
{
    if (!m_cookiesEnabled || !m_cookieJar) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "HttpClient: Failed to load cookies from" << filePath;
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    QList<QNetworkCookie> cookies;
    QJsonArray cookieArray = doc.array();

    for (const auto &value : cookieArray) {
        QJsonObject cookieObj = value.toObject();
        QNetworkCookie cookie(
            cookieObj["name"].toString().toUtf8(),
            cookieObj["value"].toString().toUtf8()
        );

        if (cookieObj.contains("domain")) {
            cookie.setDomain(cookieObj["domain"].toString());
        }
        if (cookieObj.contains("path")) {
            cookie.setPath(cookieObj["path"].toString());
        }

        cookies.append(cookie);
    }

    m_cookieJar->setAllCookiesPublic(cookies);
    qDebug() << "HttpClient: Loaded" << cookies.size() << "cookies from" << filePath;
}

void HttpClient::saveCookiesToFile(const QString &filePath)
{
    if (!m_cookiesEnabled || !m_cookieJar) return;

    QList<QNetworkCookie> cookies = m_cookieJar->allCookiesPublic();
    QJsonArray cookieArray;

    for (const auto &cookie : cookies) {
        QJsonObject cookieObj;
        cookieObj["name"] = QString::fromUtf8(cookie.name());
        cookieObj["value"] = QString::fromUtf8(cookie.value());
        cookieObj["domain"] = cookie.domain();
        cookieObj["path"] = cookie.path();
        cookieArray.append(cookieObj);
    }

    QJsonDocument doc(cookieArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "HttpClient: Failed to save cookies to" << filePath;
        return;
    }

    file.write(doc.toJson());
    qDebug() << "HttpClient: Saved" << cookies.size() << "cookies to" << filePath;
}





QString HttpClient::getSync(const QString &url, const QJsonObject &headers, bool *success, QString *error)
{
    return performSyncRequest("GET", url, QByteArray(), headers, success, error);
}

QString HttpClient::postSync(const QString &url, const QByteArray &data, const QJsonObject &headers, bool *success, QString *error)
{
    return performSyncRequest("POST", url, data, headers, success, error);
}

QString HttpClient::performSyncRequest(const QString &method, const QString &url, const QByteArray &data, const QJsonObject &headers, bool *success, QString *error)
{
    qDebug() << "HttpClient::performSyncRequest - START" << method << url;

    // Create request
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    // Set default headers
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("User-Agent", "curl/7.68.0");

    // Set custom headers
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        qDebug() << "HttpClient: Setting header" << it.key() << "=" << it.value().toString();
    }

    qDebug() << "HttpClient: Request data:" << data;

    // Create reply
    QNetworkReply *reply = nullptr;
    if (method.toLower() == "post") {
        reply = m_networkManager->post(request, data);
        qDebug() << "HttpClient: POST request created, reply pointer:" << reply;
    } else {
        reply = m_networkManager->get(request);
        qDebug() << "HttpClient: GET request created, reply pointer:" << reply;
    }

    if (!reply) {
        qDebug() << "HttpClient: ERROR - Failed to create network request";
        if (success) *success = false;
        if (error) *error = "Failed to create network request";
        return QString();
    }

    // Wait for completion using event loop
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    bool timedOut = false;

    // Connect signals
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, [&]() {
        timedOut = true;
        loop.quit();
    });

    // Start timeout timer
    timeoutTimer.start(m_timeout);

    qDebug() << "HttpClient: Starting event loop, timeout:" << m_timeout << "ms";
    loop.exec();

    // Stop timer
    timeoutTimer.stop();

    // Check for timeout
    if (timedOut) {
        qDebug() << "HttpClient: Request timed out";
        reply->abort();
        reply->deleteLater();
        if (success) *success = false;
        if (error) *error = "Request timeout";
        return QString();
    }

    // Process response
    bool requestSuccess = (reply->error() == QNetworkReply::NoError);
    QString response;
    QString requestError;

    qDebug() << "HttpClient: Request finished with error code:" << reply->error();

    // Check for redirects
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QUrl finalUrl = reply->url();
    QUrl originalUrl = reply->request().url();

    qDebug() << "HttpClient: HTTP status code:" << statusCode;
    qDebug() << "HttpClient: Original URL:" << originalUrl.toString();
    qDebug() << "HttpClient: Final URL:" << finalUrl.toString();

    if (originalUrl != finalUrl) {
        qDebug() << "HttpClient: Redirect detected - from" << originalUrl.toString() << "to" << finalUrl.toString();
    }

    if (requestSuccess) {
        response = QString::fromUtf8(reply->readAll());
        qDebug() << "HttpClient: SUCCESS - Response length:" << response.length();
    } else {
        requestError = reply->errorString();
        qDebug() << "HttpClient: ERROR -" << requestError;
    }

    // Capture response cookies for later extraction
    QVariant cookieVar = reply->header(QNetworkRequest::SetCookieHeader);
    if (cookieVar.isValid()) {
        m_lastResponseCookies = qvariant_cast<QList<QNetworkCookie>>(cookieVar);
        qDebug() << "HttpClient: Captured" << m_lastResponseCookies.size() << "response cookies";
    } else {
        m_lastResponseCookies.clear();
    }

    // Clean up
    reply->deleteLater();

    qDebug() << "HttpClient: Final result - Success:" << requestSuccess
             << "Error:" << requestError
             << "Response length:" << response.length();

    if (success) *success = requestSuccess;
    if (error) *error = requestError;

    return response;
}

QList<QNetworkCookie> HttpClient::getLastResponseCookies() const
{
    return m_lastResponseCookies;
}
