#include <QApplication>
#include <QMessageBox>
#include <QWebEngineView>

#include "mainwindow.h"
#include "SettingsDialog.h"
#include "Settings.h"

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);  // QApplication 应该在创建任何 GUI 组件之前实例化

	//QWebEngineView view;
	//view.setUrl(QUrl("https://www.baidu.com"));
	//view.show();

	// 在对话框关闭后创建并显示主窗口
	MainWindow w;
	w.show();

	// 启动事件循环
	return a.exec();
}
