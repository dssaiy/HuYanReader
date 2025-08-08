#include "NovelSearchViewEnhanced.h"
#include "../novel/NovelSearchManager.h"
#include "../config/NovelConfig.h"
#include "../config/NovelSettingsDialog.h"
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

NovelSearchViewEnhanced::NovelSearchViewEnhanced(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchGroup(nullptr)
    , m_resultsGroup(nullptr)
    , m_downloadGroup(nullptr)
    , m_progressGroup(nullptr)
    , m_sourceComboBox(nullptr)
    , m_searchLineEdit(nullptr)
    , m_searchButton(nullptr)
    , m_settingsButton(nullptr)
    , m_resultsTable(nullptr)
    , m_downloadModeComboBox(nullptr)
    , m_startChapterSpinBox(nullptr)
    , m_endChapterSpinBox(nullptr)
    , m_customPathLineEdit(nullptr)
    , m_browseButton(nullptr)
    , m_downloadButton(nullptr)
    , m_cancelButton(nullptr)
    , m_searchProgressBar(nullptr)
    , m_downloadProgressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_sourceStatusLabel(nullptr)
    , m_isSearching(false)
    , m_isDownloading(false)
    , m_novelConfig(nullptr)
    , m_settingsDialog(nullptr)
{
    // Remove frameless window flag and add normal window controls
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setWindowTitle("Novel Search - Enhanced");
    setMinimumSize(600, 500);
    resize(800, 600);
    
    setupEnhancedUI();
    
    qDebug() << "NovelSearchViewEnhanced created successfully";
}

NovelSearchViewEnhanced::~NovelSearchViewEnhanced()
{
    qDebug() << "NovelSearchViewEnhanced destroyed";
}

void NovelSearchViewEnhanced::setNovelConfig(NovelConfig* config)
{
    m_novelConfig = config;

    // Create settings dialog when config is set
    if (m_novelConfig && !m_settingsDialog) {
        m_settingsDialog = new NovelSettingsDialog(m_novelConfig, this);
    }

    // Load default download path from config
    if (m_novelConfig && m_customPathLineEdit) {
        QString defaultPath = m_novelConfig->getDownloadPath();
        if (defaultPath.isEmpty()) {
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/NovelDownloads";
        }
        m_customPathLineEdit->setText(defaultPath);
    }

    qDebug() << "NovelConfig linked successfully";
}

void NovelSearchViewEnhanced::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}

