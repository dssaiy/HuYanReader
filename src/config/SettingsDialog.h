#pragma once

#include <QDialog>
#include <QColorDialog>
#include <QFontDatabase>
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
	void onFontColorButtonClicked();  // ��ť����ۺ���
	void onBackgroundColorButtonClicked();  // ��ť����ۺ���

private:
	void loadSettings();
	void populateFontComboBox();  // 填充字体下拉框

	Ui::SettingsDialog* ui;
	Settings* m_settings;
};