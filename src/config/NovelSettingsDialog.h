#ifndef NOVELSETTINGSDIALOG_H
#define NOVELSETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

// Forward declarations
class NovelConfig;

/**
 * @brief NovelSettingsDialog类 - 小说搜索专用设置对话框
 * 
 * 参考现有SettingsDialog的实现模式，提供书源选择、下载路径配置、
 * 爬取限制参数、代理设置等配置选项
 */
class NovelSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NovelSettingsDialog(NovelConfig* config, QWidget *parent = nullptr);
    ~NovelSettingsDialog();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void onApplyButtonClicked();
    void onCancelButtonClicked();
    void onResetButtonClicked();
    void onBrowseActiveRulesClicked();
    void onBrowseDownloadPathClicked();
    void onTestProxyClicked();

private:
    void setupUI();
    void setupBasicTab();
    void setupCrawlTab();
    void setupProxyTab();
    void setupButtonBox();
    void setupChangeDetection();
    
    void loadSettings();
    void applySettings();
    void resetToDefaults();
    void restoreOriginalSettings();
    bool validateSettings();

    // Enhanced UI behavior methods
    bool hasUnsavedChanges() const;
    void markAsModified();
    void markAsApplied();
    bool askUserToSaveChanges();
    
    // UI Components
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;
    
    // Tab 1: 基础设置
    QWidget *m_basicTab;
    QFormLayout *m_basicLayout;
    QComboBox *m_languageComboBox;
    QLineEdit *m_activeRulesLineEdit;
    QPushButton *m_browseActiveRulesButton;
    QLineEdit *m_downloadPathLineEdit;
    QPushButton *m_browseDownloadPathButton;
    QComboBox *m_extNameComboBox;
    QSpinBox *m_sourceIdSpinBox;
    QSpinBox *m_searchLimitSpinBox;
    QCheckBox *m_autoUpdateCheckBox;
    
    // Tab 2: 爬取限制
    QWidget *m_crawlTab;
    QFormLayout *m_crawlLayout;
    QSpinBox *m_crawlMinIntervalSpinBox;
    QSpinBox *m_crawlMaxIntervalSpinBox;
    QSpinBox *m_crawlThreadsSpinBox;
    QCheckBox *m_preserveChapterCacheCheckBox;
    QSpinBox *m_maxRetryAttemptsSpinBox;
    QSpinBox *m_retryMinIntervalSpinBox;
    QSpinBox *m_retryMaxIntervalSpinBox;
    
    // Tab 3: 代理设置
    QWidget *m_proxyTab;
    QFormLayout *m_proxyLayout;
    QCheckBox *m_proxyEnabledCheckBox;
    QLineEdit *m_proxyHostLineEdit;
    QSpinBox *m_proxyPortSpinBox;
    QPushButton *m_testProxyButton;
    QLabel *m_proxyStatusLabel;
    
    // Button Box
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_applyButton;
    QPushButton *m_cancelButton;
    QPushButton *m_resetButton;
    
    // Data
    NovelConfig *m_novelConfig;
    
    // Original settings for cancel operation
    struct OriginalSettings {
        QString language;
        QString activeRules;
        QString downloadPath;
        QString extName;
        int sourceId;
        int searchLimit;
        bool autoUpdate;
        
        int crawlMinInterval;
        int crawlMaxInterval;
        int crawlThreads;
        bool preserveChapterCache;
        
        int maxRetryAttempts;
        int retryMinInterval;
        int retryMaxInterval;
        
        bool proxyEnabled;
        QString proxyHost;
        int proxyPort;
    } m_originalSettings;

    // State tracking for enhanced UI behavior
    bool m_hasUnsavedChanges;
    bool m_hasAppliedChanges;
};

#endif // NOVELSETTINGSDIALOG_H
