#ifndef TEXTDOCUMENTMODEL_H
#define TEXTDOCUMENTMODEL_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QFile>

#include <QTextStream>         // 处理文本流
#include <QRegularExpression>  // 正则表达式匹配章节标题

#include "Settings.h"


/**
 * @brief TextDocumentModel 类负责管理文本阅读器的数据模型
 *
 * 该类负责文件的加载、文本内容管理、书签功能和文本查找
 */
class TextDocumentModel : public QObject
{
    Q_OBJECT

public:

    explicit TextDocumentModel(QObject* parent = nullptr);

    ~TextDocumentModel();

	void setTotalPages();

    void setMenuEncoding(const QString& encoding);

    void setEncoding(const QString& encoding);

    void setLinesPerPage(int lines);

	void setCharactersPerPage(int count);

	void setCurrentPage(int page);

    void reloadFile(const QString& filePath);

    /**
     * @brief 加载文件
     * @param filePath 文件路径
     * @return 加载是否成功
     */
    bool loadFile(const QString& filePath);

	// 获取指定页的文本内容
	QString getPageContent(int pageIndex);

	void initIndexMap();
    void initializeDocument();
	// 获取总页数
	int getTotalPages() const;

    int getCurrentPage() const;

    /**
     * @brief 获取当前文件路径
     * @return 当前文件路径
     */
    QString currentFilePath() const;


    /**
     * @brief 查找文本
     * @param text 要查找的文本
     * @param caseSensitive 是否区分大小写
     * @return 找到的所有位置列表
     */
    QList<int> findText(const QString& text, bool caseSensitive = false) const;

    QMap<int, QString> menuIndexMap() {
        return m_menuIndexMap;
    }

signals:
    /**
     * @brief 文件加载完成信号
     * @param success 是否成功
     */
    void fileLoaded(bool success);

	void pageChanged(int page);

    /**
     * @brief 书签变化信号
     */
    void bookmarkChanged();

private:

	QMap<int, QString> m_menuIndexMap; // 存储页码与章节标题
    QMap<int, qint64> m_charIndexMap;
    void updatePageCache(int pageIndex);

    QString m_filePath;       ///< 当前文件路径
    QString m_text;           ///< 文本内容
    QString m_encoding;       ///< 文件编码
    QString m_menuEncoding;       ///< 目录检索文件编码

    QMap<int, QString> m_bookmarks;  ///< 书签集合

    bool m_useCache;        // 是否启用缓存模式
	int m_currentPage;      // 当前页码
    int m_totalPage;      // 当前页码
	int m_numPerPage;       // 每页显示的字符数
	QFile* m_file;          // 用于大文件的随机访问

};

#endif // TEXTDOCUMENTMODEL_H