void NovelSearchViewEnhanced::setupEnhancedUI()
{
    // Clear existing simple layout
    if (layout()) {
        QLayoutItem *item;
        while ((item = layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout();
    }
    
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Setup each section
    setupSearchSection();
    setupResultsSection();
    setupDownloadSection();
    setupProgressSection();
    
    setLayout(m_mainLayout);
    
    // Initialize UI state
    resetUI();
}

void NovelSearchViewEnhanced::setupSearchSection()
{
    m_searchGroup = new QGroupBox("Search Settings", this);
    m_searchLayout = new QHBoxLayout(m_searchGroup);
    
    // Source selection
    QLabel *sourceLabel = new QLabel("Source:", m_searchGroup);
    m_sourceComboBox = new QComboBox(m_searchGroup);
    m_sourceComboBox->addItem("All Sources", -1);
    m_sourceComboBox->addItem("Qidian (起点中文网)", 1);
    m_sourceComboBox->addItem("Zongheng (纵横中文网)", 2);
    m_sourceComboBox->addItem("17K Novel", 3);
    m_sourceComboBox->addItem("Jinjiang (晋江文学城)", 4);
    m_sourceComboBox->addItem("Xiaoxiang (潇湘书院)", 5);
    m_sourceComboBox->addItem("Custom Source", 99);
    
    // Search input
    QLabel *keywordLabel = new QLabel("Keyword:", m_searchGroup);
    m_searchLineEdit = new QLineEdit(m_searchGroup);
    m_searchLineEdit->setPlaceholderText("Enter novel title or author");
    
    // Search button
    m_searchButton = new QPushButton("Search", m_searchGroup);
    m_searchButton->setMinimumWidth(80);
    
    // Settings button
    m_settingsButton = new QPushButton("Settings", m_searchGroup);
    m_settingsButton->setMinimumWidth(60);
    
    // Layout
    m_searchLayout->addWidget(sourceLabel);
    m_searchLayout->addWidget(m_sourceComboBox);
    m_searchLayout->addWidget(keywordLabel);
    m_searchLayout->addWidget(m_searchLineEdit, 1);
    m_searchLayout->addWidget(m_searchButton);
    m_searchLayout->addWidget(m_settingsButton);
    
    m_mainLayout->addWidget(m_searchGroup);
    
    // Connect signals
    connect(m_searchButton, &QPushButton::clicked, this, &NovelSearchViewEnhanced::onSearchButtonClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &NovelSearchViewEnhanced::onSettingsButtonClicked);
    connect(m_sourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            [this](int index) {
                int sourceId = m_sourceComboBox->itemData(index).toInt();
                emit sourceChanged(sourceId);
            });
    
    // Enter to search
    connect(m_searchLineEdit, &QLineEdit::returnPressed, this, &NovelSearchViewEnhanced::onSearchButtonClicked);
}

void NovelSearchViewEnhanced::setupResultsSection()
{
    m_resultsGroup = new QGroupBox("Search Results", this);
    m_resultsLayout = new QVBoxLayout(m_resultsGroup);
    
    // Results table
    m_resultsTable = new QTableWidget(0, 4, m_resultsGroup);
    QStringList headers;
    headers << "Title" << "Author" << "Latest Chapter" << "Source";
    m_resultsTable->setHorizontalHeaderLabels(headers);
    
    // Set table properties
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->verticalHeader()->setVisible(false);
    
    // Set column width
    m_resultsTable->setColumnWidth(0, 200);  // Title
    m_resultsTable->setColumnWidth(1, 120);  // Author
    m_resultsTable->setColumnWidth(2, 150);  // Latest Chapter
    
    m_resultsLayout->addWidget(m_resultsTable);
    m_mainLayout->addWidget(m_resultsGroup);
    
    // Connect signals
    connect(m_resultsTable, &QTableWidget::itemSelectionChanged,
            this, &NovelSearchViewEnhanced::onResultSelectionChanged);
}

void NovelSearchViewEnhanced::setupDownloadSection()
{
    m_downloadGroup = new QGroupBox("Download Settings", this);
    m_downloadMainLayout = new QVBoxLayout(m_downloadGroup);

    // Download mode selection
    m_downloadModeLayout = new QHBoxLayout();
    m_downloadModeLabel = new QLabel("Download Mode:", m_downloadGroup);
    m_downloadModeComboBox = new QComboBox(m_downloadGroup);
    m_downloadModeComboBox->addItem("Chapter Range", static_cast<int>(DownloadMode::ChapterRange));
    m_downloadModeComboBox->addItem("Full Novel", static_cast<int>(DownloadMode::FullNovel));
    m_downloadModeComboBox->addItem("Custom Path", static_cast<int>(DownloadMode::CustomPath));

    m_downloadModeLayout->addWidget(m_downloadModeLabel);
    m_downloadModeLayout->addWidget(m_downloadModeComboBox);
    m_downloadModeLayout->addStretch();
    m_downloadMainLayout->addLayout(m_downloadModeLayout);

    // Chapter range control
    m_chapterRangeLayout = new QHBoxLayout();
    m_startChapterLabel = new QLabel("Start Chapter:", m_downloadGroup);
    m_startChapterSpinBox = new QSpinBox(m_downloadGroup);
    m_startChapterSpinBox->setMinimum(1);
    m_startChapterSpinBox->setMaximum(9999);
    m_startChapterSpinBox->setValue(1);

    m_toLabel = new QLabel("to", m_downloadGroup);
    m_endChapterSpinBox = new QSpinBox(m_downloadGroup);
    m_endChapterSpinBox->setMinimum(1);
    m_endChapterSpinBox->setMaximum(9999);
    m_endChapterSpinBox->setValue(100);

    m_chapterRangeLayout->addWidget(m_startChapterLabel);
    m_chapterRangeLayout->addWidget(m_startChapterSpinBox);
    m_chapterRangeLayout->addWidget(m_toLabel);
    m_chapterRangeLayout->addWidget(m_endChapterSpinBox);
    m_chapterRangeLayout->addStretch();
    m_downloadMainLayout->addLayout(m_chapterRangeLayout);

    // Custom path control
    m_customPathLayout = new QHBoxLayout();
    m_customPathLabel = new QLabel("Save Path:", m_downloadGroup);
    m_customPathLineEdit = new QLineEdit(m_downloadGroup);
    m_customPathLineEdit->setPlaceholderText("Select file save path");
    m_browseButton = new QPushButton("Browse", m_downloadGroup);
    m_browseButton->setMaximumWidth(60);

    m_customPathLayout->addWidget(m_customPathLabel);
    m_customPathLayout->addWidget(m_customPathLineEdit, 1);
    m_customPathLayout->addWidget(m_browseButton);
    m_downloadMainLayout->addLayout(m_customPathLayout);

    // Download buttons
    m_downloadButtonLayout = new QHBoxLayout();
    m_downloadButton = new QPushButton("Start Download", m_downloadGroup);
    m_downloadButton->setMinimumHeight(35);
    m_cancelButton = new QPushButton("Cancel Download", m_downloadGroup);
    m_cancelButton->setMinimumHeight(35);
    m_cancelButton->setEnabled(false);

    m_downloadButtonLayout->addWidget(m_downloadButton);
    m_downloadButtonLayout->addWidget(m_cancelButton);
    m_downloadMainLayout->addLayout(m_downloadButtonLayout);

    m_mainLayout->addWidget(m_downloadGroup);

    // Connect signals
    connect(m_downloadModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NovelSearchViewEnhanced::onDownloadModeChanged);
    connect(m_browseButton, &QPushButton::clicked, this, &NovelSearchViewEnhanced::onBrowseButtonClicked);
    connect(m_downloadButton, &QPushButton::clicked, this, &NovelSearchViewEnhanced::onDownloadButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &NovelSearchViewEnhanced::onCancelButtonClicked);
}

void NovelSearchViewEnhanced::setupProgressSection()
{
    m_progressGroup = new QGroupBox("Progress Display", this);
    m_progressLayout = new QVBoxLayout(m_progressGroup);

    // Search progress
    QLabel *searchLabel = new QLabel("Search Progress:", m_progressGroup);
    m_searchProgressBar = new QProgressBar(m_progressGroup);
    m_searchProgressBar->setVisible(true);
    m_searchProgressBar->setRange(0, 1);
    m_searchProgressBar->setValue(0);
    m_searchProgressBar->setFormat("Ready to search...");

    // Download progress
    QLabel *downloadLabel = new QLabel("Download Progress:", m_progressGroup);
    m_downloadProgressBar = new QProgressBar(m_progressGroup);
    m_downloadProgressBar->setVisible(false);

    // Status labels
    m_statusLabel = new QLabel("Ready", m_progressGroup);
    m_statusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; }");

    m_sourceStatusLabel = new QLabel("", m_progressGroup);
    m_sourceStatusLabel->setStyleSheet("QLabel { color: #888; font-size: 11px; }");

    // Layout
    m_progressLayout->addWidget(searchLabel);
    m_progressLayout->addWidget(m_searchProgressBar);
    m_progressLayout->addWidget(downloadLabel);
    m_progressLayout->addWidget(m_downloadProgressBar);
    m_progressLayout->addWidget(m_statusLabel);
    m_progressLayout->addWidget(m_sourceStatusLabel);

    m_mainLayout->addWidget(m_progressGroup);
}

void NovelSearchViewEnhanced::resetUI()
{
    m_isSearching = false;
    m_isDownloading = false;

    if (m_resultsTable) {
        m_resultsTable->setRowCount(0);
    }

    if (m_searchProgressBar) {
        m_searchProgressBar->setRange(0, 1);
        m_searchProgressBar->setValue(0);
        m_searchProgressBar->setFormat("Ready to search...");
    }

    if (m_downloadProgressBar) {
        m_downloadProgressBar->setVisible(false);
        m_downloadProgressBar->setValue(0);
    }

    if (m_statusLabel) {
        m_statusLabel->setText("Ready");
    }

    if (m_sourceStatusLabel) {
        m_sourceStatusLabel->setText("");
    }

    enableControls(true);
    updateDownloadModeUI();
}

void NovelSearchViewEnhanced::enableControls(bool enabled)
{
    if (m_searchButton) m_searchButton->setEnabled(enabled && !m_isDownloading);
    if (m_downloadButton) m_downloadButton->setEnabled(enabled && !m_isSearching && !m_selectedResult.bookName().isEmpty());
    if (m_cancelButton) m_cancelButton->setEnabled(m_isDownloading);
    if (m_sourceComboBox) m_sourceComboBox->setEnabled(enabled && !m_isSearching && !m_isDownloading);
    if (m_downloadModeComboBox) m_downloadModeComboBox->setEnabled(enabled && !m_isDownloading);
}

void NovelSearchViewEnhanced::updateDownloadModeUI()
{
    if (!m_downloadModeComboBox) return;

    DownloadMode mode = static_cast<DownloadMode>(m_downloadModeComboBox->currentData().toInt());

    // Show/hide chapter range controls
    bool showChapterRange = (mode == DownloadMode::ChapterRange);
    if (m_startChapterLabel) m_startChapterLabel->setVisible(showChapterRange);
    if (m_startChapterSpinBox) m_startChapterSpinBox->setVisible(showChapterRange);
    if (m_toLabel) m_toLabel->setVisible(showChapterRange);
    if (m_endChapterSpinBox) m_endChapterSpinBox->setVisible(showChapterRange);

    // Show/hide custom path controls
    bool showCustomPath = (mode == DownloadMode::CustomPath);
    if (m_customPathLabel) m_customPathLabel->setVisible(showCustomPath);
    if (m_customPathLineEdit) m_customPathLineEdit->setVisible(showCustomPath);
    if (m_browseButton) m_browseButton->setVisible(showCustomPath);
}

// Slot function implementations
void NovelSearchViewEnhanced::onSearchButtonClicked()
{
    if (!m_searchLineEdit || m_searchLineEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Notice", "Please enter search keyword");
        return;
    }

    QString keyword = m_searchLineEdit->text().trimmed();
    int sourceId = m_sourceComboBox ? m_sourceComboBox->currentData().toInt() : -1;

    qDebug() << "Search requested:" << keyword << "Source:" << sourceId;
    emit searchRequested(keyword, sourceId);
}

void NovelSearchViewEnhanced::onDownloadButtonClicked()
{
    if (m_selectedResult.bookName().isEmpty()) {
        QMessageBox::warning(this, "Notice", "Please select a novel to download first");
        return;
    }

    if (!m_downloadModeComboBox) return;

    DownloadMode mode = static_cast<DownloadMode>(m_downloadModeComboBox->currentData().toInt());
    int startChapter = 1;
    int endChapter = -1; // -1 means all chapters
    QString customPath;

    switch (mode) {
    case DownloadMode::ChapterRange:
        if (m_startChapterSpinBox && m_endChapterSpinBox) {
            startChapter = m_startChapterSpinBox->value();
            endChapter = m_endChapterSpinBox->value();

            if (startChapter > endChapter) {
                QMessageBox::warning(this, "Notice", "Start chapter cannot be greater than end chapter");
                return;
            }
        }
        break;

    case DownloadMode::FullNovel:
        startChapter = 1;
        endChapter = -1;
        break;

    case DownloadMode::CustomPath:
        if (m_customPathLineEdit) {
            customPath = m_customPathLineEdit->text().trimmed();
            if (customPath.isEmpty()) {
                QMessageBox::warning(this, "Notice", "Please select save path");
                return;
            }
        }
        break;
    }

    qDebug() << "Download requested:" << m_selectedResult.bookName()
             << "Mode:" << static_cast<int>(mode)
             << "Range:" << startChapter << "-" << endChapter
             << "Path:" << customPath;

    emit downloadRequested(m_selectedResult, startChapter, endChapter, static_cast<int>(mode), customPath);
}

void NovelSearchViewEnhanced::onCancelButtonClicked()
{
    // TODO: Implement cancel download function
    qDebug() << "Cancel download requested";
}

void NovelSearchViewEnhanced::onSettingsButtonClicked()
{
    qDebug() << "Settings requested";

    if (m_settingsDialog) {
        m_settingsDialog->show();
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
    } else {
        qDebug() << "Settings dialog not initialized";
    }

    emit settingsRequested();
}

void NovelSearchViewEnhanced::onBrowseButtonClicked()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (m_novelConfig) {
        QString configPath = m_novelConfig->getDownloadPath();
        if (!configPath.isEmpty()) {
            defaultPath = configPath;
        }
    }

    QString selectedPath = QFileDialog::getExistingDirectory(this, "Select Save Path", defaultPath);
    if (!selectedPath.isEmpty() && m_customPathLineEdit) {
        m_customPathLineEdit->setText(selectedPath);
    }
}

