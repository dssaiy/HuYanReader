#include "WebEngineView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QFrame>
#include <QSizeGrip>

WebEngineView::WebEngineView(QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

	m_webView = new QWebEngineView(this);

	// Create an off-the-record profile for privacy mode
	auto offTheRecordProfile = new QWebEngineProfile(this);
	// Set a mobile User-Agent to improve compatibility
	offTheRecordProfile->setHttpUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
	auto webPage = new QWebEnginePage(offTheRecordProfile, this);
	m_webView->setPage(webPage);

	m_backButton = new QPushButton("<", this);
	m_backButton->setMinimumWidth(1);
	m_forwardButton = new QPushButton(">", this);
	m_forwardButton->setMinimumWidth(1);
	m_refreshButton = new QPushButton("R", this);
	m_refreshButton->setMinimumWidth(1);
	m_dragBar = new QFrame(this);
	m_dragBar->setFixedHeight(5);

	// dragBar color
	//m_dragBar->setStyleSheet("background-color: transparent;");
	m_dragBar->setStyleSheet("background-color: rgba(127, 127, 127, 127);");

	m_dragBar->setCursor(Qt::SizeAllCursor);
	m_addressBar = new QLineEdit(this);
	m_addressBar->setMinimumWidth(1);
	m_goButton = new QPushButton("Go", this);
	m_goButton->setMinimumWidth(1);
	m_sizeGrip = new QSizeGrip(this);

	QHBoxLayout* addressLayout = new QHBoxLayout;
	addressLayout->addWidget(m_backButton);
	addressLayout->addWidget(m_forwardButton);
	addressLayout->addWidget(m_refreshButton);
	addressLayout->addWidget(m_addressBar);
	addressLayout->addWidget(m_goButton);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(2, 0, 2, 2);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(m_dragBar);
	mainLayout->addSpacing(4);  // 在 dragBar 和下一个控件之间添加 4px 空白
	mainLayout->addLayout(addressLayout);
	mainLayout->addWidget(m_webView, 1);
	mainLayout->addWidget(m_sizeGrip, 0, Qt::AlignBottom | Qt::AlignRight);
	setLayout(mainLayout);

	resize(1024, 768);

	connect(m_backButton, &QPushButton::clicked, m_webView, &QWebEngineView::back);
	connect(m_forwardButton, &QPushButton::clicked, m_webView, &QWebEngineView::forward);
	connect(m_refreshButton, &QPushButton::clicked, m_webView, &QWebEngineView::reload);
	connect(m_goButton, &QPushButton::clicked, this, &WebEngineView::loadUrl);
	connect(m_addressBar, &QLineEdit::returnPressed, this, &WebEngineView::loadUrl);
	connect(m_webView, &QWebEngineView::urlChanged, this, &WebEngineView::updateAddressBar);
	connect(m_webView, &QWebEngineView::loadFinished, this, &WebEngineView::onPageLoadFinished);  // Connect loadFinished

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

void WebEngineView::updateAddressBar(const QUrl& url)
{
	m_addressBar->setText(url.toString());
}

void WebEngineView::setOpacity(double opacity)
{
	setWindowOpacity(opacity);
}

void WebEngineView::closeEvent(QCloseEvent* event)
{
	this->hide();
	event->ignore();
}

void WebEngineView::keyPressEvent(QKeyEvent* event)
{
	const float opacityStep = 0.01;

	if ((event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) ==
		(Qt::ControlModifier | Qt::AltModifier))
	{
		if (event->key() == Qt::Key_Plus) {
			double currentOpacity = windowOpacity();
			currentOpacity += opacityStep;
			if (currentOpacity > 1.0) currentOpacity = 1.0;
			setWindowOpacity(currentOpacity);
			event->accept();
		}
		else if (event->key() == Qt::Key_Minus) {
			double currentOpacity = windowOpacity();
			currentOpacity -= opacityStep;
			if (currentOpacity < 0.01) currentOpacity = 0.01;
			setWindowOpacity(currentOpacity);
			event->accept();
		}
		else {
			QWidget::keyPressEvent(event);
		}
	}
	else {
		QWidget::keyPressEvent(event);
	}
}

void WebEngineView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragBar->underMouse()) {
        m_isDragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void WebEngineView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void WebEngineView::mouseReleaseEvent(QMouseEvent *event)
{
	   m_isDragging = false;
	   event->accept();
}

void WebEngineView::onPageLoadFinished(bool ok)
{
	if (ok) {
		m_webView->page()->runJavaScript(R"(
            // α�� navigator.userAgentData
            if (navigator.userAgentData) {
                Object.defineProperty(navigator, 'userAgentData', {
                    get: () => ({
                        brands: [
                            { brand: "Google Chrome", version: "126" },
                            { brand: "Chromium", version: "126" }
                        ],
                        mobile: false
                    })
                });
            }

            // ģ�� chrome.runtime ����
            if (!window.chrome) window.chrome = {};
            if (!chrome.runtime) chrome.runtime = {};
        )");
	}
}
