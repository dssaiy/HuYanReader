#include "TextReaderView.h"
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QDebug>
#include <QCursor>
#include <QFontDatabase>


bool isChineseCharacter(QChar ch) {
	return (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF);
}

// 检测最适合的中文字体
QString TextReaderView::detectBestChineseFont() const {
	QFontDatabase fontDb;
	QStringList availableFamilies = fontDb.families();

	// 按优先级定义中文字体列表
	QStringList chineseFonts = {
		"SimHei",              // 黑体
		"SimSun",              // 宋体 (Windows经典)
		"Microsoft YaHei",     // 微软雅黑 (Windows推荐)
		"Microsoft YaHei UI",  // 微软雅黑UI
		"KaiTi",               // 楷体
		"FangSong",            // 仿宋
		"Noto Sans CJK SC",    // Google Noto (跨平台)
		"Source Han Sans SC",  // 思源黑体
		"PingFang SC",         // 苹方 (macOS)
		"Hiragino Sans GB",    // 冬青黑体 (macOS)
		"Arial Unicode MS",    // 通用Unicode字体
		"WenQuanYi Micro Hei", // 文泉驿微米黑 (Linux)
		"Droid Sans Fallback"  // Android回退字体
	};

	// 检查每个字体是否可用
	for (const QString& fontName : chineseFonts) {
		if (availableFamilies.contains(fontName)) {
			// 进一步验证字体是否真正支持中文
			QFont testFont(fontName);
			QFontMetrics fm(testFont);

			int chineseCharWidth = fm.horizontalAdvance(QString::fromUtf8("\u548c"));
			int latinCharWidth = fm.horizontalAdvance("A");

			// 中文字符宽度应该大于拉丁字符，且不为0
			if (chineseCharWidth > 0 && chineseCharWidth >= latinCharWidth) {
				qDebug() << "Detected Chinese font:" << fontName;
				return fontName;
			}
		}
	}

	// 如果没有找到合适的字体，返回系统默认字体
	qDebug() << "No Chinese font found, using system default";
	QFont defaultFont;
	return defaultFont.family();
}

TextReaderView::TextReaderView(QWidget* parent)
	: QWidget(parent)
	, m_resizeTimer(new QTimer(this))
	, m_contextMenu(new QMenu(this))
	, m_showPageNumber(0)
	, m_showProgress(0)
	, m_isDragging(false)
	, m_currentPage(0)
	, m_visibleLinesPerPage(0)
	, m_resizeRegion(None)
	, m_isResizing(false)
{

	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
	setFocusPolicy(Qt::StrongFocus);

	resize(600, 100);


	// 自动检测并设置最适合的中文字体
	QString bestChineseFont = detectBestChineseFont();
	m_font = QFont(bestChineseFont, 12);
	m_textColor = Qt::black;
	m_backgroundColor = QColor(0, 0, 0, 20);
	m_margins = QMargins(20, 20, 20, 20);
	
	
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_TranslucentBackground);
	setMouseTracking(true);
	
	setCursor(Qt::IBeamCursor);

	createContextMenu();
}

TextReaderView::~TextReaderView()
{
}

void TextReaderView::setFontSize(float size)
{
	m_fontSize = size;
	m_font.setPointSize(m_fontSize);
	update(); 
}

void TextReaderView::setTextSpacing(int spacing)
{
	m_textSpacing = spacing;
	update();
}

void TextReaderView::setLineSpacing(int spacing)
{
	m_lineSpacing = spacing;
	update();
}

void TextReaderView::setFontAndBackgroundColor(const QString& fontColor, const QString& backColor)
{

	QString cleanBackColor1 = fontColor.simplified();
	cleanBackColor1.remove(';');
	QString cleanBackColor2 = backColor.simplified();
	cleanBackColor2.remove(';');

	m_textColor = QColor(cleanBackColor1);
	m_backgroundColor = QColor(cleanBackColor2);
	m_backgroundColor.setAlpha(20);
}

void TextReaderView::setFontFamily(const QString& fontFamily)
{
	if (fontFamily.isEmpty()) {
		// 空字符串表示自动检测
		QString bestFont = detectBestChineseFont();
		m_font.setFamily(bestFont);
	} else {
		m_font.setFamily(fontFamily);
	}
	update(); // 重新绘制
}

void TextReaderView::showPage(const QString& text, int currentPage)
{
	m_currentPage = currentPage;
	m_formattedLines = formatText(text);
	refresh();
}