void NovelSearchViewEnhanced::onDownloadModeChanged(int index)
{
    Q_UNUSED(index)
    updateDownloadModeUI();
}

void NovelSearchViewEnhanced::onResultSelectionChanged()
{
    if (!m_resultsTable) return;

    int currentRow = m_resultsTable->currentRow();
    if (currentRow >= 0 && currentRow < m_currentResults.size()) {
        m_selectedResult = m_currentResults[currentRow];
        qDebug() << "Selected result:" << m_selectedResult.bookName();
        emit searchResultSelected(m_selectedResult);
    } else {
        m_selectedResult = SearchResult();
    }

    enableControls(true);
}

// Public slot function implementations
void NovelSearchViewEnhanced::onSearchStarted(const QString &keyword)
{
    m_isSearching = true;
    m_currentResults.clear();

    if (m_resultsTable) {
        m_resultsTable->setRowCount(0);
    }

    if (m_searchProgressBar) {
        m_searchProgressBar->setVisible(true);
        m_searchProgressBar->setRange(0, 0); // Start with indeterminate progress
        m_searchProgressBar->setValue(0);
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Searching: %1").arg(keyword));
    }

    enableControls(false);

    qDebug() << "Search started for:" << keyword;
}

void NovelSearchViewEnhanced::onSearchProgress(const QString &status, int current, int total)
{
    qDebug() << "UI: Search progress received:" << status << "(" << current << "/" << total << ")";

    if (m_searchProgressBar) {
        if (total > 0) {
            // Set determinate progress for sequential search
            m_searchProgressBar->setRange(0, total);
            m_searchProgressBar->setValue(current);

            // Calculate percentage
            int percentage = (current * 100) / total;
            qDebug() << "UI: Progress bar updated - range:" << 0 << "to" << total << "value:" << current << "percentage:" << percentage << "%";

            // Update progress bar format to show percentage
            m_searchProgressBar->setFormat(QString("Searching... %1/%2 (%3%)").arg(current).arg(total).arg(percentage));
        } else {
            // Set indeterminate progress for unknown total
            m_searchProgressBar->setRange(0, 0);
            m_searchProgressBar->setFormat("Searching...");
            qDebug() << "UI: Progress bar set to indeterminate mode";
        }
        m_searchProgressBar->setVisible(true);
    } else {
        qDebug() << "UI: WARNING - m_searchProgressBar is null!";
    }

    if (m_statusLabel) {
        m_statusLabel->setText(status);
        qDebug() << "UI: Status label updated to:" << status;
    } else {
        qDebug() << "UI: WARNING - m_statusLabel is null!";
    }
}

