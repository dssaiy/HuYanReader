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
#include "TextReaderView.h"
#include "TextReaderManager.h"
#include "SettingsDialog.h"
#include "QHotkey.h" 
#include "chapterdialog.h"
#include "WebEngineView.h"

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
	QHotkey* m_hotkeyQuit;
	QHotkey* m_hotkeyBoss;
	WebEngineView* m_webView;
	bool m_windowsVisible;
	bool m_readerViewActive;
	bool m_webViewActive;

private slots:
	
	void onChapterSelectClicked();
	void onChapterSelected(int pageIndex);
	void openWebBrowser();
	void toggleAllWindows();

};

#endif