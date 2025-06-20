#include "TextDocumentModel.h"
//#include <regex> // 使用 C++ 标准库的正则表达式
#include <QTextCodec> 
#include <QDebug> 

#include <QProgressDialog>
//#include <QInputDialog>

TextDocumentModel::TextDocumentModel(QObject* parent)
	: QObject(parent),
	m_useCache(false),
	m_currentPage(0),
	m_totalPage(0),
	m_numPerPage(50),  // 设置默认每页 1000 字
	m_file(nullptr),
	m_filePath(""),
	m_text(""),
	m_encoding("UTF-8"),
	m_menuEncoding("UTF-8")
{
}

TextDocumentModel::~TextDocumentModel() {
	if (m_file) {  // 应该先检查 m_file 是否为 nullptr
		if (m_file->isOpen()) {
			m_file->close();
		}
		delete m_file;
	}
}

void TextDocumentModel::setMenuEncoding(const QString& encoding)
{
	m_menuEncoding = encoding;

}
void TextDocumentModel::setEncoding(const QString& encoding)
{
	m_encoding = encoding;
}

void TextDocumentModel::setLinesPerPage(int lines)
{
	if (lines > 0 && lines != m_numPerPage) {
		m_numPerPage = lines;
		//setTotalPages();
		//initIndexMap();
		//m_charIndexMap.clear();
	}

}



void TextDocumentModel::initIndexMap()
{
	m_menuIndexMap.clear();
	// 创建进度对话框
	QProgressDialog progressDialog(u8"正在初始化章节索引...", u8"取消", 0, getTotalPages(), nullptr);
	progressDialog.setWindowModality(Qt::WindowModal);
	progressDialog.setMinimumDuration(500); // 避免闪烁
	progressDialog.setValue(0);

	// 动态创建 QRegularExpression
	QRegularExpression regex(QString::fromUtf8(u8"(第(\\d+|[一二三四五六七八九十百千万]+)章\\s*(.{1,10}))"));

	int totalPageNum = getTotalPages();
	for (int page = 0; page <= totalPageNum; page++) {
		// 如果用户点击了“取消”，提前退出
		if (progressDialog.wasCanceled()) {
			break;
		}

		QString pageContent = getPageContent(page);
		QRegularExpressionMatch match = regex.match(pageContent);
		if (match.hasMatch()) {
			QString chapterTitle = match.captured(0);
			m_menuIndexMap[page] = chapterTitle;
		}

		// 更新进度
		progressDialog.setValue(page);
	}

	progressDialog.setValue(totalPageNum); // 确保进度条到达最大值
}




void TextDocumentModel::reloadFile(const QString& filePath)
{
	if (filePath.isEmpty())
		return;
	if (filePath == m_filePath)
		return;
	// 重新加载文件
	// m_currentPage = 0;
	loadFile(filePath);

	// 更新总页数
	setTotalPages();

	// 更新目录
	int userCurrentPage = m_currentPage;
	initIndexMap();
	m_currentPage = userCurrentPage;
	updatePageCache(m_currentPage);

}

bool TextDocumentModel::loadFile(const QString& filePath)
{
	m_filePath = filePath;

	// 检查文件路径是否为空
	if (filePath.isEmpty()) {
		emit fileLoaded(false);
		return false;
	}

	// 关闭之前可能打开的文件
	if (m_file) {
		if (m_file->isOpen()) {
			m_file->close();
		}
		delete m_file;
		m_file = nullptr;
	}

	// 创建新的文件对象
	m_file = new QFile(filePath);
	if (!m_file->open(QIODevice::ReadOnly)) {
		delete m_file;
		m_file = nullptr;
		emit fileLoaded(false);
		return false;
	}

	// 获取文件大小
	qint64 fileSize = m_file->size();

	// 判断是否启用缓存模式（文件大于某个阈值时）
	m_useCache = true; //fileSize > 1024 * 1024; // 默认超过1MB启用缓存

	// 重置文件位置
	m_file->seek(0);

	// 在缓存模式下，m_text只保存当前页的内容
	m_text = "";
	//}

	// 加载第一页内容到缓存
	if (m_useCache && m_currentPage) {
		updatePageCache(m_currentPage);
	}
	else {
		updatePageCache(0);
	}

	//emit pageChanged(m_currentPage);
	emit fileLoaded(true);

	return true;
}

