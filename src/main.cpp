#include <QApplication>
#include <QMessageBox>
#include <QWebEngineView>

#include "mainwindow.h"
#include "SettingsDialog.h"
#include "Settings.h"

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	MainWindow w;

	return a.exec();
}
