#include <QApplication>
#include <QMessageBox>

#include "mainwindow.h"
#include "../config/SettingsDialog.h"
#include "../config/settings.h"

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	MainWindow w;

	return a.exec();
}
