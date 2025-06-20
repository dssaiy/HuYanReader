#include <QMenuBar>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QRandomGenerator> 
#include <QTextLayout>
#include <QApplication>
#include <QTextBlock>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>

#include "mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	m_hotkeyMainwindow(nullptr),
	settings(new Settings()),  
	settingsDialog(new SettingsDialog(settings)),
	tdm(new TextDocumentManager(settings)),
	view(new TextReaderView()),
	model(new TextDocumentModel()),
	trayIcon(new QSystemTrayIcon(this)),
	contextMenu(new QMenu(this)),
	restoreAction(new QAction("Restore", this)),
	exitAction(new QAction("Exit", this))
{  

	
	setAttribute(Qt::WA_TranslucentBackground);

	initUI();

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

	if (m_hotkeyMainwindow) {
		delete m_hotkeyMainwindow;
		m_hotkeyMainwindow = nullptr;
	}

	if (settings) {
		delete settings;
		settings = nullptr;
	}
	
}

void MainWindow::initHotkey() {
	m_hotkeyMainwindow = new QHotkey(QKeySequence("Ctrl+ALT+Q"), true, this);
	if (!m_hotkeyMainwindow->isRegistered()) {
		QMessageBox::warning(nullptr, "Warning", "hot key registration failed!");
	}

	QObject::connect(m_hotkeyMainwindow, &QHotkey::activated, qApp, &QApplication::quit);
}

void MainWindow::closeEvent(QCloseEvent* event) {
	
	if (!trayIcon->isVisible())
	{
		trayIcon->show();
	}
	tdm->tableView()->hide();
	this->hide();
	event->ignore();  
	
	

}


void MainWindow::changeEvent(QEvent* event) {
	if (event->type() == QEvent::WindowStateChange) {
		if (isMinimized()) {
			
			hide();
			if (!trayIcon->isVisible())
			{
				trayIcon->show();
			}
		}
	}
	QMainWindow::changeEvent(event);
}

void MainWindow::initTrayIcon() {
	trayIcon->setToolTip("My Application");
	trayIcon->setIcon(QIcon("E:\\code\\cProject\\protectEYE\\images\\thief.png")); 
	
	createTrayMenu();
}

void MainWindow::initUI() {

	
	QPushButton* settingsButton = new QPushButton("Settings", this);
	settingsButton->setStyleSheet("QPushButton { padding: 5px; }");

	QPushButton* menuButton = new QPushButton("Menu", this);
	menuButton->setStyleSheet("QPushButton { padding: 5px; }");

	QPushButton* studyButton = new QPushButton("study", this);
	studyButton->setStyleSheet("QPushButton { padding: 5px; }");

	
	QVBoxLayout* layout = new QVBoxLayout();
	
	layout->addWidget(settingsButton);  
	layout->addWidget(menuButton);  
	layout->addWidget(studyButton);  

	QWidget* centralWidget = new QWidget(this);
	centralWidget->setLayout(layout);
	setCentralWidget(centralWidget);

	
	connect(settingsButton, &QPushButton::clicked, this, [this]() {
		settingsDialog->exec();
		});

	
	connect(studyButton, &QPushButton::clicked, this, &MainWindow::test);

	connect(menuButton, &QPushButton::clicked, this, &MainWindow::onChapterSelectClicked);
}


void MainWindow::test() {

	tdm->tableView()->show();
}

void MainWindow::createTrayMenu() {


	
	
	

	
	exitAction = contextMenu->addAction("Exit");
	connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

	
	trayIcon->setContextMenu(contextMenu);

	
	connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
		if (reason == QSystemTrayIcon::DoubleClick) { 
			restoreWindow();
		}
		});

	trayIcon->show();
}

void MainWindow::restoreWindow() {
	
	showNormal();
	activateWindow();  
	raise();  

}

void MainWindow::showContextMenu(const QPoint& pos) {
	
	contextMenu->exec(mapToGlobal(pos));
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