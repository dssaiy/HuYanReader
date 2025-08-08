#include "FileGenerator.h"
#include "ChapterDownloader.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QStandardPaths>

FileGenerator::FileGenerator(QObject *parent)
    : QObject(parent)
    , m_isGenerating(false)
{
    initializeGenerator();
}

FileGenerator::~FileGenerator()
{
}

void FileGenerator::initializeGenerator()
{
    m_config = FileGeneratorConfig();

    QDir outputDir(m_config.outputPath);
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }

    emitDebugMessage("FileGenerator initialized");
}

void FileGenerator::setConfig(const FileGeneratorConfig &config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;

    QDir outputDir(config.outputPath);
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }

    emitDebugMessage(QString("Config updated: format=%1, encoding=%2")
                    .arg(static_cast<int>(config.format))
                    .arg(static_cast<int>(config.encoding)));
}

bool FileGenerator::generateFromTasks(const QList<DownloadTask> &tasks, const QString &fileName)
{
    if (tasks.isEmpty()) {
        setError("Task list is empty");
        return false;
    }

    if (m_isGenerating) {
        setError("Another file is being generated");
        return false;
    }

    m_isGenerating = true;
    QElapsedTimer timer;
    timer.start();

    QList<Chapter> chapters;
    QStringList contents;
    QString bookName, author;

    for (const DownloadTask &task : tasks) {
        if (task.status == DownloadStatus::Completed) {
            chapters.append(task.chapter);
            contents.append(task.content);

            if (bookName.isEmpty() && task.bookSource.bookRule()) {
                bookName = task.bookSource.name();
            }
        }
    }

    if (chapters.isEmpty()) {
        setError("No completed chapters");
        m_isGenerating = false;
        return false;
    }
    
    std::sort(chapters.begin(), chapters.end(), [](const Chapter &a, const Chapter &b) {
        return a.order() < b.order();
    });

    QStringList sortedContents;
    for (const Chapter &chapter : chapters) {
        for (int i = 0; i < tasks.size(); ++i) {
            if (tasks[i].chapter.order() == chapter.order() &&
                tasks[i].status == DownloadStatus::Completed) {
                sortedContents.append(tasks[i].content);
                break;
            }
        }
    }

    bool result = generateFromChapters(chapters, sortedContents, bookName, author, fileName);

    m_isGenerating = false;
    return result;
}

