#include "TextReaderManager.h"


TextDocumentManager::TextDocumentManager(Settings* settings, QObject* parent)
	: QObject(parent),
	m_Model(nullptr),
	m_View(nullptr),
	m_Settings(settings)  // 传入已有的 Settings
	//m_currentPage(0)
{
	if (m_Settings)
	{
		m_Settings->loadSettings();
	}
	m_Settings->getpSettings()->beginGroup("User");
	// 从配置文件加载设置
	m_currentPage = m_Settings->getpSettings()->value("currentPage", 0).toInt();
	m_Settings->getpSettings()->endGroup();

	connect(m_Settings, &Settings::settingsChanged, this, &TextDocumentManager::applySettings);

}

TextDocumentManager::~TextDocumentManager()
{
	m_Settings->getpSettings()->beginGroup("User");
	// 保存设置到配置文件
	QString feet = QString::number(m_Model->getCurrentPage());
	m_Settings->getpSettings()->setValue("currentPage", feet);
	// 立即写入磁盘
	m_Settings->getpSettings()->sync();
	m_Settings->getpSettings()->endGroup();
}

void TextDocumentManager::openFile()
{
	if (!m_Model) return;

	QString filepath = m_Settings->getNovelPath();

	// 加载文件
	if (m_Model->loadFile(filepath)) {  // 确保 `loadFile()` 返回 `bool`
		m_View->setTotalPages(m_Model->getTotalPages());
		//m_currentPage = 0;  // 打开文件后，从第一页开始
		//m_View->showPage(m_Model->getPageContent(m_currentPage), m_currentPage);
		//m_View->show();
	}



}

void TextDocumentManager::nextPage()
{
	if (m_Model->getCurrentPage() < m_Model->getTotalPages() - 1) {

		m_Model->setCurrentPage(m_Model->getCurrentPage()+1);
		m_View->showPage(m_Model->getPageContent(m_Model->getCurrentPage()), m_Model->getCurrentPage());
	}
}

void TextDocumentManager::prevPage()
{
	if (m_Model->getCurrentPage() > 0) {  // 允许翻到第 0 页
		m_Model->setCurrentPage(m_Model->getCurrentPage()-1);
		m_View->showPage(m_Model->getPageContent(m_Model->getCurrentPage()), m_Model->getCurrentPage());
	}
}

void TextDocumentManager::applySettings()
{
	if (!m_Settings || !m_View || !m_Model)
		return;
	
	// 应用到 Model
	//m_Model->setCharactersPerPage(m_Settings->getLinesPerPage());
	m_Model->setCurrentPage(m_currentPage);
	m_Model->setMenuEncoding(m_Settings->getMenuEncoding());
	m_Model->setEncoding(m_Settings->getEncoding());
	m_Model->setLinesPerPage(m_Settings->getLinesPerPage());
	m_Model->reloadFile(m_Settings->getNovelPath());  // 重新加载文本

	// 应用到 View
	m_View->setFontAndBackgroundColor(m_Settings->getFontColor(), m_Settings->getBackgroundColor());

	m_View->setWindowOpacity(m_Settings->getOpacity());
	m_View->setFontSize(m_Settings->getFontSize());
	m_View->setTextSpacing(m_Settings->getTextSpacing());
	m_View->setLineSpacing(m_Settings->getLineSpacing());
	m_View->setTotalPages(m_Model->getTotalPages());

	m_View->showPage(m_Model->getPageContent(m_Model->getCurrentPage()), m_Model->getCurrentPage());
}

void TextDocumentManager::linkViewAndModel(TextReaderView* pTableView, TextDocumentModel* pTableModel)
{
	if (!pTableModel || !pTableView)
		return;

	// 先断开旧的连接，防止重复绑定
	if (m_View) {
		disconnect(m_View, &TextReaderView::nextPageRequested, this, &TextDocumentManager::nextPage);
		disconnect(m_View, &TextReaderView::previousPageRequested, this, &TextDocumentManager::prevPage);
	}

	m_Model = pTableModel;
	m_View = pTableView;

	applySettings();

	// 监听 View 的键盘事件
	m_View->installEventFilter(this);

	connect(m_Model, &TextDocumentModel::pageChanged, this, &TextDocumentManager::updateText);
	connect(m_View, &TextReaderView::nextPageRequested, this, &TextDocumentManager::nextPage);
	connect(m_View, &TextReaderView::previousPageRequested, this, &TextDocumentManager::prevPage);
}

void TextDocumentManager::updateText(int page)
{
	m_currentPage = page;
	if (m_View) {
		m_View->showPage(m_Model->getPageContent(page), page);
	}
	m_View->update();
}

bool TextDocumentManager::eventFilter(QObject* obj, QEvent* event) {
	if (obj == m_View && event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Plus) {
			m_Settings->setOpacity(qMin(m_Settings->getOpacity() + 0.05, 1.0));  // 增加透明度qMin(windowOpacity() + opacityStep, 1.0)
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Minus) {
			m_Settings->setOpacity(qMax(m_Settings->getOpacity() - 0.05, 0.05)); // 降低透明度
			return true;
		}
	}
	return QObject::eventFilter(obj, event);
}