void TextReaderView::refresh()
{
	update();
}

void TextReaderView::setShowPageNumber(bool show)
{
	if (m_showPageNumber != show) {
		m_showPageNumber = show;
		update();
	}
}

void TextReaderView::setShowProgress(bool show)
{
	if (m_showProgress != show) {
		m_showProgress = show;
		update();
	}
}

void TextReaderView::setTotalPages(int totalPages) {
	m_totalPages = totalPages;  
	update();  
}

void TextReaderView::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);

	painter.fillRect(rect(), m_backgroundColor);

	drawText(painter);

	if (m_showPageNumber || m_showProgress) {
		drawPageInfo(painter);
	}
}

void TextReaderView::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	
	m_resizeTimer->start();
}

void TextReaderView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_resizeRegion = getResizeRegion(event->pos());
        if (m_resizeRegion != None) {
            m_isResizing = true;
            m_isDragging = false;
            m_dragStartPos = event->globalPos();
            m_windowStartGeometry = this->geometry();
            event->accept();
            return;
        }
        m_isDragging = false;
        m_dragStartPos = event->globalPos();
        m_windowStartPos = this->pos();
    } else if (event->button() == Qt::RightButton) {
        m_isDragging = false;
        m_dragStartPos = event->globalPos();
        m_windowStartPos = this->pos();
    }
    // Call base class if event not fully handled or for other buttons.
    // If you want to preserve base class behavior for unhandled parts of left/right clicks,
    // or for other mouse buttons, uncomment the line below.
    // However, for this specific resize/drag logic, we often manage event acceptance explicitly.
    // QWidget::mousePressEvent(event);
}