bool FileGenerator::generateFromChapters(const QList<Chapter> &chapters, const QStringList &contents,
                                        const QString &bookName, const QString &author,
                                        const QString &fileName)
{
    if (chapters.size() != contents.size()) {
        setError("Chapter count does not match content count");
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    GenerationStats stats;
    stats.totalChapters = chapters.size();
    stats.processedChapters = 0;

    emit generationStarted(stats.totalChapters, m_config.format);
    emitDebugMessage(QString("Starting generation: %1 chapters, format=%2")
                    .arg(stats.totalChapters)
                    .arg(static_cast<int>(m_config.format)));

    QString finalContent;

    try {
        switch (m_config.format) {
        case FileFormat::TXT:
            finalContent = convertToTXT(chapters, contents, bookName, author);
            break;
        case FileFormat::HTML:
            finalContent = convertToHTML(chapters, contents, bookName, author);
            break;
        case FileFormat::MARKDOWN:
            finalContent = convertToMarkdown(chapters, contents, bookName, author);
            break;
        case FileFormat::EPUB:
            return generateEPUB(chapters, contents, bookName, author, fileName);
        default:
            setError("Unsupported file format");
            return false;
        }

        QString outputFileName = fileName.isEmpty() ?
            generateFileName(bookName, author, m_config.format) : fileName;

        QString fullPath = QDir(m_config.outputPath).absoluteFilePath(outputFileName);

        if (!saveToFile(finalContent, fullPath)) {
            return false;
        }

        stats.processedChapters = chapters.size();
        stats.totalCharacters = finalContent.length();
        stats.totalLines = finalContent.count('\n') + 1;
        stats.fileSize = QFileInfo(fullPath).size();
        stats.processingTime = timer.elapsed();
        stats.outputFilePath = fullPath;

        m_lastStats = stats;

        emit generationFinished(stats);
        emitDebugMessage(QString("Generation completed: %1 (%2 chars, %3ms)")
                        .arg(fullPath)
                        .arg(stats.totalCharacters)
                        .arg(stats.processingTime));

        return true;

    } catch (const std::exception &e) {
        setError(QString("Exception during generation: %1").arg(e.what()));
        return false;
    }
}

QString FileGenerator::convertToTXT(const QList<Chapter> &chapters, const QStringList &contents,
                                   const QString &bookName, const QString &author) const
{
    QStringList result;

    if (!bookName.isEmpty()) {
        result << QString("Title: %1").arg(bookName);
    }
    if (!author.isEmpty()) {
        result << QString("Author: %1").arg(author);
    }
    if (!bookName.isEmpty() || !author.isEmpty()) {
        result << QString("Generated: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        result << "";
    }

    if (m_config.includeTableOfContents) {
        result << "Table of Contents";
        result << getChapterSeparatorString();
        result << generateTableOfContents(chapters);
        result << "";
        result << "";
    }

    for (int i = 0; i < chapters.size(); ++i) {
        const Chapter &chapter = chapters[i];
        const QString &content = contents[i];

        result << formatChapterTitle(chapter);
        result << getChapterSeparatorString();
        result << "";

        QString cleanedContent = m_config.cleanContent ?
            cleanChapterContent(content) : content;

        if (m_config.addLineBreaks) {
            cleanedContent = formatParagraphs(cleanedContent);
        }

        if (m_config.maxLineLength > 0) {
            cleanedContent = wrapLines(cleanedContent, m_config.maxLineLength);
        }

        result << cleanedContent;
        result << "";
        result << "";

        emit const_cast<FileGenerator*>(this)->generationProgress(i + 1, chapters.size(), chapter.title());
    }

    QString finalText = result.join('\n');

    if (m_config.removeEmptyLines) {
        finalText.replace(QRegularExpression("\n{3,}"), "\n\n");
    }

    return finalText;
}

QString FileGenerator::convertToHTML(const QList<Chapter> &chapters, const QStringList &contents,
                                    const QString &bookName, const QString &author) const
{
    QStringList html;

    html << "<!DOCTYPE html>";
    html << "<html lang=\"en\">";
    html << "<head>";
    html << "    <meta charset=\"UTF-8\">";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html << QString("    <title>%1</title>").arg(bookName.isEmpty() ? "Novel" : bookName);
    html << "    <style>";
    html << "        body { font-family: 'Microsoft YaHei', sans-serif; line-height: 1.6; margin: 40px; }";
    html << "        .book-info { text-align: center; margin-bottom: 40px; }";
    html << "        .toc { margin-bottom: 40px; }";
    html << "        .chapter { margin-bottom: 30px; }";
    html << "        .chapter-title { font-size: 1.5em; font-weight: bold; margin-bottom: 20px; }";
    html << "        .chapter-content { text-indent: 2em; }";
    html << "        .separator { border-top: 2px solid #ccc; margin: 20px 0; }";
    html << "    </style>";
    html << "</head>";
    html << "<body>";

    if (!bookName.isEmpty() || !author.isEmpty()) {
        html << "    <div class=\"book-info\">";
        if (!bookName.isEmpty()) {
            html << QString("        <h1>%1</h1>").arg(bookName);
        }
        if (!author.isEmpty()) {
            html << QString("        <p>Author: %1</p>").arg(author);
        }
        html << QString("        <p>Generated: %1</p>")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        html << "    </div>";
    }
    
    if (m_config.includeTableOfContents) {
        html << "    <div class=\"toc\">";
        html << "        <h2>Table of Contents</h2>";
        html << "        <ul>";
        for (const Chapter &chapter : chapters) {
            QString chapterTitle = formatChapterTitle(chapter);
            html << QString("            <li><a href=\"#chapter-%1\">%2</a></li>")
                    .arg(chapter.order())
                    .arg(chapterTitle);
        }
        html << "        </ul>";
        html << "    </div>";
        html << "    <div class=\"separator\"></div>";
    }

    for (int i = 0; i < chapters.size(); ++i) {
        const Chapter &chapter = chapters[i];
        const QString &content = contents[i];

        html << QString("    <div class=\"chapter\" id=\"chapter-%1\">").arg(chapter.order());
        html << QString("        <h2 class=\"chapter-title\">%1</h2>").arg(formatChapterTitle(chapter));

        QString cleanedContent = m_config.cleanContent ?
            cleanChapterContent(content) : content;

        QStringList paragraphs = cleanedContent.split('\n', Qt::SkipEmptyParts);
        for (const QString &paragraph : paragraphs) {
            if (!paragraph.trimmed().isEmpty()) {
                html << QString("        <p class=\"chapter-content\">%1</p>").arg(paragraph.trimmed());
            }
        }

        html << "    </div>";

        emit const_cast<FileGenerator*>(this)->generationProgress(i + 1, chapters.size(), chapter.title());
    }

    html << "</body>";
    html << "</html>";

    return html.join('\n');
}

QString FileGenerator::convertToMarkdown(const QList<Chapter> &chapters, const QStringList &contents,
                                        const QString &bookName, const QString &author) const
{
    QStringList md;

    if (!bookName.isEmpty()) {
        md << QString("# %1").arg(bookName);
        md << "";
    }
    if (!author.isEmpty()) {
        md << QString("**Author**: %1").arg(author);
    }
    md << QString("**Generated**: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    md << "";
    md << "---";
    md << "";

    if (m_config.includeTableOfContents) {
        md << "## Table of Contents";
        md << "";
        for (const Chapter &chapter : chapters) {
            QString chapterTitle = formatChapterTitle(chapter);
            QString anchor = QString("chapter-%1").arg(chapter.order());
            md << QString("- [%1](#%2)").arg(chapterTitle).arg(anchor);
        }
        md << "";
        md << "---";
        md << "";
    }

    for (int i = 0; i < chapters.size(); ++i) {
        const Chapter &chapter = chapters[i];
        const QString &content = contents[i];

        md << QString("## %1 {#chapter-%2}").arg(formatChapterTitle(chapter)).arg(chapter.order());
        md << "";

        QString cleanedContent = m_config.cleanContent ?
            cleanChapterContent(content) : content;

        QStringList paragraphs = cleanedContent.split('\n', Qt::SkipEmptyParts);
        for (const QString &paragraph : paragraphs) {
            if (!paragraph.trimmed().isEmpty()) {
                md << paragraph.trimmed();
                md << "";
            }
        }

        md << "---";
        md << "";

        emit const_cast<FileGenerator*>(this)->generationProgress(i + 1, chapters.size(), chapter.title());
    }

    return md.join('\n');
}

QString FileGenerator::cleanChapterContent(const QString &content) const
{
    QString cleaned = content;

    cleaned = cleanHtmlTags(cleaned);

    QStringList adPatterns = {
        "www\\..*?\\.com",
        "\\(.*?www\\..*?\\.com.*?\\)"
    };

    for (const QString &pattern : adPatterns) {
        cleaned.remove(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption));
    }

    cleaned.replace(QRegularExpression("\\s+"), " ");
    cleaned.replace(QRegularExpression("\\s*\\n\\s*"), "\n");

    cleaned = cleaned.trimmed();

    return cleaned;
}

QString FileGenerator::formatChapterTitle(const Chapter &chapter) const
{
    QString title = chapter.title();

    if (m_config.includeChapterNumbers && chapter.order() > 0) {
        if (!title.contains(QRegularExpression("Chapter\\s*\\d+"))) {
            title = QString("Chapter %1: %2").arg(chapter.order()).arg(title);
        }
    }

    return title;
}

QString FileGenerator::generateTableOfContents(const QList<Chapter> &chapters) const
{
    QStringList toc;

    for (const Chapter &chapter : chapters) {
        QString title = formatChapterTitle(chapter);
        toc << QString("%1. %2").arg(chapter.order()).arg(title);
    }

    return toc.join('\n');
}

QString FileGenerator::getChapterSeparatorString() const
{
    switch (m_config.separator) {
    case ChapterSeparator::None:
        return "";
    case ChapterSeparator::SimpleLine:
        return "----------------------------------------";
    case ChapterSeparator::DoubleLine:
        return "========================================";
    case ChapterSeparator::StarLine:
        return "* * * * * * * * * * * * * * * * * * * *";
    case ChapterSeparator::CustomLine:
        return m_config.customSeparator;
    default:
        return "========================================";
    }
}

QString FileGenerator::cleanHtmlTags(const QString &content) const
{
    QString cleaned = content;

    cleaned.remove(QRegularExpression("<[^>]*>"));

    cleaned.replace("&nbsp;", " ");
    cleaned.replace("&lt;", "<");
    cleaned.replace("&gt;", ">");
    cleaned.replace("&amp;", "&");
    cleaned.replace("&quot;", "\"");
    cleaned.replace("&#39;", "'");

    return cleaned;
}

QString FileGenerator::formatParagraphs(const QString &content) const
{
    QStringList lines = content.split('\n');
    QStringList formatted;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            if (!formatted.isEmpty() && !formatted.last().isEmpty()) {
                formatted << "";
            }
            formatted << QString("    %1").arg(trimmed);
        }
    }

    return formatted.join('\n');
}

