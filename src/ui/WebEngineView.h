#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QWidget>
#include <QWebEngineView>
#include <QCloseEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QFrame>

class QSizeGrip;

class WebEngineView : public QWidget
{
    Q_OBJECT

public:
    explicit WebEngineView(QWidget *parent = nullptr);

public slots:
    void setOpacity(double opacity);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void onPageLoadFinished(bool ok);

private slots:
    void loadUrl();
    void updateAddressBar(const QUrl &url);

private:
    QWebEngineView *m_webView;
    QLineEdit *m_addressBar;
    QPushButton *m_goButton;
    QPushButton *m_backButton;
    QPushButton *m_forwardButton;
    QPushButton *m_refreshButton;
    QFrame *m_dragBar;
    QSizeGrip *m_sizeGrip;

    bool m_isDragging = false;
    QPoint m_dragStartPos;
};

#endif // WEBENGINEVIEW_H