void TextDocumentModel::updatePageCache(int pageIndex)
{
	if (!m_useCache || !m_file || !m_file->isOpen()) {
		return;
	}

	// 获取编码器
	QTextCodec* codec = QTextCodec::codecForName(m_encoding.toUtf8());
	if (!codec) {
		codec = QTextCodec::codecForName("UTF-8");
	}

	// 如果我们有缓存的字符索引表，直接使用它
	if (m_charIndexMap.contains(pageIndex)) {
		qint64 startPos = m_charIndexMap[pageIndex];
		if (!m_file->seek(startPos)) {
			return;
		}
	}
	else {
		// 从最近的已知位置开始
		int nearestKnownPage = -1;
		for (int i = pageIndex - 1; i >= 0; i--) {
			if (m_charIndexMap.contains(i)) {
				nearestKnownPage = i;
				break;
			}
		}

		qint64 startPos = 0;
		int charCount = 0;

		if (nearestKnownPage >= 0) {
			// 从最近的已知页开始
			startPos = m_charIndexMap[nearestKnownPage];
			charCount = nearestKnownPage * m_numPerPage;
			if (!m_file->seek(startPos)) {
				return;
			}
		}
		else {
			// 从文件开始
			m_file->seek(0);
			// 清除缓存的索引表
			m_charIndexMap.clear();
			m_charIndexMap[0] = 0; // 第0页的开始位置是0
		}

		// 向前读取直到达到目标页的开始
		QByteArray buffer;
		int targetCharPos = pageIndex * m_numPerPage;

		while (charCount < targetCharPos && !m_file->atEnd()) {
			qint64 currentPos = m_file->pos();
			buffer = m_file->read(4096); // 读取合理大小的块
			if (buffer.isEmpty()) break;

			QString decodedText = codec->toUnicode(buffer);
			int charsInBuffer = decodedText.length();

			// 如果可以完整地跳过这个buffer
			if (charCount + charsInBuffer <= targetCharPos) {
				charCount += charsInBuffer;
				// 每当经过一个完整页的边界，记录该位置
				int pageJustPassed = charCount / m_numPerPage;
				if (charCount % m_numPerPage == 0 && !m_charIndexMap.contains(pageJustPassed)) {
					m_charIndexMap[pageJustPassed] = m_file->pos();
				}
			}
			else {
				// 需要在buffer中间找到正确的位置
				int charsNeeded = targetCharPos - charCount;
				QString firstPart = decodedText.left(charsNeeded);
				QByteArray encodedFirstPart = codec->fromUnicode(firstPart);

				// 计算目标位置
				startPos = currentPos + encodedFirstPart.size();

				// 记录找到的页面边界
				m_charIndexMap[pageIndex] = startPos;

				// 定位到正确位置
				if (!m_file->seek(startPos)) {
					return;
				}
				break;
			}
		}

		// 如果到达文件末尾还未找到目标页
		if (charCount < targetCharPos && m_file->atEnd()) {
			m_text.clear();
			return;
		}
	}

	// 读取一页数据，确保读取足够多以获取完整的m_numPerPage个字符
	const int bytesPerChar = 4; // UTF-8最大值
	const qint64 estimatedBytesPerPage = m_numPerPage * bytesPerChar * 2; // 额外冗余

	QByteArray pageData = m_file->read(estimatedBytesPerPage);
	QString fullText = codec->toUnicode(pageData);

	// 只保留需要的字符数
	if (fullText.length() > m_numPerPage) {
		m_text = fullText.left(m_numPerPage);

		// 计算并保存下一页的起始位置
		QByteArray encodedText = codec->fromUnicode(m_text);
		m_charIndexMap[pageIndex + 1] = m_charIndexMap[pageIndex] + encodedText.size();
	}
	else {
		m_text = fullText;

		// 如果字符不足且文件未结束，继续读取
		qint64 nextPageStartPos = m_file->pos(); // 先记录当前位置

		while (m_text.length() < m_numPerPage && !m_file->atEnd()) {
			QByteArray moreData = m_file->read(estimatedBytesPerPage);
			if (moreData.isEmpty()) break;

			QString moreText = codec->toUnicode(moreData);
			int remainingChars = m_numPerPage - m_text.length();

			if (moreText.length() <= remainingChars) {
				m_text += moreText;
				nextPageStartPos = m_file->pos();
			}
			else {
				QString addedText = moreText.left(remainingChars);
				m_text += addedText;

				// 计算下一页起始位置
				QByteArray encodedAddedText = codec->fromUnicode(addedText);
				nextPageStartPos = m_file->pos() - moreData.size() + encodedAddedText.size();
				break;
			}
		}

		// 如果我们填满了一页，保存下一页的起始位置
		if (m_text.length() == m_numPerPage) {
			m_charIndexMap[pageIndex + 1] = nextPageStartPos;
		}
	}

	m_currentPage = pageIndex;
	emit pageChanged(m_currentPage);
}

