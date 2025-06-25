#ifndef TEXTREADERVIEW_H
#define TEXTREADERVIEW_H

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QFontMetrics>
#include <QMessagebox>
#include <QColor>
#include <QRandomGenerator>

#include "TextDocumentModel.h"
#include "Settings.h"
#include "QHotkey.h" 

#define DSL(str) QStringLiteral(str)

class TextReaderView : public QWidget
{
	Q_OBJECT

public:
	explicit TextReaderView(QWidget* parent = nullptr);
	~TextReaderView();

	
	enum ResizeRegion {
		None,
		Top,
		Bottom,
		Left,
		Right,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight
	};

	void setFontSize(float size);

	void setTextSpacing(int spacing);

	void setLineSpacing(int spacing);

	void setFontAndBackgroundColor(const QString& fontColor, const QString& backColor);
	

	void showPage(const QString& text,int currentPage);

	void refresh();



public slots:

	
	void setShowPageNumber(bool show);
	void setShowProgress(bool show);

	
	void setTotalPages(int totalPages);

signals:
	
	void pageRequested(int page);
	void nextPageRequested();
	void previousPageRequested();
	void mouseClickedAt(const QPoint& pos);
	void contextMenuRequested(const QPoint& pos);

protected:
	
	void paintEvent(QPaintEvent* event) override;

	
	void resizeEvent(QResizeEvent* event) override;

	
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;

	
	void keyPressEvent(QKeyEvent* event) override;

	
	void wheelEvent(QWheelEvent* event) override;

	
	

	
	void showEvent(QShowEvent* event) override; 

private:
	
	void drawText(QPainter& painter);

	
	void drawPageInfo(QPainter& painter);

	
	

	
	void createContextMenu();

	
	QRect textRect() const;

	
	QStringList formatText(const QString& text) const;

	void toggleVisibility();
	
	QPoint getRandomPointInRect(const QPoint& topLeft, const QPoint& bottomRight);

	
	ResizeRegion getResizeRegion(const QPoint& pos);
	void updateResizeCursor(const QPoint& pos);

private:

	

	QFont m_font;                     
	int m_fontSize;
	int m_textSpacing;
	int m_lineSpacing;

	QColor m_textColor;               
	QColor m_backgroundColor;         
	QMargins m_margins;               

	QTimer* m_resizeTimer;            
	QMenu* m_contextMenu;             

	bool m_showPageNumber;            
	bool m_showProgress;              

	QStringList m_formattedLines;     
	int m_visibleLinesPerPage;        

	bool m_isDragging;                
	QPoint m_dragStartPos;            
	QPoint m_windowStartPos;

	// Resize related members
	const int m_borderWidth = 5;      // Border width for resize detection
	ResizeRegion m_resizeRegion;      // Current resize region
	bool m_isResizing;                // Flag to indicate if resizing is active
	QRect m_windowStartGeometry;      // Window geometry at the start of resize

	int m_currentPage;
	int m_totalPages;				  

};

#endif // TEXTREADERVIEW_H