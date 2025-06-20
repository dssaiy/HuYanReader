#pragma once

#include <QDialog>
#include <QColorDialog>
#include "Settings.h"

namespace Ui {
	class SettingsDialog;
}

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(Settings* settings, QWidget* parent = nullptr);
	~SettingsDialog();

private slots:
	void applySettings();
	void selectNovelPath();
	void onFontColorButtonClicked();  // 按钮点击槽函数
	void onBackgroundColorButtonClicked();  // 按钮点击槽函数

private:
	void loadSettings();

	Ui::SettingsDialog* ui;
	Settings* m_settings;
};