void NovelSearchViewEnhanced::onSearchResultsUpdated(const QList<SearchResult> &results, int sourceId)
{
    qDebug() << "Real-time results update: received" << results.size() << "results from source" << sourceId;

    // Add new results to current results list
    m_currentResults.append(results);

    // Update results table with new results
    if (m_resultsTable && !results.isEmpty()) {
        int currentRowCount = m_resultsTable->rowCount();
        m_resultsTable->setRowCount(currentRowCount + results.size());

        for (int i = 0; i < results.size(); ++i) {
            const SearchResult &result = results[i];
            int row = currentRowCount + i;

            m_resultsTable->setItem(row, 0, new QTableWidgetItem(result.bookName()));
            m_resultsTable->setItem(row, 1, new QTableWidgetItem(result.author()));
            m_resultsTable->setItem(row, 2, new QTableWidgetItem(result.latestChapter()));
            m_resultsTable->setItem(row, 3, new QTableWidgetItem(result.sourceName()));
        }
    }

    // Update status label with current progress
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Found %1 results from source %2 (Total: %3)")
                              .arg(results.size())
                              .arg(sourceId)
                              .arg(m_currentResults.size()));
    }

    qDebug() << "Real-time update completed, total results:" << m_currentResults.size();
}

void NovelSearchViewEnhanced::onSearchCompleted(const QList<SearchResult> &results)
{
    m_isSearching = false;

    // For sequential search, results are already accumulated via onSearchResultsUpdated
    // So we only need to update the final status and enable controls
    if (m_currentResults.isEmpty()) {
        // Fallback for non-sequential search or if no real-time updates occurred
        m_currentResults = results;
    }

    if (m_searchProgressBar) {
        m_searchProgressBar->setRange(0, 1);
        m_searchProgressBar->setValue(1);
        m_searchProgressBar->setFormat(QString("Search completed - %1 results found").arg(m_currentResults.size()));
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Search completed, found %1 results").arg(m_currentResults.size()));
    }

    // For non-sequential search, update results table if needed
    if (m_resultsTable && m_currentResults.isEmpty() && !results.isEmpty()) {
        m_resultsTable->setRowCount(results.size());

        for (int i = 0; i < results.size(); ++i) {
            const SearchResult &result = results[i];

            m_resultsTable->setItem(i, 0, new QTableWidgetItem(result.bookName()));
            m_resultsTable->setItem(i, 1, new QTableWidgetItem(result.author()));
            m_resultsTable->setItem(i, 2, new QTableWidgetItem(result.latestChapter()));
            m_resultsTable->setItem(i, 3, new QTableWidgetItem(result.sourceName()));
        }
    }

    enableControls(true);

    qDebug() << "Search completed with" << m_currentResults.size() << "total results";
}

