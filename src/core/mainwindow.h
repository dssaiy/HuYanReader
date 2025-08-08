#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QLabel>
#include <QTextDocument>
#include <QSettings>
#include <QTextStream>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QCloseEvent>


#include "TextDocumentModel.h"
#include "../ui/TextReaderView.h"
#include "TextReaderManager.h"
#include "../config/SettingsDialog.h"
#include "QHotkey.h"
#include "../ui/chapterdialog.h"
#include "../ui/WebEngineView.h"
#include "../ui/NovelSearchViewEnhanced.h"
#include "../novel/NovelSearchManager.h"
#include "../config/NovelConfig.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();


	void showChapterDialog();

protected:

	
	void closeEvent(QCloseEvent* event) override;
	void changeEvent(QEvent* event) override;

private:
	
	void initTrayIcon();
	void initHotkey();
	
	void createTrayMenu();
	void test();

	
	QLabel* textLabel;
	Settings* settings;
	SettingsDialog* settingsDialog;
	TextDocumentManager* tdm;
	TextDocumentModel* model;
	TextReaderView* view;

	
	QSystemTrayIcon* trayIcon;
	QMenu* contextMenu;
	QAction* exitAction;
	QAction* m_openWebEngineAction;
	QAction* m_openNovelSearchAction;
	QHotkey* m_hotkeyQuit;
	QHotkey* m_hotkeyBoss;
	WebEngineView* m_webView;
	NovelSearchViewEnhanced* m_novelSearchViewEnhanced;
	NovelSearchManager* m_novelSearchManager;
	NovelConfig* m_novelConfig;
	bool m_windowsVisible;
	bool m_readerViewActive;
	bool m_webViewActive;
	bool m_novelSearchActive;

private slots:
	
	void onChapterSelectClicked();
	void onChapterSelected(int pageIndex);
	void openWebBrowser();
	void openNovelSearch();
	void toggleAllWindows();

};

#endif