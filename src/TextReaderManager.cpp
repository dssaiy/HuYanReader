#include "TextReaderManager.h"


TextDocumentManager::TextDocumentManager(Settings* settings, QObject* parent)
	: QObject(parent),
	m_Model(nullptr),
	m_View(nullptr),
	m_Settings(settings)  // �������е� Settings
	//m_currentPage(0)
{
	if (m_Settings)
	{
		m_Settings->loadSettings();
	}
	m_Settings->getpSettings()->beginGroup("User");
	// �������ļ���������
	m_currentPage = m_Settings->getpSettings()->value("currentPage", 0).toInt();
	m_Settings->getpSettings()->endGroup();

	connect(m_Settings, &Settings::settingsChanged, this, &TextDocumentManager::applySettings);

}

TextDocumentManager::~TextDocumentManager()
{
	m_Settings->getpSettings()->beginGroup("User");
	// �������õ������ļ�
	QString feet = QString::number(m_Model->getCurrentPage());
	m_Settings->getpSettings()->setValue("currentPage", feet);
	// ����д�����
	m_Settings->getpSettings()->sync();
	m_Settings->getpSettings()->endGroup();
}

void TextDocumentManager::openFile()
{
	if (!m_Model) return;

	QString filepath = m_Settings->getNovelPath();

	// �����ļ�
	if (m_Model->loadFile(filepath)) {  // ȷ�� `loadFile()` ���� `bool`
		m_View->setTotalPages(m_Model->getTotalPages());
		//m_currentPage = 0;  // ���ļ��󣬴ӵ�һҳ��ʼ
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
	if (m_Model->getCurrentPage() > 0) {  // �������� 0 ҳ
		m_Model->setCurrentPage(m_Model->getCurrentPage()-1);
		m_View->showPage(m_Model->getPageContent(m_Model->getCurrentPage()), m_Model->getCurrentPage());
	}
}

void TextDocumentManager::applySettings()
{
	if (!m_Settings || !m_View || !m_Model)
		return;
	
	// Ӧ�õ� Model
	//m_Model->setCharactersPerPage(m_Settings->getLinesPerPage());
	m_Model->setCurrentPage(m_currentPage);
	m_Model->setMenuEncoding(m_Settings->getMenuEncoding());
	m_Model->setEncoding(m_Settings->getEncoding());
	m_Model->setLinesPerPage(m_Settings->getLinesPerPage());
	m_Model->reloadFile(m_Settings->getNovelPath());  // ���¼����ı�

	// Ӧ�õ� View
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

	// �ȶϿ��ɵ����ӣ���ֹ�ظ���
	if (m_View) {
		disconnect(m_View, &TextReaderView::nextPageRequested, this, &TextDocumentManager::nextPage);
		disconnect(m_View, &TextReaderView::previousPageRequested, this, &TextDocumentManager::prevPage);
	}

	m_Model = pTableModel;
	m_View = pTableView;

	applySettings();

	// ���� View �ļ����¼�
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
			m_Settings->setOpacity(qMin(m_Settings->getOpacity() + 0.05, 1.0));  // ����͸����qMin(windowOpacity() + opacityStep, 1.0)
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Minus) {
			m_Settings->setOpacity(qMax(m_Settings->getOpacity() - 0.05, 0.05)); // ����͸����
			return true;
		}
	}
	return QObject::eventFilter(obj, event);
}