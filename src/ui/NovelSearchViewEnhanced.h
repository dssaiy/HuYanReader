#ifndef NOVELSEARCHVIEWENHANCED_H
#define NOVELSEARCHVIEWENHANCED_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QSpinBox>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QFrame>
#include <QFileDialog>
#include "../novel/NovelModels.h"

// Forward declarations
class NovelSearchManager;
class NovelConfig;
class NovelSettingsDialog;

/**
 * @brief Enhanced novel search interface class
 *
 * Inherits NovelSearchViewSimple, adds complete download control interface
 * Includes chapter range selection, download mode selection, custom path settings, progress display and settings button components
 */
class NovelSearchViewEnhanced : public QWidget
{
    Q_OBJECT

public:
    // Download mode enumeration
    enum class DownloadMode {
        ChapterRange,  // Download by chapter range
        FullNovel,     // Full novel download
        CustomPath     // Custom path download
    };

    explicit NovelSearchViewEnhanced(QWidget *parent = nullptr);
    ~NovelSearchViewEnhanced();

    // Set configuration manager
    void setNovelConfig(NovelConfig* config);

signals:
    void searchRequested(const QString &keyword, int sourceId);
    void downloadRequested(const SearchResult &result, int startChapter, int endChapter, int mode, const QString &customPath);
    void sourceChanged(int sourceId);
    void searchResultSelected(const SearchResult &result);
    void settingsRequested();

public slots:
    void onSearchStarted(const QString &keyword);
    void onSearchProgress(const QString &status, int current, int total);
    void onSearchCompleted(const QList<SearchResult> &results);
    void onSearchResultsUpdated(const QList<SearchResult> &results, int sourceId);
    void onSearchFailed(const QString &error);
    void onDownloadStarted(const SearchResult &result);
    void onDownloadProgress(const QString &status, int current, int total);
    void onDownloadCompleted(const QString &filePath);
    void onDownloadFailed(const QString &error);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onSearchButtonClicked();
    void onDownloadButtonClicked();
    void onCancelButtonClicked();
    void onSettingsButtonClicked();
    void onBrowseButtonClicked();
    void onDownloadModeChanged(int index);
    void onResultSelectionChanged();

private:
    void setupEnhancedUI();
    void setupSearchSection();
    void setupResultsSection();
    void setupDownloadSection();
    void setupProgressSection();
    void resetUI();
    void enableControls(bool enabled);
    void updateDownloadModeUI();
    
    // UI Components - Enhanced Layout
    QVBoxLayout *m_mainLayout;
    
    // Search section
    QGroupBox *m_searchGroup;
    QHBoxLayout *m_searchLayout;
    QComboBox *m_sourceComboBox;
    QLineEdit *m_searchLineEdit;
    QPushButton *m_searchButton;
    QPushButton *m_settingsButton;
    
    // Results section
    QGroupBox *m_resultsGroup;
    QVBoxLayout *m_resultsLayout;
    QTableWidget *m_resultsTable;
    
    // Download section
    QGroupBox *m_downloadGroup;
    QVBoxLayout *m_downloadMainLayout;
    
    // Download mode selection
    QHBoxLayout *m_downloadModeLayout;
    QLabel *m_downloadModeLabel;
    QComboBox *m_downloadModeComboBox;
    
    // Chapter range controls
    QHBoxLayout *m_chapterRangeLayout;
    QLabel *m_startChapterLabel;
    QSpinBox *m_startChapterSpinBox;
    QLabel *m_toLabel;
    QSpinBox *m_endChapterSpinBox;
    
    // Custom path controls
    QHBoxLayout *m_customPathLayout;
    QLabel *m_customPathLabel;
    QLineEdit *m_customPathLineEdit;
    QPushButton *m_browseButton;
    
    // Download buttons
    QHBoxLayout *m_downloadButtonLayout;
    QPushButton *m_downloadButton;
    QPushButton *m_cancelButton;
    
    // Progress section
    QGroupBox *m_progressGroup;
    QVBoxLayout *m_progressLayout;
    QProgressBar *m_searchProgressBar;
    QProgressBar *m_downloadProgressBar;
    QLabel *m_statusLabel;
    QLabel *m_sourceStatusLabel;
    
    // Data
    QList<SearchResult> m_currentResults;
    SearchResult m_selectedResult;
    bool m_isSearching;
    bool m_isDownloading;
    
    // Configuration
    NovelConfig *m_novelConfig;
    NovelSettingsDialog *m_settingsDialog;
};

#endif // NOVELSEARCHVIEWENHANCED_H
