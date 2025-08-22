#include <QTextStream>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTextLayout>
#include <QApplication>
#include <QTextBlock>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>
#include <QStatusBar>
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
	// Close button minimizes to taskbar instead of quitting
	qDebug() << "Close button clicked - minimizing to taskbar";
	this->showMinimized();
	event->ignore();
}


void MainWindow::changeEvent(QEvent* event) {
	// This is not needed anymore as the window is not visible.
	QMainWindow::changeEvent(event);
}

void MainWindow::initTrayIcon() {
	// Use window mode instead of system tray
	this->setWindowTitle("HuYan Reader v1.0");
	this->setWindowIcon(QIcon(":/MainWindow/icon.ico"));
	
	// Set window size and position
	this->resize(600, 450);
	this->move(100, 100);
	
	// Create central widget
	QWidget* centralWidget = new QWidget(this);
	this->setCentralWidget(centralWidget);
	
	QVBoxLayout* layout = new QVBoxLayout(centralWidget);
	
	// Add welcome label
	QLabel* welcomeLabel = new QLabel("Welcome to HuYan Reader", this);
	welcomeLabel->setAlignment(Qt::AlignCenter);
	welcomeLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin: 20px;");
	layout->addWidget(welcomeLabel);
	
	// Add function buttons
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	
	QPushButton* readingButton = new QPushButton("Start Reading", this);
	readingButton->setMinimumHeight(40);
	readingButton->setStyleSheet("QPushButton { background-color: #27ae60; color: white; border: none; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #229954; }");
	connect(readingButton, &QPushButton::clicked, [this]() {
		// Open text reader view
		if (view) {
			m_readerViewActive = true;
			view->show();
			view->raise();
			view->activateWindow();
		}
	});
	
	QPushButton* contentsButton = new QPushButton("Show Contents", this);
	contentsButton->setMinimumHeight(40);
	contentsButton->setStyleSheet("QPushButton { background-color: #3498db; color: white; border: none; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #2980b9; }");
	connect(contentsButton, &QPushButton::clicked, this, &MainWindow::onChapterSelectClicked);
	
	QPushButton* settingsButton = new QPushButton("Settings", this);
	settingsButton->setMinimumHeight(40);
	settingsButton->setStyleSheet("QPushButton { background-color: #95a5a6; color: white; border: none; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #7f8c8d; }");
	connect(settingsButton, &QPushButton::clicked, [this]() {
		settingsDialog->show();
	});
	
	buttonLayout->addWidget(readingButton);
	buttonLayout->addWidget(contentsButton);
	buttonLayout->addWidget(settingsButton);
	layout->addLayout(buttonLayout);
	
	// Add info text
	QLabel* infoLabel = new QLabel(
		"<h3>Features:</h3>"
		"<ul>"
		"<li><b>Text Reading</b>: Read TXT files with advanced features</li>"
		"<li><b>Eye Protection</b>: Reduce eye strain with special modes</li>"
		"<li><b>Global Hotkeys</b>: Control reading with keyboard shortcuts</li>"
		"<li><b>Novel Download</b>: Search and download novels from web</li>"
		"</ul>"
		"<p style='margin-top: 20px;'><b>Tip:</b> Click 'Start Reading' to begin or use hotkeys for quick access</p>", 
		this);
	infoLabel->setWordWrap(true);
	infoLabel->setStyleSheet("color: #34495e; margin: 20px; line-height: 1.5;");
	layout->addWidget(infoLabel);
	
	layout->addStretch(); // Add flexible space
	
	// Create menu bar
	QMenuBar* menuBar = this->menuBar();
	QMenu* fileMenu = menuBar->addMenu("&File");
	QMenu* viewMenu = menuBar->addMenu("&View");
	QMenu* toolsMenu = menuBar->addMenu("&Tools");
	QMenu* helpMenu = menuBar->addMenu("&Help");
	
	// Add menu items
	QAction* openFileAction = fileMenu->addAction("&Open Text File...");
	connect(openFileAction, &QAction::triggered, [this]() {
		if (view) {
			m_readerViewActive = true;
			view->show();
			view->raise();
			view->activateWindow();
		}
	});
	
	fileMenu->addSeparator();
	QAction* exitAction = fileMenu->addAction("E&xit");
	connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	
	QAction* startReadingAction = viewMenu->addAction("&Start Reading");
	connect(startReadingAction, &QAction::triggered, [this]() {
		if (view) {
			m_readerViewActive = true;
			view->show();
			view->raise();
			view->activateWindow();
		}
	});
	
	QAction* novelSearchAction = toolsMenu->addAction("&Novel Search");
	connect(novelSearchAction, &QAction::triggered, this, &MainWindow::openNovelSearch);
	
	QAction* settingsAction = toolsMenu->addAction("&Settings");
	connect(settingsAction, &QAction::triggered, [this]() {
		settingsDialog->show();
	});
	
	QAction* aboutAction = helpMenu->addAction("&About");
	connect(aboutAction, &QAction::triggered, [this]() {
		QMessageBox::about(this, "About HuYan Reader", 
			"HuYan Reader v1.0\n\n"
			"Advanced TXT file reader with eye protection features\n"
			"Support novel downloading and global hotkey control\n\n"
			"Main Features:\n"
			"• TXT file reading with multiple view modes\n"
			"• Eye protection settings and themes\n"
			"• Global hotkey control for hands-free operation\n"
			"• Novel search and download capabilities\n\n"
			"Lightweight version without WebEngine component");
	});
	
	// Create status bar
	this->statusBar()->showMessage("HuYan Reader ready - Click 'Start Reading' to open a text file");
	
	// Show main window
	this->show();
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
	qDebug() << "toggleAllWindows() called - Boss Key (Ctrl+Alt+M)";
	qDebug() << "m_windowsVisible:" << m_windowsVisible;
	qDebug() << "m_readerViewActive:" << m_readerViewActive;
	
	m_windowsVisible = !m_windowsVisible;

	// Boss key only controls TextReaderView, not main window
	if (m_readerViewActive && view) {
		qDebug() << "Setting TextReaderView visible to:" << m_windowsVisible;
		view->setVisible(m_windowsVisible);
	}
	
	// Main window and NovelSearch are NOT controlled by boss key
}