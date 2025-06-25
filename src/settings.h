#pragma once
#include <QObject>
#include <QString>
#include <QColor>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QMessageBox>

class Settings : public QObject {
	Q_OBJECT

public:
	explicit Settings(QObject* parent = nullptr);

	~Settings();
	

	void setFontSize(float size); 
	void setTextSpacing(int spacing); 
	void setLineSpacing(int spacing); 
	void setNovelPath(const QString& path); 
	void setEncoding(const QString& encoding); 
	void setMenuEncoding(const QString& encoding); 
	void setLinesPerPage(int lines); 
	void setOpacity(double opa);
	void setFontColor(const QString& color);
	void setBackgroundColor(const QString& color);
	void setStartInTray(bool enabled);


	float getFontSize() const;
	int getTextSpacing() const;
	int getLineSpacing() const;
	QString getNovelPath() const;
	QString getEncoding() const;
	QString getMenuEncoding() const;
	int getLinesPerPage() const;
	double getOpacity() const;
	QString getFontColor() const;
	QString getBackgroundColor() const;
	bool getStartInTray() const;

	QSettings* getpSettings() { return m_settings; }

	
	void saveSettings(); 
	void loadSettings(); 
	bool hasSettings() const; 

signals:
	void settingsChanged();

private:

	
	void setDefaults();

	float m_fontSize;           
	int m_textSpacing;        
	int m_lineSpacing;        
	QString m_novelPath;      
	QString m_encoding;       
	QString m_menuEncoding;       
	int m_linesPerPage;       

	QSettings* m_settings;    
	QString m_settingsFilePath; 

	double m_opacity;          

	QString m_fontColor;
	QString m_backgroundColor;
	bool m_startInTray;
};