QString TextDocumentModel::getPageContent(int pageIndex)
{
	if (m_useCache) {
		// 只有当请求的页码与当前缓存页不同时才更新缓存
		if (pageIndex != m_currentPage) {
			updatePageCache(pageIndex);
		}
		return m_text;
	}
	else {
		// 非缓存模式，从内存中截取对应页的内容
		int startPos = pageIndex * m_numPerPage;
		if (startPos >= m_text.length()) {
			return QString();
		}

		return m_text.mid(startPos, m_numPerPage);
	}
}

int TextDocumentModel::getTotalPages() const
{
	return m_totalPage;
}

void TextDocumentModel::setTotalPages()
{
	if (m_numPerPage <= 0) { // 避免除零错误
		return;
	}

	if (m_useCache && m_file) {
		if (!m_file->isOpen()) {
			return;
		}

		// 记录当前文件指针位置
		qint64 originalPos = m_file->pos();

		QTextStream in(m_file);
		in.setCodec(m_encoding.toUtf8()); // 确保按照 UTF-8 解析
		m_file->seek(0);  // 读取前先跳到文件开头

		qint64 totalChars = 0; // 统计字符数
		while (!in.atEnd()) {
			QString line = in.readLine(); // 按行读取
			totalChars += line.length();  // 统计字符数
		}

		m_totalPage = (totalChars + m_numPerPage - 1) / m_numPerPage;

		// **恢复文件指针位置**
		m_file->seek(originalPos);

		return;
	}
	else if (!m_text.isEmpty()) {
		m_totalPage = (m_text.length() + m_numPerPage - 1) / m_numPerPage;
		return;
	}
}


void TextDocumentModel::setCharactersPerPage(int count)
{
	if (count > 0 && count != m_numPerPage) {
		m_numPerPage = count;
		setTotalPages();
		initIndexMap();
		m_charIndexMap.clear();
		// 更新当前页
		if (m_useCache) {
			updatePageCache(m_currentPage);
		}

		//emit pageChanged(m_currentPage);
	}
}

QString TextDocumentModel::currentFilePath() const
{
	return m_filePath;
}

int TextDocumentModel::getCurrentPage() const {
	return m_currentPage;
}

void TextDocumentModel::setCurrentPage(int page) {
	// 参数有效性检查
	if (page < 0 || (m_useCache && m_file && page >= getTotalPages())) {
		return;
	}

	// 如果页码没有变化，不做处理
	if (m_currentPage == page) {
		return;
	}

	// 更新当前页码
	m_currentPage = page;

	// 在缓存模式下，更新页面缓存内容
	if (m_useCache) {
		updatePageCache(page);
	}

}