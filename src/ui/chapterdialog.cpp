#include "chapterdialog.h"

ChapterDialog::ChapterDialog(const QMap<int, QString>& menuIndexMap, QWidget* parent)
	: QDialog(parent), m_menuIndexMap(menuIndexMap)
{
	setWindowTitle(u8"章节选择");
	setMinimumSize(400, 300);

	QVBoxLayout* layout = new QVBoxLayout(this);
	listWidget = new QListWidget(this);
	layout->addWidget(listWidget);

	// 填充 QListWidget
	for (auto it = m_menuIndexMap.begin(); it != m_menuIndexMap.end(); ++it) {
		QListWidgetItem* item = new QListWidgetItem(it.value(), listWidget);

		item->setData(Qt::UserRole, it.key()); // 存储章节对应的页码
	}

	// 连接点击信号
	connect(listWidget, &QListWidget::itemClicked, this, &ChapterDialog::onItemClicked);

	// 设置布局
	setLayout(layout);
}

void ChapterDialog::onItemClicked(QListWidgetItem* item)
{
	int pageIndex = item->data(Qt::UserRole).toInt(); 

	//int pageIndex = item->data(Qt::UserRole).toInt();
	emit chapterSelected(pageIndex); // 发送信号，通知 MainWindow
	accept(); // 关闭对话框
}
