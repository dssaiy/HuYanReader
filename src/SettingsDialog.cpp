#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QFileDialog>

SettingsDialog::SettingsDialog(Settings* settings, QWidget* parent)
	: QDialog(parent), m_settings(settings), ui(new Ui::SettingsDialog) {
	ui->setupUi(this);
	setWindowTitle("Settings");

	connect(ui->selectPathButton, &QPushButton::clicked, this, &SettingsDialog::selectNovelPath);
	connect(ui->applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
	connect(ui->cancelButton, &QPushButton::clicked, this, [this]() {
		loadSettings();  // 先调用 loadSettings
		reject();        // 然后关闭对话框
		});

	// 在构造函数中连接按钮点击事件：
	connect(ui->fontColorButton, &QPushButton::clicked, this, &SettingsDialog::onFontColorButtonClicked);
	connect(ui->backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::onBackgroundColorButtonClicked);

	loadSettings();
}

SettingsDialog::~SettingsDialog() {
	delete ui;
}

//menuEncodingLineEdit

void SettingsDialog::loadSettings() {
	ui->fontSizeSpinBox->setValue(m_settings->getFontSize());
	ui->textSpacingSpinBox->setValue(m_settings->getTextSpacing());
	ui->lineSpacingSpinBox->setValue(m_settings->getLineSpacing());
	ui->novelPathLineEdit->setText(m_settings->getNovelPath());
	ui->encodingLineEdit->setCurrentText(m_settings->getEncoding());
	ui->menuEncodingLineEdit->setCurrentText(m_settings->getMenuEncoding());
	ui->linesPerPageSpinBox->setValue(m_settings->getLinesPerPage());
	ui->opacitySpinBox->setValue(m_settings->getOpacity());

	// 加载字体颜色
	QString savedFontColor = m_settings->getFontColor();
	ui->fontColorLabel->setStyleSheet(QString("background-color: %1;").arg(savedFontColor));

	// 加载背景颜色
	QString savedBackgroundColor = m_settings->getBackgroundColor();
	ui->backgroundColorLabel->setStyleSheet(QString("background-color: %1;").arg(savedBackgroundColor));
}

void SettingsDialog::applySettings() {
	m_settings->setFontSize(ui->fontSizeSpinBox->value());
	m_settings->setTextSpacing(ui->textSpacingSpinBox->value());
	m_settings->setLineSpacing(ui->lineSpacingSpinBox->value());
	m_settings->setNovelPath(ui->novelPathLineEdit->text());
	m_settings->setEncoding(ui->encodingLineEdit->currentText());
	m_settings->setMenuEncoding(ui->menuEncodingLineEdit->currentText());
	m_settings->setLinesPerPage(ui->linesPerPageSpinBox->value());
	m_settings->setOpacity(ui->opacitySpinBox->value());

	// 保存字体颜色
	QString fontColor = ui->fontColorLabel->styleSheet();
	fontColor = fontColor.mid(fontColor.indexOf('#'));  // 获取十六进制颜色值
	m_settings->setFontColor(fontColor);

	// 保存背景颜色
	QString backgroundColor = ui->backgroundColorLabel->styleSheet();
	backgroundColor = backgroundColor.mid(backgroundColor.indexOf('#'));  // 获取十六进制颜色值
	m_settings->setBackgroundColor(backgroundColor);


	m_settings->saveSettings();

	emit m_settings->settingsChanged();
	accept();
}

void SettingsDialog::selectNovelPath() {
	QString path = QFileDialog::getOpenFileName(this, "Select Novel File", "", "Text Files (*.txt);;All Files (*)");
	if (!path.isEmpty()) {
		ui->novelPathLineEdit->setText(path);
	}
}

// 槽函数实现
void SettingsDialog::onFontColorButtonClicked()
{
	QColor color = QColorDialog::getColor(Qt::black, this, "选择字体颜色");

	// 如果用户选择了颜色，则更新显示当前颜色的标签背景
	if (color.isValid()) {
		ui->fontColorLabel->setStyleSheet(QString("background-color: %1;").arg(color.name()));

		// 可以在此处保存选中的颜色，供后续使用
	}
}

// 槽函数实现
void SettingsDialog::onBackgroundColorButtonClicked()
{
	QColor color = QColorDialog::getColor(Qt::black, this, "选择字体颜色");

	// 如果用户选择了颜色，则更新显示当前颜色的标签背景
	if (color.isValid()) {
		ui->backgroundColorLabel->setStyleSheet(QString("background-color: %1;").arg(color.name()));

		// 可以在此处保存选中的颜色，供后续使用
	}
}