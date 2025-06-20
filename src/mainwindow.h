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
	
	void initUI();
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
	QAction* restoreAction;
	QAction* exitAction;
	QHotkey* m_hotkeyMainwindow;

private slots:
	
	void restoreWindow();
	void showContextMenu(const QPoint& pos);

	void onChapterSelectClicked();
	void onChapterSelected(int pageIndex);

};

#endif