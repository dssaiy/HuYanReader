#include "WebEngineView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QWebEngineProfile>
#include <QWebEnginePage>

WebEngineView::WebEngineView(QWidget *parent)
    : QWidget(parent)
{
    m_webView = new QWebEngineView(this);

    // Create an off-the-record profile for privacy mode
    auto offTheRecordProfile = new QWebEngineProfile(this);
    // Set a mobile User-Agent to improve compatibility
    // Final attempt with a very recent User-Agent
    offTheRecordProfile->setHttpUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
    auto webPage = new QWebEnginePage(offTheRecordProfile, this);
    m_webView->setPage(webPage);

    m_addressBar = new QLineEdit(this);
    m_goButton = new QPushButton("Go", this);

    QHBoxLayout *addressLayout = new QHBoxLayout;
    addressLayout->addWidget(m_addressBar);
    addressLayout->addWidget(m_goButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->addLayout(addressLayout);
    mainLayout->addWidget(m_webView);
    setLayout(mainLayout);

    setWindowTitle("Web Browser");
    resize(1024, 768);

    connect(m_goButton, &QPushButton::clicked, this, &WebEngineView::loadUrl);
    connect(m_addressBar, &QLineEdit::returnPressed, this, &WebEngineView::loadUrl);
    connect(m_webView, &QWebEngineView::urlChanged, this, &WebEngineView::updateAddressBar);

    m_webView->load(QUrl("https://www.bing.com"));
}

void WebEngineView::loadUrl()
{
    QString urlString = m_addressBar->text();
    if (!urlString.startsWith("http://") && !urlString.startsWith("https://")) {
        urlString.prepend("https://");
    }
    m_webView->load(QUrl(urlString));
}

void WebEngineView::updateAddressBar(const QUrl &url)
{
    m_addressBar->setText(url.toString());
}

void WebEngineView::setOpacity(double opacity)
{
    setWindowOpacity(opacity);
}

void WebEngineView::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}
void WebEngineView::keyPressEvent(QKeyEvent *event)
{
    const float opacityStep = 0.1;

	if ((event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) ==
		(Qt::ControlModifier | Qt::AltModifier))
	{
        if (event->key() == Qt::Key_Plus) {
            double currentOpacity = windowOpacity();
            currentOpacity += opacityStep;
            if (currentOpacity > 1.0) currentOpacity = 1.0;
            setWindowOpacity(currentOpacity);
            event->accept();
        } else if (event->key() == Qt::Key_Minus) {
            double currentOpacity = windowOpacity();
            currentOpacity -= opacityStep;
            if (currentOpacity < 0.1) currentOpacity = 0.1;
            setWindowOpacity(currentOpacity);
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}