QString FileGenerator::wrapLines(const QString &content, int maxLength) const
{
    if (maxLength <= 0) {
        return content;
    }

    QStringList lines = content.split('\n');
    QStringList wrapped;

    for (const QString &line : lines) {
        if (line.length() <= maxLength) {
            wrapped << line;
        } else {
            QString remaining = line;
            while (remaining.length() > maxLength) {
                wrapped << remaining.left(maxLength);
                remaining = remaining.mid(maxLength);
            }
            if (!remaining.isEmpty()) {
                wrapped << remaining;
            }
        }
    }

    return wrapped.join('\n');
}

bool FileGenerator::saveToFile(const QString &content, const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const_cast<FileGenerator*>(this)->setError(QString("Cannot create file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);

    QTextCodec *codec = getTextCodec(m_config.encoding);
    if (codec) {
        out.setCodec(codec);
    }

    if (m_config.encoding == TextEncoding::UTF8_BOM) {
        out << QString::fromUtf8("\xEF\xBB\xBF");
    }

    out << content;

    return true;
}

QTextCodec* FileGenerator::getTextCodec(TextEncoding encoding) const
{
    switch (encoding) {
    case TextEncoding::UTF8:
    case TextEncoding::UTF8_BOM:
        return QTextCodec::codecForName("UTF-8");
    case TextEncoding::GBK:
        return QTextCodec::codecForName("GBK");
    case TextEncoding::GB2312:
        return QTextCodec::codecForName("GB2312");
    default:
        return QTextCodec::codecForName("UTF-8");
    }
}

QString FileGenerator::generateFileName(const QString &bookName, const QString &author,
                                       FileFormat format, const QString &customName) const
{
    if (!customName.isEmpty()) {
        return customName;
    }

    QString fileName = m_config.fileNameTemplate;
    fileName.replace("{bookName}", FileFormatUtils::sanitizeFileName(bookName));
    fileName.replace("{author}", FileFormatUtils::sanitizeFileName(author));
    fileName.replace("{date}", QDate::currentDate().toString("yyyy-MM-dd"));
    fileName.replace("{time}", QTime::currentTime().toString("hh-mm-ss"));

    QString extension = FileFormatUtils::getFormatExtension(format);
    if (!fileName.endsWith(extension)) {
        fileName += extension;
    }

    return fileName;
}

void FileGenerator::setError(const QString &error)
{
    m_lastError = error;
    emit generationError(error);
    emitDebugMessage(QString("Error: %1").arg(error));
}

bool FileGenerator::generateFromDownloader(ChapterDownloader *downloader, const QString &fileName)
{
    if (!downloader) {
        setError("ChapterDownloader is null");
        return false;
    }

    QList<DownloadTask> completedTasks = downloader->getCompletedTasks();
    return generateFromTasks(completedTasks, fileName);
}

bool FileGenerator::generateMultipleFiles(const QMap<QString, QList<DownloadTask>> &bookTasks)
{
    bool allSuccess = true;

    for (auto it = bookTasks.begin(); it != bookTasks.end(); ++it) {
        const QString &bookName = it.key();
        const QList<DownloadTask> &tasks = it.value();

        QString fileName = generateFileName(bookName, "", m_config.format);
        if (!generateFromTasks(tasks, fileName)) {
            allSuccess = false;
            emitDebugMessage(QString("Failed to generate file: %1").arg(bookName));
        }
    }

    return allSuccess;
}

bool FileGenerator::generateEPUB(const QList<Chapter> &chapters, const QStringList &contents,
                                const QString &bookName, const QString &author,
                                const QString &filePath) const
{
    const_cast<FileGenerator*>(this)->emitDebugMessage("EPUB generation not fully implemented");
    const_cast<FileGenerator*>(this)->setError("EPUB generation not fully implemented");
    return false;
}

void FileGenerator::emitDebugMessage(const QString &message)
{
    emit debugMessage(QString("[FileGenerator] %1").arg(message));
}

FileFormat FileFormatUtils::detectFormat(const QString &filePath)
{
    QString extension = QFileInfo(filePath).suffix().toLower();

    if (extension == "txt") {
        return FileFormat::TXT;
    } else if (extension == "epub") {
        return FileFormat::EPUB;
    } else if (extension == "html" || extension == "htm") {
        return FileFormat::HTML;
    } else if (extension == "md" || extension == "markdown") {
        return FileFormat::MARKDOWN;
    }

    return FileFormat::TXT;
}

QString FileFormatUtils::getFormatExtension(FileFormat format)
{
    switch (format) {
    case FileFormat::TXT:
        return ".txt";
    case FileFormat::EPUB:
        return ".epub";
    case FileFormat::HTML:
        return ".html";
    case FileFormat::MARKDOWN:
        return ".md";
    default:
        return ".txt";
    }
}

QString FileFormatUtils::getFormatName(FileFormat format)
{
    switch (format) {
    case FileFormat::TXT:
        return "Plain Text";
    case FileFormat::EPUB:
        return "EPUB eBook";
    case FileFormat::HTML:
        return "HTML";
    case FileFormat::MARKDOWN:
        return "Markdown";
    default:
        return "Unknown Format";
    }
}

TextEncoding FileFormatUtils::detectEncoding(const QByteArray &data)
{
    if (data.startsWith("\xEF\xBB\xBF")) {
        return TextEncoding::UTF8_BOM;
    }

    if (data.contains('\0')) {
        return TextEncoding::UTF8;
    }

    QString text = QString::fromUtf8(data);
    if (text.contains(QRegularExpression("[\\x4e00-\\x9fff]"))) {
        return TextEncoding::UTF8;
    }

    return TextEncoding::UTF8;
}

QString FileFormatUtils::getEncodingName(TextEncoding encoding)
{
    switch (encoding) {
    case TextEncoding::UTF8:
        return "UTF-8";
    case TextEncoding::UTF8_BOM:
        return "UTF-8 with BOM";
    case TextEncoding::GBK:
        return "GBK";
    case TextEncoding::GB2312:
        return "GB2312";
    default:
        return "UTF-8";
    }
}

bool FileFormatUtils::isValidFileName(const QString &fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }

    QStringList invalidChars = {"<", ">", ":", "\"", "|", "?", "*", "/", "\\"};
    for (const QString &ch : invalidChars) {
        if (fileName.contains(ch)) {
            return false;
        }
    }

    QStringList reservedNames = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4",
                                "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2",
                                "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    QString baseName = QFileInfo(fileName).baseName().toUpper();
    if (reservedNames.contains(baseName)) {
        return false;
    }

    return true;
}

QString FileFormatUtils::sanitizeFileName(const QString &fileName)
{
    QString sanitized = fileName;

    QStringList invalidChars = {"<", ">", ":", "\"", "|", "?", "*", "/", "\\"};
    for (const QString &ch : invalidChars) {
        sanitized.replace(ch, "_");
    }

    sanitized = sanitized.trimmed();
    while (sanitized.endsWith('.')) {
        sanitized.chop(1);
    }

    if (sanitized.length() > 200) {
        sanitized = sanitized.left(200);
    }

    if (sanitized.isEmpty()) {
        sanitized = "untitled";
    }

    return sanitized;
}

QString FileFormatUtils::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / (double)GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / (double)MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / (double)KB, 0, 'f', 2);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

qint64 FileFormatUtils::estimateFileSize(const QStringList &contents, TextEncoding encoding)
{
    qint64 totalChars = 0;
    for (const QString &content : contents) {
        totalChars += content.length();
    }

    switch (encoding) {
    case TextEncoding::UTF8:
    case TextEncoding::UTF8_BOM:
        return totalChars * 3;
    case TextEncoding::GBK:
    case TextEncoding::GB2312:
        return totalChars * 2;
    default:
        return totalChars * 3;
    }
}
