#ifndef TEXTDOCUMENTMODEL_H
#define TEXTDOCUMENTMODEL_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QFile>

#include <QTextStream>         // �����ı���
#include <QRegularExpression>  // �������ʽƥ���½ڱ���

#include "../config/settings.h"


/**
 * @brief TextDocumentModel �ฺ������ı��Ķ���������ģ��
 *
 * ���ฺ���ļ��ļ��ء��ı����ݹ�������ǩ���ܺ��ı�����
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
     * @brief �����ļ�
     * @param filePath �ļ�·��
     * @return �����Ƿ�ɹ�
     */
    bool loadFile(const QString& filePath);

	// ��ȡָ��ҳ���ı�����
	QString getPageContent(int pageIndex);

	void initIndexMap();
    void initializeDocument();
	// ��ȡ��ҳ��
	int getTotalPages() const;

    int getCurrentPage() const;

    /**
     * @brief ��ȡ��ǰ�ļ�·��
     * @return ��ǰ�ļ�·��
     */
    QString currentFilePath() const;


    /**
     * @brief �����ı�
     * @param text Ҫ���ҵ��ı�
     * @param caseSensitive �Ƿ����ִ�Сд
     * @return �ҵ�������λ���б�
     */
    QList<int> findText(const QString& text, bool caseSensitive = false) const;

    QMap<int, QString> menuIndexMap() {
        return m_menuIndexMap;
    }

signals:
    /**
     * @brief �ļ���������ź�
     * @param success �Ƿ�ɹ�
     */
    void fileLoaded(bool success);

	void pageChanged(int page);

    /**
     * @brief ��ǩ�仯�ź�
     */
    void bookmarkChanged();

private:

	QMap<int, QString> m_menuIndexMap; // �洢ҳ�����½ڱ���
    QMap<int, qint64> m_charIndexMap;
    void updatePageCache(int pageIndex);

    QString m_filePath;       ///< ��ǰ�ļ�·��
    QString m_text;           ///< �ı�����
    QString m_encoding;       ///< �ļ�����
    QString m_menuEncoding;       ///< Ŀ¼�����ļ�����

    QMap<int, QString> m_bookmarks;  ///< ��ǩ����

    bool m_useCache;        // �Ƿ����û���ģʽ
	int m_currentPage;      // ��ǰҳ��
    int m_totalPage;      // ��ǰҳ��
	int m_numPerPage;       // ÿҳ��ʾ���ַ���
	QFile* m_file;          // ���ڴ��ļ����������

};

#endif // TEXTDOCUMENTMODEL_H

