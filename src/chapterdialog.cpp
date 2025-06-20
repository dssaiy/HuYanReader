#include "chapterdialog.h"

ChapterDialog::ChapterDialog(const QMap<int, QString>& menuIndexMap, QWidget* parent)
	: QDialog(parent), m_menuIndexMap(menuIndexMap)
{
	setWindowTitle(u8"�½�ѡ��");
	setMinimumSize(400, 300);

	QVBoxLayout* layout = new QVBoxLayout(this);
	listWidget = new QListWidget(this);
	layout->addWidget(listWidget);

	// ��� QListWidget
	for (auto it = m_menuIndexMap.begin(); it != m_menuIndexMap.end(); ++it) {
		QListWidgetItem* item = new QListWidgetItem(it.value(), listWidget);

		item->setData(Qt::UserRole, it.key()); // �洢�½ڶ�Ӧ��ҳ��
	}

	// ���ӵ���ź�
	connect(listWidget, &QListWidget::itemClicked, this, &ChapterDialog::onItemClicked);

	// ���ò���
	setLayout(layout);
}

void ChapterDialog::onItemClicked(QListWidgetItem* item)
{
	int pageIndex = item->data(Qt::UserRole).toInt(); 

	//int pageIndex = item->data(Qt::UserRole).toInt();
	emit chapterSelected(pageIndex); // �����źţ�֪ͨ MainWindow
	accept(); // �رնԻ���
}
