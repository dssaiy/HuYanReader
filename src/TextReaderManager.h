#pragma once
#include <QObject>
#include <QString>

#include "TextDocumentModel.h"
#include "TextReaderView.h"

class TextDocumentManager : public QObject
{
	Q_OBJECT

public:
	explicit TextDocumentManager(Settings* settings = nullptr,QObject* parent = nullptr);
	~TextDocumentManager();

	void openFile();
	void linkViewAndModel(TextReaderView* pTableView, TextDocumentModel* pTableModel);

	// 绑定 Settings
	void applySettings();  // 让 Manager 处理新设置
	Settings* getSettings() { return m_Settings; }


	TextReaderView* tableView() { return m_View; }
	TextDocumentModel* tableModel() { return m_Model; }

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
	void updateText(int page);

private:
	void nextPage();
	void prevPage();

private:
	TextDocumentModel* m_Model;
	TextReaderView* m_View;
	Settings* m_Settings;
	int m_currentPage;

};
