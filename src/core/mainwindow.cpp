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
	m_openNovelSearchAction(new QAction("Open Novel Search", this)),
	m_novelSearchViewEnhanced(new NovelSearchViewEnhanced()),
	m_novelSearchManager(new NovelSearchManager(settings, this)),
	m_novelConfig(new NovelConfig(this)),
	m_windowsVisible(true),
	m_readerViewActive(false),
	m_novelSearchActive(false)
{
	setAttribute(Qt::WA_TranslucentBackground);

	initTrayIcon();
	initHotkey();

	// Register meta types for Qt signal-slot system (same as test_dialog)
	qRegisterMetaType<QList<SearchResult>>("QList<SearchResult>");
	qRegisterMetaType<SearchResult>("SearchResult");

	tdm->linkViewAndModel(view, model);

	// Setup enhanced novel search view
	m_novelSearchViewEnhanced->setNovelConfig(m_novelConfig);

	// Link NovelConfig to search manager
	m_novelSearchManager->setNovelConfig(m_novelConfig);

	// Connect enhanced view signals to manager
	connect(m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::searchRequested,
	        m_novelSearchManager, &NovelSearchManager::startSearch);
	connect(m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::downloadRequested,
	        m_novelSearchManager, &NovelSearchManager::startDownload);

	// Connect manager signals to enhanced view
	bool connected1 = connect(m_novelSearchManager, &NovelSearchManager::searchStarted,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onSearchStarted);
	bool connected2 = connect(m_novelSearchManager, &NovelSearchManager::searchProgress,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onSearchProgress);
	bool connected3 = connect(m_novelSearchManager, &NovelSearchManager::searchCompleted,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onSearchCompleted);
	bool connected4 = connect(m_novelSearchManager, &NovelSearchManager::searchResultsUpdated,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onSearchResultsUpdated);
	bool connected5 = connect(m_novelSearchManager, &NovelSearchManager::searchFailed,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onSearchFailed);

	// Connect download-related signals
	bool connected6 = connect(m_novelSearchManager, &NovelSearchManager::downloadStarted,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onDownloadStarted);
	bool connected7 = connect(m_novelSearchManager, &NovelSearchManager::downloadProgress,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onDownloadProgress);
	bool connected8 = connect(m_novelSearchManager, &NovelSearchManager::downloadCompleted,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onDownloadCompleted);
	bool connected9 = connect(m_novelSearchManager, &NovelSearchManager::downloadFailed,
	        m_novelSearchViewEnhanced, &NovelSearchViewEnhanced::onDownloadFailed);

	qDebug() << "Signal connections established:";
	qDebug() << "  searchStarted:" << connected1;
	qDebug() << "  searchProgress:" << connected2;
	qDebug() << "  searchCompleted:" << connected3;
	qDebug() << "  searchResultsUpdated:" << connected4;
	qDebug() << "  searchFailed:" << connected5;
	qDebug() << "  downloadStarted:" << connected6;
	qDebug() << "  downloadProgress:" << connected7;
	qDebug() << "  downloadCompleted:" << connected8;
	qDebug() << "  downloadFailed:" << connected9;

	connect(settings, &Settings::settingsChanged, tdm, &TextDocumentManager::applySettings);
}

MainWindow::~MainWindow() {
	if (view) {
		view->close();
		delete view;
		view = nullptr;
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
	trayIcon->setIcon(QIcon(":/MainWindow/icon.ico")); // Use resource path
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

	connect(m_openNovelSearchAction, &QAction::triggered, this, &MainWindow::openNovelSearch);
	contextMenu->addAction(m_openNovelSearchAction);

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

void MainWindow::openNovelSearch()
{
	m_novelSearchActive = true;
	// Show the enhanced novel search view
	if (m_novelSearchViewEnhanced) {
		m_novelSearchViewEnhanced->show();
		m_novelSearchViewEnhanced->raise();
		m_novelSearchViewEnhanced->activateWindow();
	}
}

void MainWindow::toggleAllWindows()
{
	m_windowsVisible = !m_windowsVisible;

	if (m_readerViewActive) {
		view->setVisible(m_windowsVisible);
	}
	
	if (m_novelSearchActive) {
		m_novelSearchViewEnhanced->setVisible(m_windowsVisible);
	}
}