#include <QTextStream>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTextLayout>
#include <QApplication>
#include <QTextBlock>
#include <algorithm>

#include "mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	m_hotkeyQuit(nullptr),
	m_hotkeyBoss(nullptr),
	settings(new Settings()),
	settingsDialog(new SettingsDialog(settings)),
	tdm(new TextDocumentManager(settings)),
	view(new TextReaderView()),
	model(new TextDocumentModel()),
	trayIcon(new QSystemTrayIcon(this)),
	contextMenu(new QMenu(this)),
	exitAction(new QAction("Exit", this)),
	m_openWebEngineAction(new QAction("Open Web Browser", this)),
	m_webView(new WebEngineView()),
	m_windowsVisible(true),
	m_readerViewActive(false),
	m_webViewActive(false)
{
	setAttribute(Qt::WA_TranslucentBackground);

	initTrayIcon();
	initHotkey();

	tdm->linkViewAndModel(view, model);

	connect(settings, &Settings::settingsChanged, tdm, &TextDocumentManager::applySettings);
}

MainWindow::~MainWindow() {
	if (view) {
		view->close();
		delete view;
		view = nullptr;
	}

	if (m_webView) {
		m_webView->close();
		delete m_webView;
		m_webView = nullptr;
	}

	if (tdm) {
		delete tdm;
		tdm = nullptr;
	}

	if (model) {
		delete model;
		model = nullptr;
	}

	if (trayIcon->isVisible()) {
		trayIcon->hide();
	}

	if (m_hotkeyQuit) {
		delete m_hotkeyQuit;
		m_hotkeyQuit = nullptr;
	}
	if (m_hotkeyBoss) {
		delete m_hotkeyBoss;
		m_hotkeyBoss = nullptr;
	}

	if (settings) {
		delete settings;
		settings = nullptr;
	}
}

void MainWindow::initHotkey() {
	// Quit Application Hotkey
	m_hotkeyQuit = new QHotkey(QKeySequence("Ctrl+Alt+Q"), true, this);
	if (!m_hotkeyQuit->isRegistered()) {
		QMessageBox::warning(nullptr, "Warning", "Quit hotkey (Ctrl+Alt+Q) registration failed!");
	}
	connect(m_hotkeyQuit, &QHotkey::activated, qApp, &QApplication::quit);

	// Boss Key Hotkey
	m_hotkeyBoss = new QHotkey(QKeySequence("Ctrl+Alt+M"), true, this);
	if (!m_hotkeyBoss->isRegistered()) {
		QMessageBox::warning(nullptr, "Warning", "Boss key (Ctrl+Alt+M) registration failed!");
	}
	connect(m_hotkeyBoss, &QHotkey::activated, this, &MainWindow::toggleAllWindows);

}

void MainWindow::closeEvent(QCloseEvent* event) {
	// Since there is no main window, this might not be triggered.
	// We can quit the app from the tray menu.
	event->ignore();
}


void MainWindow::changeEvent(QEvent* event) {
	// This is not needed anymore as the window is not visible.
	QMainWindow::changeEvent(event);
}

void MainWindow::initTrayIcon() {
	trayIcon->setToolTip("Huyan Reader");
	trayIcon->setIcon(QIcon(":/images/icon.ico")); // Use resource path
	createTrayMenu();
	trayIcon->show();
}

//void MainWindow::initUI() {
//	// This is now empty as we have no main UI.
//}

void MainWindow::test() {
	m_readerViewActive = true;
	tdm->tableView()->show();
}

void MainWindow::createTrayMenu() {
	QAction* settingsAction = contextMenu->addAction("Settings");
	connect(settingsAction, &QAction::triggered, this, [this]() {
		settingsDialog->exec();
	});

	QAction* menuAction = contextMenu->addAction("Menu");
	connect(menuAction, &QAction::triggered, this, &MainWindow::onChapterSelectClicked);

	QAction* startAction = contextMenu->addAction("Start Reading");
	connect(startAction, &QAction::triggered, this, &MainWindow::test);

	contextMenu->addSeparator();

	connect(m_openWebEngineAction, &QAction::triggered, this, &MainWindow::openWebBrowser);
	contextMenu->addAction(m_openWebEngineAction);

	contextMenu->addSeparator();

	connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	contextMenu->addAction(exitAction);

	trayIcon->setContextMenu(contextMenu);
}

void MainWindow::onChapterSelectClicked()
{
	QMap<int, QString> charIndexMap = tdm->tableModel()->menuIndexMap();

	ChapterDialog dialog(charIndexMap, this);
	connect(&dialog, &ChapterDialog::chapterSelected, this, &MainWindow::onChapterSelected);

	dialog.exec();
}

void MainWindow::onChapterSelected(int pageIndex)
{
	tdm->tableModel()->setCurrentPage(pageIndex);
}

void MainWindow::openWebBrowser()
{
	m_webViewActive = true;
	m_webView->show();
}

void MainWindow::toggleAllWindows()
{
	m_windowsVisible = !m_windowsVisible;

	if (m_readerViewActive) {
		view->setVisible(m_windowsVisible);
	}
	
	if (m_webViewActive) {
		m_webView->setVisible(m_windowsVisible);
	}
}