void TextReaderView::mouseMoveEvent(QMouseEvent* event)
{
    // Check if the left mouse button is pressed.
    if (event->buttons() & Qt::LeftButton) {
        if (m_isResizing) {
            // Resizing logic
            QPoint delta = event->globalPos() - m_dragStartPos;
            QRect newGeometry = m_windowStartGeometry;

            switch (m_resizeRegion) {
                case Top:
                    newGeometry.setTop(m_windowStartGeometry.top() + delta.y());
                    break;
                case Bottom:
                    newGeometry.setBottom(m_windowStartGeometry.bottom() + delta.y());
                    break;
                case Left:
                    newGeometry.setLeft(m_windowStartGeometry.left() + delta.x());
                    break;
                case Right:
                    newGeometry.setRight(m_windowStartGeometry.right() + delta.x());
                    break;
                case TopLeft:
                    newGeometry.setTopLeft(m_windowStartGeometry.topLeft() + delta);
                    break;
                case TopRight:
                    newGeometry.setTopRight(m_windowStartGeometry.topRight() + delta);
                    break;
                case BottomLeft:
                    newGeometry.setBottomLeft(m_windowStartGeometry.bottomLeft() + delta);
                    break;
                case BottomRight:
                    newGeometry.setBottomRight(m_windowStartGeometry.bottomRight() + delta);
                    break;
                default: // None
                    break;
            }

            // Enforce minimum size (e.g., 50x50)
            if (newGeometry.width() < 50) {
                if (m_resizeRegion == Left || m_resizeRegion == TopLeft || m_resizeRegion == BottomLeft) {
                    newGeometry.setLeft(newGeometry.right() - 50);
                } else {
                    newGeometry.setWidth(50);
                }
            }
            if (newGeometry.height() < 50) {
                if (m_resizeRegion == Top || m_resizeRegion == TopLeft || m_resizeRegion == TopRight) {
                    newGeometry.setTop(newGeometry.bottom() - 50);
                } else {
                    newGeometry.setHeight(50);
                }
            }

            this->setGeometry(newGeometry);
            event->accept();
        } else { // Not resizing, so it could be a drag.
            // If dragging hasn't been confirmed yet for this press action.
            if (!m_isDragging) {
                // Check if the mouse has moved enough to be considered a drag.
                if ((event->globalPos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
                    m_isDragging = true; // Now it's officially a drag.
                }
            }
            // If dragging is active (either just started or was already active).
            if (m_isDragging) {
                move(m_windowStartPos + (event->globalPos() - m_dragStartPos));
                event->accept();
            }
        }
    } else {
        // No buttons (or not the left button) are pressed; update cursor for hover.
        updateResizeCursor(event->pos());
        // Optionally call base class if other hover effects are needed.
        // QWidget::mouseMoveEvent(event);
    }
}

void TextReaderView::mouseReleaseEvent(QMouseEvent* event)
{
    bool wasResizing = m_isResizing;
    bool wasDragging = m_isDragging;

    m_isResizing = false;
    m_isDragging = false;
    m_resizeRegion = None;

    if (event->button() == Qt::LeftButton) {
        if (wasResizing || wasDragging) {
            
            
            event->accept();
        } else {
            
            emit nextPageRequested();
            event->accept();
        }
    } else if (event->button() == Qt::RightButton) {
        if (!wasDragging && !wasResizing) {
            emit previousPageRequested();
            event->accept();
        }
    }

    updateResizeCursor(event->pos());

    if (!event->isAccepted()) {
        QWidget::mouseReleaseEvent(event);
    }
}


void TextReaderView::mouseDoubleClickEvent(QMouseEvent* event)
{
	
	QWidget::mouseDoubleClickEvent(event);
}

void TextReaderView::keyPressEvent(QKeyEvent* event)
{
	
	const float opacityStep = 0.1;

	if (this->isVisible()) {
		if ((event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) ==
			(Qt::ControlModifier | Qt::AltModifier)) {
			if (event->key() == Qt::Key_Plus) {
				double currentOpacity = windowOpacity();
				currentOpacity += opacityStep;
				if (currentOpacity > 1.0) currentOpacity = 1.0;
				setWindowOpacity(currentOpacity);
				event->accept();
			}
			else if (event->key() == Qt::Key_Minus) {
				double currentOpacity = windowOpacity();
				currentOpacity -= opacityStep;
				if (currentOpacity < 0.1) currentOpacity = 0.1;
				setWindowOpacity(currentOpacity);
				event->accept();
			}
			else {
				QWidget::keyPressEvent(event);
			}
		}
		else {
			switch (event->key()) {
			case Qt::Key_PageDown:
			case Qt::Key_Space:
			case Qt::Key_Right:
			case Qt::Key_Down:
			case Qt::Key_3:
				emit nextPageRequested();
				event->accept();
				break;
			case Qt::Key_PageUp:
			case Qt::Key_Backspace:
			case Qt::Key_Left:
			case Qt::Key_Up:
			case Qt::Key_1:
				emit previousPageRequested();
				event->accept();
				break;

			default:
				QWidget::keyPressEvent(event);
			}
		}
	}
}


void TextReaderView::wheelEvent(QWheelEvent* event)
{
	
	if (event->angleDelta().y() < 0) {
		emit nextPageRequested();
	}
	
	else if (event->angleDelta().y() > 0) {
		emit previousPageRequested();
	}
	event->accept();
}


void TextReaderView::drawText(QPainter& painter)
{
	
	painter.setFont(m_font);
	painter.setPen(m_textColor);

	
	QRect textArea = textRect();
	int lineHeight = QFontMetrics(m_font).height();

	int y = textArea.top();
	for (const QString& line : m_formattedLines) {
		
		if (y + lineHeight > textArea.bottom()) {
			break;
		}

		painter.drawText(textArea.left(), y + QFontMetrics(m_font).ascent(), line);
		y += lineHeight;
	}
}

void TextReaderView::drawPageInfo(QPainter& painter)
{

	painter.save();
	painter.setFont(QFont(m_font.family(), m_font.pointSize() - 2));
	painter.setPen(m_textColor);

	QRect footerRect = rect().adjusted(m_margins.left(), 0, -m_margins.right(), -10);
	footerRect.setTop(rect().bottom() - 30);

	QString pageInfo;
	int totalPages = m_totalPages;

	
	if (m_showPageNumber) {
		pageInfo = QString(DSL("No. %1/%2 page")).arg(m_currentPage + 1).arg(totalPages);
	}

	if (m_showProgress) {
		int progress = totalPages > 0 ? ((m_currentPage + 1) * 100) / totalPages : 0;
		if (!pageInfo.isEmpty()) {
			pageInfo += " - ";
		}
		pageInfo += QString(DSL("进度: %1%")).arg(progress);
	}

	
	painter.drawText(footerRect, Qt::AlignCenter, pageInfo);
	painter.restore();
}


void TextReaderView::createContextMenu()
{


	QAction* actionShowPageNumber = m_contextMenu->addAction(DSL("显示页码"));
	actionShowPageNumber->setCheckable(true);
	actionShowPageNumber->setChecked(m_showPageNumber);
	connect(actionShowPageNumber, &QAction::toggled, this, &TextReaderView::setShowPageNumber);

	QAction* actionShowProgress = m_contextMenu->addAction(DSL("显示进度"));
	actionShowProgress->setCheckable(true);
	actionShowProgress->setChecked(m_showProgress);
	connect(actionShowProgress, &QAction::toggled, this, &TextReaderView::setShowProgress);

	
}


QRect TextReaderView::textRect() const
{
	
	return rect().adjusted(
		m_margins.left(),
		m_margins.top(),
		-m_margins.right(),
		-m_margins.bottom()
	);
}

QStringList TextReaderView::formatText(const QString& text) const
{
	QStringList lines;
	if (text.isEmpty()) return lines;

	QFontMetrics fm(m_font);
	int maxWidth = textRect().width();
	
	int spacing = 0;
	int spaceWidth = (spacing > 0) ? fm.horizontalAdvance(' ') : 0;

	
	QString cleanedText;
	for (const QChar& ch : text) {
		if (!ch.isSpace()) {
			cleanedText += ch;
		}
	}

	
	QStringList paragraphs = cleanedText.split('\n');
	for (const QString& paragraph : paragraphs) {
		if (paragraph.isEmpty()) {
			lines.append("");
			continue;
		}

		QString currentLine;
		int currentWidth = 0;

		for (int i = 0; i < paragraph.length(); i++) {
			QChar ch = paragraph.at(i);
			int charWidth = fm.horizontalAdvance(ch);

			
			int additionalWidth = charWidth;
			if (spacing > 0 && !currentLine.isEmpty() && i < paragraph.length() - 1) {
				additionalWidth += spaceWidth * spacing;
			}

			
			if (currentWidth + additionalWidth > maxWidth) {
				if (!currentLine.isEmpty()) {
					lines.append(currentLine);
					currentLine.clear();
					currentWidth = 0;
				}
				if (charWidth > maxWidth) {
					lines.append(ch); 
					continue;
				}
			}

			currentLine += ch;
			currentWidth += charWidth; 
		}

		if (!currentLine.isEmpty()) {
			lines.append(currentLine);
		}
	}
	return lines;
}

void TextReaderView::toggleVisibility()
{
	if (this->isVisible()) {
		this->hide();
	}
	else {
		this->show();
	}
}

void TextReaderView::showEvent(QShowEvent* event) {
	QWidget::showEvent(event); 
	activateWindow();
	setFocus();
}

QPoint TextReaderView::getRandomPointInRect(const QPoint& topLeft, const QPoint& bottomRight) {
	int x = QRandomGenerator::global()->bounded(topLeft.x(), bottomRight.x() + 1);
	int y = QRandomGenerator::global()->bounded(topLeft.y(), bottomRight.y() + 1);
	return QPoint(x, y);
}


TextReaderView::ResizeRegion TextReaderView::getResizeRegion(const QPoint& pos)
{
	   int x = pos.x();
	   int y = pos.y();
	   int w = width();
	   int h = height();

	   bool onLeft = x >= 0 && x < m_borderWidth;
	   bool onRight = x >= (w - m_borderWidth) && x < w;
	   bool onTop = y >= 0 && y < m_borderWidth;
	   bool onBottom = y >= (h - m_borderWidth) && y < h;

	   if (onTop && onLeft) return TopLeft;
	   if (onTop && onRight) return TopRight;
	   if (onBottom && onLeft) return BottomLeft;
	   if (onBottom && onRight) return BottomRight;
	   if (onTop) return Top;
	   if (onBottom) return Bottom;
	   if (onLeft) return Left;
	   if (onRight) return Right;
	   
	   return None;
}


void TextReaderView::updateResizeCursor(const QPoint& pos)
{
	   ResizeRegion region = getResizeRegion(pos);
	   switch (region) {
	       case Top:
	       case Bottom:
	           setCursor(Qt::SizeVerCursor);
	           break;
	       case Left:
	       case Right:
	           setCursor(Qt::SizeHorCursor);
	           break;
	       case TopLeft:
	       case BottomRight:
	           setCursor(Qt::SizeFDiagCursor);
	           break;
	       case TopRight:
	       case BottomLeft:
	           setCursor(Qt::SizeBDiagCursor);
	           break;
	       default: 
	           setCursor(Qt::ArrowCursor); 
	           break;
	   }
}