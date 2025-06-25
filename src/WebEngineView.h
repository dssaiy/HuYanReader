#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QWidget>
#include <QWebEngineView>
#include <QCloseEvent>
#include <QLineEdit>
#include <QPushButton>

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

private slots:
    void loadUrl();
    void updateAddressBar(const QUrl &url);

private:
    QWebEngineView *m_webView;
    QLineEdit *m_addressBar;
    QPushButton *m_goButton;
};

#endif // WEBENGINEVIEW_H