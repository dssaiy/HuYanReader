#include "Settings.h"

Settings::Settings(QObject* parent) : QObject(parent)
{
	QString exeDir = QCoreApplication::applicationDirPath();
	
	m_settingsFilePath = exeDir + "/settings.ini";
	
	m_settings = new QSettings(m_settingsFilePath, QSettings::IniFormat, this);

	if (!hasSettings()) {

		
		setDefaults();
	}
	else if (hasSettings()) {	
		loadSettings();
	}

	saveSettings();
}

Settings::~Settings()
{
	
	saveSettings();
}

bool Settings::hasSettings() const
{
	
	QFileInfo checkFile(m_settingsFilePath);
	return checkFile.exists() && checkFile.isFile();

}

void Settings::setDefaults()
{
	
	m_fontSize = 12;
	m_textSpacing = 1;
	m_lineSpacing = 1;
	m_novelPath = "";
	m_encoding = "UTF-8";
	m_menuEncoding = "UTF-8";
	m_linesPerPage = 20;
	m_opacity = 0.85;
	m_fontColor = "#000000";  
	m_backgroundColor = "#FFFFFF";  
}

void Settings::loadSettings()
{
	m_settings->beginGroup("Settings");
	
	m_fontSize = m_settings->value("fontSize", 12).toFloat();
	m_textSpacing = m_settings->value("textSpacing", 1).toInt();
	m_lineSpacing = m_settings->value("lineSpacing", 1).toInt();
	m_novelPath = m_settings->value("novelPath", "").toString();

	if (!m_novelPath.isEmpty() && !QFileInfo(m_novelPath).exists()) {
		QMessageBox::warning(nullptr, "路径无效", "当前路径有误,请在setting中设置有效的文本路径");
		m_novelPath = "";
		m_settings->setValue("novelPath", "");
		m_settings->sync();
	}

	m_encoding = m_settings->value("encoding", "UTF-8").toString();
	m_menuEncoding = m_settings->value("menuEncoding", "UTF-8").toString();
	m_linesPerPage = m_settings->value("linesPerPage", 20).toInt();
	m_opacity = m_settings->value("opacity", 0.85).toDouble();

	
	m_fontColor = m_settings->value("fontColor", "#000000").toString();
	m_backgroundColor = m_settings->value("backgroundColor", "#FFFFFF").toString();

	m_settings->endGroup();
}

void Settings::saveSettings()
{
	m_settings->beginGroup("Settings");
	
	m_settings->setValue("fontSize", m_fontSize);
	m_settings->setValue("textSpacing", m_textSpacing);
	m_settings->setValue("lineSpacing", m_lineSpacing);
	m_settings->setValue("novelPath", m_novelPath);
	m_settings->setValue("encoding", m_encoding);
	m_settings->setValue("menuEncoding", m_menuEncoding);
	m_settings->setValue("linesPerPage", m_linesPerPage);
	m_settings->setValue("opacity", m_opacity);

	
	m_settings->setValue("fontColor", m_fontColor);
	m_settings->setValue("backgroundColor", m_backgroundColor);

	
	m_settings->sync();
	m_settings->endGroup();
}


void Settings::setFontSize(float size) {
	if (m_fontSize != size) {
		m_fontSize = size;
		
	}
}

void Settings::setTextSpacing(int spacing) {
	if (m_textSpacing != spacing) {
		m_textSpacing = spacing;
		
	}
}

void Settings::setLineSpacing(int spacing) {
	if (m_lineSpacing != spacing) {
		m_lineSpacing = spacing;
		
	}
}

void Settings::setNovelPath(const QString& path) {
	if (m_novelPath != path) {
		m_novelPath = path;
		
	}
}

void Settings::setEncoding(const QString& encoding) {
	if (m_encoding != encoding) {
		m_encoding = encoding;
		
	}
}

void Settings::setMenuEncoding(const QString& encoding) {
	if (m_menuEncoding != encoding) {
		m_menuEncoding = encoding;
		
	}
}

void Settings::setLinesPerPage(int lines) {
	if (m_linesPerPage != lines) {
		m_linesPerPage = lines;
		
	}
}

void Settings::setOpacity(double opa) {
	if (m_opacity != opa) {
		m_opacity = opa;
		emit settingsChanged();
	}
}


void Settings::setFontColor(const QString& color) {
	if (m_fontColor != color) {
		m_fontColor = color;
	}
}

void Settings::setBackgroundColor(const QString& color) {
	if (m_backgroundColor != color) {
		m_backgroundColor = color;
	}
}


float Settings::getFontSize() const {
	return m_fontSize;
}

int Settings::getTextSpacing() const {
	return m_textSpacing;
}

int Settings::getLineSpacing() const {
	return m_lineSpacing;
}

QString Settings::getNovelPath() const {
	return m_novelPath;
}

QString Settings::getEncoding() const {
	return m_encoding;
}

QString Settings::getMenuEncoding() const {
	return m_menuEncoding;
}

int Settings::getLinesPerPage() const {
	return m_linesPerPage;
}

double Settings::getOpacity() const {
	return m_opacity;
}

QString Settings::getFontColor() const {
	return m_fontColor;
}

QString Settings::getBackgroundColor() const {
	return m_backgroundColor;
}