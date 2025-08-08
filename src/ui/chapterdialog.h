#ifndef CHAPTERDIALOG_H
#define CHAPTERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMap>

class ChapterDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChapterDialog(const QMap<int, QString>& m_menuIndexMap, QWidget* parent = nullptr);

signals:
	void chapterSelected(int pageIndex); // ���û�����½�ʱ�����ź�

private slots:
	void onItemClicked(QListWidgetItem* item);

private:
	QListWidget* listWidget;
	QMap<int, QString> m_menuIndexMap;
};

#endif // CHAPTERDIALOG_H