void NovelSearchViewEnhanced::onSearchFailed(const QString &error)
{
    m_isSearching = false;

    if (m_searchProgressBar) {
        m_searchProgressBar->setRange(0, 1);
        m_searchProgressBar->setValue(0);
        m_searchProgressBar->setFormat(QString("Search failed: %1").arg(error));
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Search failed: %1").arg(error));
    }

    enableControls(true);

    QMessageBox::warning(this, "Search Failed", error);

    qDebug() << "Search failed:" << error;
}

void NovelSearchViewEnhanced::onDownloadStarted(const SearchResult &result)
{
    m_isDownloading = true;

    if (m_downloadProgressBar) {
        m_downloadProgressBar->setVisible(true);
        m_downloadProgressBar->setRange(0, 100);
        m_downloadProgressBar->setValue(0);
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Starting download: %1").arg(result.bookName()));
    }

    enableControls(false);

    qDebug() << "Download started for:" << result.bookName();
}

void NovelSearchViewEnhanced::onDownloadProgress(const QString &status, int current, int total)
{
    if (m_downloadProgressBar) {
        if (total > 0) {
            m_downloadProgressBar->setRange(0, total);
            m_downloadProgressBar->setValue(current);
        }
    }

    if (m_statusLabel) {
        if (total > 0) {
            m_statusLabel->setText(QString("%1 (%2/%3)").arg(status).arg(current).arg(total));
        } else {
            m_statusLabel->setText(status);
        }
    }

    qDebug() << "Download progress:" << status << current << "/" << total;
}

void NovelSearchViewEnhanced::onDownloadCompleted(const QString &filePath)
{
    m_isDownloading = false;

    if (m_downloadProgressBar) {
        m_downloadProgressBar->setVisible(false);
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Download completed: %1").arg(filePath));
    }

    enableControls(true);

    QMessageBox::information(this, "Download Complete", QString("File saved to: %1").arg(filePath));

    qDebug() << "Download completed:" << filePath;
}

void NovelSearchViewEnhanced::onDownloadFailed(const QString &error)
{
    m_isDownloading = false;

    if (m_downloadProgressBar) {
        m_downloadProgressBar->setVisible(false);
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QString("Download failed: %1").arg(error));
    }

    enableControls(true);

    QMessageBox::warning(this, "Download Failed", error);

    qDebug() << "Download failed:" << error;
}