#ifndef FILEGENERATOR_H
#define FILEGENERATOR_H

#include <QObject>
#include <QStringList>
#include <QTextStream>
#include <QDir>
#include <QMutex>
#include "NovelModels.h"

class ChapterDownloader;
struct DownloadTask;
enum class DownloadStatus;

enum class FileFormat {
    TXT,
    EPUB,
    HTML,
    MARKDOWN
};

enum class TextEncoding {
    UTF8,
    UTF8_BOM,
    GBK,
    GB2312
};

enum class ChapterSeparator {
    None,
    SimpleLine,
    DoubleLine,
    StarLine,
    CustomLine
};

struct FileGeneratorConfig {
    FileFormat format = FileFormat::TXT;
    TextEncoding encoding = TextEncoding::UTF8;
    ChapterSeparator separator = ChapterSeparator::DoubleLine;
    QString customSeparator;
    bool includeTableOfContents = true;
    bool includeChapterNumbers = true;
    bool cleanContent = true;
    bool removeEmptyLines = true;
    bool addLineBreaks = true;
    int maxLineLength = 0;
    QString outputPath = "output/";
    QString fileNameTemplate = "{bookName}_{author}";

    QString epubTitle;
    QString epubAuthor;
    QString epubLanguage = "zh-CN";
    QString epubCoverImage;
};

struct GenerationStats {
    int totalChapters = 0;
    int processedChapters = 0;
    int totalCharacters = 0;
    int totalLines = 0;
    qint64 fileSize = 0;
    qint64 processingTime = 0;
    QString outputFilePath;

    double getProgress() const {
        return totalChapters > 0 ? (double)processedChapters / totalChapters * 100.0 : 0.0;
    }
};

/**
 * @brief File generator and format processor
 */
class FileGenerator : public QObject
{
    Q_OBJECT

public:
    explicit FileGenerator(QObject *parent = nullptr);
    ~FileGenerator();

    void setConfig(const FileGeneratorConfig &config);
    FileGeneratorConfig getConfig() const { return m_config; }

    bool generateFromTasks(const QList<DownloadTask> &tasks, const QString &fileName = QString());
    bool generateFromChapters(const QList<Chapter> &chapters, const QStringList &contents,
                             const QString &bookName, const QString &author,
                             const QString &fileName = QString());

    bool generateMultipleFiles(const QMap<QString, QList<DownloadTask>> &bookTasks);
    bool generateFromDownloader(ChapterDownloader *downloader, const QString &fileName = QString());

    QString cleanChapterContent(const QString &content) const;
    QString formatChapterTitle(const Chapter &chapter) const;
    QString generateTableOfContents(const QList<Chapter> &chapters) const;

    bool saveToFile(const QString &content, const QString &filePath) const;
    QString generateFileName(const QString &bookName, const QString &author,
                           FileFormat format, const QString &customName = QString()) const;

    QString convertToTXT(const QList<Chapter> &chapters, const QStringList &contents,
                        const QString &bookName, const QString &author) const;
    QString convertToHTML(const QList<Chapter> &chapters, const QStringList &contents,
                         const QString &bookName, const QString &author) const;
    QString convertToMarkdown(const QList<Chapter> &chapters, const QStringList &contents,
                             const QString &bookName, const QString &author) const;

    bool generateEPUB(const QList<Chapter> &chapters, const QStringList &contents,
                     const QString &bookName, const QString &author,
                     const QString &filePath) const;

    GenerationStats getLastStats() const { return m_lastStats; }
    bool isGenerating() const { return m_isGenerating; }

signals:
    void generationStarted(int totalChapters, FileFormat format);
    void generationProgress(int processed, int total, const QString &currentChapter);
    void generationFinished(const GenerationStats &stats);
    void generationError(const QString &error);
    void debugMessage(const QString &message);

private:
    void initializeGenerator();
    QString getChapterSeparatorString() const;
    QString encodeText(const QString &text, TextEncoding encoding) const;
    QTextCodec* getTextCodec(TextEncoding encoding) const;
    QString cleanHtmlTags(const QString &content) const;
    QString formatParagraphs(const QString &content) const;
    QString wrapLines(const QString &content, int maxLength) const;
    void updateStats(const GenerationStats &stats);
    void setError(const QString &error);
    void emitDebugMessage(const QString &message);

    bool createEPUBStructure(const QString &tempDir) const;
    QString generateEPUBContent(const QList<Chapter> &chapters, const QStringList &contents,
                               const QString &bookName, const QString &author) const;
    QString generateEPUBTOC(const QList<Chapter> &chapters) const;
    bool packageEPUB(const QString &tempDir, const QString &outputPath) const;

    FileGeneratorConfig m_config;
    GenerationStats m_lastStats;
    bool m_isGenerating;
    QString m_lastError;
    QMutex m_mutex;
};

class FileFormatUtils
{
public:
    static FileFormat detectFormat(const QString &filePath);
    static QString getFormatExtension(FileFormat format);
    static QString getFormatName(FileFormat format);

    static TextEncoding detectEncoding(const QByteArray &data);
    static QString getEncodingName(TextEncoding encoding);

    static bool isValidFileName(const QString &fileName);
    static QString sanitizeFileName(const QString &fileName);

    static QString formatFileSize(qint64 bytes);
    static qint64 estimateFileSize(const QStringList &contents, TextEncoding encoding);
};

#endif // FILEGENERATOR_H
