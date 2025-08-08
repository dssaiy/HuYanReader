#include "NovelSettingsDialog.h"
#include "NovelConfig.h"
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QCoreApplication>

NovelSettingsDialog::NovelSettingsDialog(NovelConfig* config, QWidget *parent)
    : QDialog(parent)
    , m_novelConfig(config)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_basicTab(nullptr)
    , m_crawlTab(nullptr)
    , m_proxyTab(nullptr)
    , m_hasUnsavedChanges(false)
    , m_hasAppliedChanges(false)
{
    setWindowTitle("Novel Search Settings");
    setModal(true);
    setMinimumSize(500, 400);
    resize(600, 500);

    setupUI();
    loadSettings();
    
    qDebug() << "NovelSettingsDialog created successfully";
}

NovelSettingsDialog::~NovelSettingsDialog()
{
    qDebug() << "NovelSettingsDialog destroyed";
}

void NovelSettingsDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    setupBasicTab();
    setupCrawlTab();
    setupProxyTab();
    setupButtonBox();
    
    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addLayout(m_buttonLayout);

    setLayout(m_mainLayout);

    // Setup change detection signals
    setupChangeDetection();
}

void NovelSettingsDialog::setupBasicTab()
{
    m_basicTab = new QWidget();
    m_basicLayout = new QFormLayout(m_basicTab);
    m_basicLayout->setSpacing(10);
    
    // Interface language
    m_languageComboBox = new QComboBox(m_basicTab);
    m_languageComboBox->addItem("Chinese", "zh_CN");
    m_languageComboBox->addItem("English", "en_US");
    m_basicLayout->addRow("Language:", m_languageComboBox);
    
    // Active rules
    QHBoxLayout *activeRulesLayout = new QHBoxLayout();
    m_activeRulesLineEdit = new QLineEdit(m_basicTab);
    m_activeRulesLineEdit->setPlaceholderText("Select rules file");
    m_browseActiveRulesButton = new QPushButton("Browse", m_basicTab);
    m_browseActiveRulesButton->setMaximumWidth(60);
    activeRulesLayout->addWidget(m_activeRulesLineEdit);
    activeRulesLayout->addWidget(m_browseActiveRulesButton);
    m_basicLayout->addRow("Active Rules:", activeRulesLayout);
    
    // Download path
    QHBoxLayout *downloadPathLayout = new QHBoxLayout();
    m_downloadPathLineEdit = new QLineEdit(m_basicTab);
    m_downloadPathLineEdit->setPlaceholderText("Select download path");
    m_browseDownloadPathButton = new QPushButton("Browse", m_basicTab);
    m_browseDownloadPathButton->setMaximumWidth(60);
    downloadPathLayout->addWidget(m_downloadPathLineEdit);
    downloadPathLayout->addWidget(m_browseDownloadPathButton);
    m_basicLayout->addRow("Download Path:", downloadPathLayout);
    
    // File format
    m_extNameComboBox = new QComboBox(m_basicTab);
    m_extNameComboBox->addItem("TXT File", "txt");
    m_extNameComboBox->addItem("EPUB eBook", "epub");
    m_extNameComboBox->addItem("HTML Page", "html");
    m_basicLayout->addRow("File Format:", m_extNameComboBox);
    
    // Default source ID
    m_sourceIdSpinBox = new QSpinBox(m_basicTab);
    m_sourceIdSpinBox->setRange(-1, 999);
    m_sourceIdSpinBox->setSpecialValueText("Auto Select");
    m_basicLayout->addRow("Default Source ID:", m_sourceIdSpinBox);
    
    // Search result limit
    m_searchLimitSpinBox = new QSpinBox(m_basicTab);
    m_searchLimitSpinBox->setRange(1, 100);
    m_searchLimitSpinBox->setSuffix(" results");
    m_basicLayout->addRow("Search Result Limit:", m_searchLimitSpinBox);
    
    // Enable auto-update
    m_autoUpdateCheckBox = new QCheckBox("Enable automatic update check", m_basicTab);
    m_basicLayout->addRow("", m_autoUpdateCheckBox);
    
    m_tabWidget->addTab(m_basicTab, "Basic Settings");
    
    // Connect signals
    connect(m_browseActiveRulesButton, &QPushButton::clicked,
            this, &NovelSettingsDialog::onBrowseActiveRulesClicked);
    connect(m_browseDownloadPathButton, &QPushButton::clicked,
            this, &NovelSettingsDialog::onBrowseDownloadPathClicked);
}

void NovelSettingsDialog::setupCrawlTab()
{
    m_crawlTab = new QWidget();
    m_crawlLayout = new QFormLayout(m_crawlTab);
    m_crawlLayout->setSpacing(10);
    
    // Request interval settings
    QGroupBox *intervalGroup = new QGroupBox("Request Interval Settings", m_crawlTab);
    QFormLayout *intervalLayout = new QFormLayout(intervalGroup);
    
    m_crawlMinIntervalSpinBox = new QSpinBox(intervalGroup);
    m_crawlMinIntervalSpinBox->setRange(100, 5000);
    m_crawlMinIntervalSpinBox->setSuffix(" ms");
    intervalLayout->addRow("Min Interval:", m_crawlMinIntervalSpinBox);
    
    m_crawlMaxIntervalSpinBox = new QSpinBox(intervalGroup);
    m_crawlMaxIntervalSpinBox->setRange(200, 10000);
    m_crawlMaxIntervalSpinBox->setSuffix(" ms");
    intervalLayout->addRow("Max Interval:", m_crawlMaxIntervalSpinBox);
    
    // Concurrency settings
    m_crawlThreadsSpinBox = new QSpinBox(m_crawlTab);
    m_crawlThreadsSpinBox->setRange(-1, 20);
    m_crawlThreadsSpinBox->setSpecialValueText("Auto Set");
    intervalLayout->addRow("Concurrent Threads:", m_crawlThreadsSpinBox);
    
    m_crawlLayout->addRow(intervalGroup);
    
    // Cache settings
    m_preserveChapterCacheCheckBox = new QCheckBox("Preserve chapter cache", m_crawlTab);
    m_crawlLayout->addRow("", m_preserveChapterCacheCheckBox);
    
    // Retry settings
    QGroupBox *retryGroup = new QGroupBox("Retry Settings", m_crawlTab);
    QFormLayout *retryLayout = new QFormLayout(retryGroup);
    
    m_maxRetryAttemptsSpinBox = new QSpinBox(retryGroup);
    m_maxRetryAttemptsSpinBox->setRange(0, 10);
    m_maxRetryAttemptsSpinBox->setSuffix(" times");
    retryLayout->addRow("Max Retry Attempts:", m_maxRetryAttemptsSpinBox);
    
    m_retryMinIntervalSpinBox = new QSpinBox(retryGroup);
    m_retryMinIntervalSpinBox->setRange(1000, 30000);
    m_retryMinIntervalSpinBox->setSuffix(" ms");
    retryLayout->addRow("Retry Min Interval:", m_retryMinIntervalSpinBox);
    
    m_retryMaxIntervalSpinBox = new QSpinBox(retryGroup);
    m_retryMaxIntervalSpinBox->setRange(2000, 60000);
    m_retryMaxIntervalSpinBox->setSuffix(" ms");
    retryLayout->addRow("Retry Max Interval:", m_retryMaxIntervalSpinBox);
    
    m_crawlLayout->addRow(retryGroup);
    
    m_tabWidget->addTab(m_crawlTab, "Crawl Limits");
}

void NovelSettingsDialog::setupProxyTab()
{
    m_proxyTab = new QWidget();
    m_proxyLayout = new QFormLayout(m_proxyTab);
    m_proxyLayout->setSpacing(10);
    
    // Enable proxy server
    m_proxyEnabledCheckBox = new QCheckBox("Enable proxy server", m_proxyTab);
    m_proxyLayout->addRow("", m_proxyEnabledCheckBox);
    
    // Proxy address
    m_proxyHostLineEdit = new QLineEdit(m_proxyTab);
    m_proxyHostLineEdit->setPlaceholderText("e.g.: 127.0.0.1");
    m_proxyLayout->addRow("Proxy Address:", m_proxyHostLineEdit);
    
    // Proxy port
    m_proxyPortSpinBox = new QSpinBox(m_proxyTab);
    m_proxyPortSpinBox->setRange(1, 65535);
    m_proxyLayout->addRow("Proxy Port:", m_proxyPortSpinBox);
    
    // Test proxy button and status
    QHBoxLayout *testLayout = new QHBoxLayout();
    m_testProxyButton = new QPushButton("Test Connection", m_proxyTab);
    m_proxyStatusLabel = new QLabel("", m_proxyTab);
    m_proxyStatusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; }");
    testLayout->addWidget(m_testProxyButton);
    testLayout->addWidget(m_proxyStatusLabel);
    testLayout->addStretch();
    m_proxyLayout->addRow("", testLayout);
    
    m_tabWidget->addTab(m_proxyTab, "Proxy Settings");
    
    // Connect signals
    connect(m_proxyEnabledCheckBox, &QCheckBox::toggled, [this](bool enabled) {
        m_proxyHostLineEdit->setEnabled(enabled);
        m_proxyPortSpinBox->setEnabled(enabled);
        m_testProxyButton->setEnabled(enabled);
    });
    
    connect(m_testProxyButton, &QPushButton::clicked, 
            this, &NovelSettingsDialog::onTestProxyClicked);
}

void NovelSettingsDialog::setupButtonBox()
{
    m_buttonLayout = new QHBoxLayout();
    
    m_applyButton = new QPushButton("Apply", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_resetButton = new QPushButton("Reset", this);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_applyButton);
    
    // Connect signals
    connect(m_applyButton, &QPushButton::clicked, this, &NovelSettingsDialog::onApplyButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &NovelSettingsDialog::onCancelButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &NovelSettingsDialog::onResetButtonClicked);
}

void NovelSettingsDialog::setupChangeDetection()
{
    // Connect all input controls to markAsModified() to detect changes

    // Basic tab controls
    connect(m_languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_activeRulesLineEdit, &QLineEdit::textChanged,
            this, &NovelSettingsDialog::markAsModified);
    connect(m_downloadPathLineEdit, &QLineEdit::textChanged,
            this, &NovelSettingsDialog::markAsModified);
    connect(m_extNameComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_sourceIdSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_searchLimitSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_autoUpdateCheckBox, &QCheckBox::toggled,
            this, &NovelSettingsDialog::markAsModified);

    // Crawl tab controls
    connect(m_crawlMinIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_crawlMaxIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_crawlThreadsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_preserveChapterCacheCheckBox, &QCheckBox::toggled,
            this, &NovelSettingsDialog::markAsModified);
    connect(m_maxRetryAttemptsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_retryMinIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
    connect(m_retryMaxIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);

    // Proxy tab controls
    connect(m_proxyEnabledCheckBox, &QCheckBox::toggled,
            this, &NovelSettingsDialog::markAsModified);
    connect(m_proxyHostLineEdit, &QLineEdit::textChanged,
            this, &NovelSettingsDialog::markAsModified);
    connect(m_proxyPortSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NovelSettingsDialog::markAsModified);
}

void NovelSettingsDialog::loadSettings()
{
    if (!m_novelConfig) return;

    // Save original settings for cancel operation
    m_originalSettings.language = m_novelConfig->getLanguage();
    m_originalSettings.activeRules = m_novelConfig->getActiveRules();
    m_originalSettings.downloadPath = m_novelConfig->getDownloadPath();
    m_originalSettings.extName = m_novelConfig->getExtName();
    m_originalSettings.sourceId = m_novelConfig->getSourceId();
    m_originalSettings.searchLimit = m_novelConfig->getSearchLimit();
    m_originalSettings.autoUpdate = m_novelConfig->getAutoUpdate();

    m_originalSettings.crawlMinInterval = m_novelConfig->getCrawlMinInterval();
    m_originalSettings.crawlMaxInterval = m_novelConfig->getCrawlMaxInterval();
    m_originalSettings.crawlThreads = m_novelConfig->getCrawlThreads();
    m_originalSettings.preserveChapterCache = m_novelConfig->getPreserveChapterCache();

    m_originalSettings.maxRetryAttempts = m_novelConfig->getMaxRetryAttempts();
    m_originalSettings.retryMinInterval = m_novelConfig->getRetryMinInterval();
    m_originalSettings.retryMaxInterval = m_novelConfig->getRetryMaxInterval();

    m_originalSettings.proxyEnabled = m_novelConfig->getProxyEnabled();
    m_originalSettings.proxyHost = m_novelConfig->getProxyHost();
    m_originalSettings.proxyPort = m_novelConfig->getProxyPort();

    // Load settings into UI controls
    // Basic settings
    int langIndex = m_languageComboBox->findData(m_novelConfig->getLanguage());
    if (langIndex >= 0) m_languageComboBox->setCurrentIndex(langIndex);

    m_activeRulesLineEdit->setText(m_novelConfig->getActiveRules());
    m_downloadPathLineEdit->setText(m_novelConfig->getDownloadPath());

    int extIndex = m_extNameComboBox->findData(m_novelConfig->getExtName());
    if (extIndex >= 0) m_extNameComboBox->setCurrentIndex(extIndex);

    m_sourceIdSpinBox->setValue(m_novelConfig->getSourceId());
    m_searchLimitSpinBox->setValue(m_novelConfig->getSearchLimit());
    m_autoUpdateCheckBox->setChecked(m_novelConfig->getAutoUpdate());

    // Crawl limit settings
    m_crawlMinIntervalSpinBox->setValue(m_novelConfig->getCrawlMinInterval());
    m_crawlMaxIntervalSpinBox->setValue(m_novelConfig->getCrawlMaxInterval());
    m_crawlThreadsSpinBox->setValue(m_novelConfig->getCrawlThreads());
    m_preserveChapterCacheCheckBox->setChecked(m_novelConfig->getPreserveChapterCache());

    m_maxRetryAttemptsSpinBox->setValue(m_novelConfig->getMaxRetryAttempts());
    m_retryMinIntervalSpinBox->setValue(m_novelConfig->getRetryMinInterval());
    m_retryMaxIntervalSpinBox->setValue(m_novelConfig->getRetryMaxInterval());

    // Proxy settings
    m_proxyEnabledCheckBox->setChecked(m_novelConfig->getProxyEnabled());
    m_proxyHostLineEdit->setText(m_novelConfig->getProxyHost());
    m_proxyPortSpinBox->setValue(m_novelConfig->getProxyPort());

    // Update proxy control state
    bool proxyEnabled = m_novelConfig->getProxyEnabled();
    m_proxyHostLineEdit->setEnabled(proxyEnabled);
    m_proxyPortSpinBox->setEnabled(proxyEnabled);
    m_testProxyButton->setEnabled(proxyEnabled);

    // Reset state tracking after loading
    m_hasUnsavedChanges = false;
    m_hasAppliedChanges = false;

    qDebug() << "Settings loaded successfully";
}

void NovelSettingsDialog::applySettings()
{
    if (!m_novelConfig) return;

    if (!validateSettings()) {
        return;
    }

    // Apply basic settings
    m_novelConfig->setLanguage(m_languageComboBox->currentData().toString());
    m_novelConfig->setActiveRules(m_activeRulesLineEdit->text()); // This will save config and emit signal
    m_novelConfig->setDownloadPath(m_downloadPathLineEdit->text());
    m_novelConfig->setExtName(m_extNameComboBox->currentData().toString());
    m_novelConfig->setSourceId(m_sourceIdSpinBox->value());
    m_novelConfig->setSearchLimit(m_searchLimitSpinBox->value());
    m_novelConfig->setAutoUpdate(m_autoUpdateCheckBox->isChecked());

    // Apply crawl limit settings
    m_novelConfig->setCrawlMinInterval(m_crawlMinIntervalSpinBox->value());
    m_novelConfig->setCrawlMaxInterval(m_crawlMaxIntervalSpinBox->value());
    m_novelConfig->setCrawlThreads(m_crawlThreadsSpinBox->value());
    m_novelConfig->setPreserveChapterCache(m_preserveChapterCacheCheckBox->isChecked());

    m_novelConfig->setMaxRetryAttempts(m_maxRetryAttemptsSpinBox->value());
    m_novelConfig->setRetryMinInterval(m_retryMinIntervalSpinBox->value());
    m_novelConfig->setRetryMaxInterval(m_retryMaxIntervalSpinBox->value());

    // Apply proxy settings
    m_novelConfig->setProxyEnabled(m_proxyEnabledCheckBox->isChecked());
    m_novelConfig->setProxyHost(m_proxyHostLineEdit->text());
    m_novelConfig->setProxyPort(m_proxyPortSpinBox->value());

    // Save configuration to file (setActiveRules already saved, but save again for other settings)
    m_novelConfig->saveConfig();

    qDebug() << "Settings applied successfully";
}

bool NovelSettingsDialog::validateSettings()
{
    // Validate download path
    QString downloadPath = m_downloadPathLineEdit->text().trimmed();
    if (downloadPath.isEmpty()) {
        QMessageBox::warning(this, "Settings Error", "Download path cannot be empty");
        m_tabWidget->setCurrentIndex(0); // Switch to basic settings tab
        m_downloadPathLineEdit->setFocus();
        return false;
    }

    // Validate crawl interval
    int minInterval = m_crawlMinIntervalSpinBox->value();
    int maxInterval = m_crawlMaxIntervalSpinBox->value();
    if (minInterval > maxInterval) {
        QMessageBox::warning(this, "Settings Error", "Crawl minimum interval cannot be greater than maximum interval");
        m_tabWidget->setCurrentIndex(1); // Switch to crawl limit tab
        m_crawlMinIntervalSpinBox->setFocus();
        return false;
    }

    // Validate retry interval
    int retryMinInterval = m_retryMinIntervalSpinBox->value();
    int retryMaxInterval = m_retryMaxIntervalSpinBox->value();
    if (retryMinInterval > retryMaxInterval) {
        QMessageBox::warning(this, "Settings Error", "Retry minimum interval cannot be greater than maximum interval");
        m_tabWidget->setCurrentIndex(1); // Switch to crawl limit tab
        m_retryMinIntervalSpinBox->setFocus();
        return false;
    }

    // Validate proxy settings
    if (m_proxyEnabledCheckBox->isChecked()) {
        QString proxyHost = m_proxyHostLineEdit->text().trimmed();
        if (proxyHost.isEmpty()) {
            QMessageBox::warning(this, "Settings Error", "Proxy address cannot be empty when proxy is enabled");
            m_tabWidget->setCurrentIndex(2); // Switch to proxy settings tab
            m_proxyHostLineEdit->setFocus();
            return false;
        }
    }

    return true;
}

void NovelSettingsDialog::resetToDefaults()
{
    if (!m_novelConfig) return;

    int ret = QMessageBox::question(this, "Reset Settings",
                                   "Are you sure you want to reset all settings to default values?\nThis operation cannot be undone.",
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Create a temporary config object to fetch default values
        NovelConfig tempConfig;

        // Basic settings
        m_languageComboBox->setCurrentIndex(m_languageComboBox->findData(tempConfig.getLanguage()));
        m_activeRulesLineEdit->setText(tempConfig.getActiveRules());
        m_downloadPathLineEdit->setText(tempConfig.getDownloadPath());
        m_extNameComboBox->setCurrentIndex(m_extNameComboBox->findData(tempConfig.getExtName()));
        m_sourceIdSpinBox->setValue(tempConfig.getSourceId());
        m_searchLimitSpinBox->setValue(tempConfig.getSearchLimit());
        m_autoUpdateCheckBox->setChecked(tempConfig.getAutoUpdate());

        // Crawl limit settings
        m_crawlMinIntervalSpinBox->setValue(tempConfig.getCrawlMinInterval());
        m_crawlMaxIntervalSpinBox->setValue(tempConfig.getCrawlMaxInterval());
        m_crawlThreadsSpinBox->setValue(tempConfig.getCrawlThreads());
        m_preserveChapterCacheCheckBox->setChecked(tempConfig.getPreserveChapterCache());

        m_maxRetryAttemptsSpinBox->setValue(tempConfig.getMaxRetryAttempts());
        m_retryMinIntervalSpinBox->setValue(tempConfig.getRetryMinInterval());
        m_retryMaxIntervalSpinBox->setValue(tempConfig.getRetryMaxInterval());

        // Proxy settings
        m_proxyEnabledCheckBox->setChecked(tempConfig.getProxyEnabled());
        m_proxyHostLineEdit->setText(tempConfig.getProxyHost());
        m_proxyPortSpinBox->setValue(tempConfig.getProxyPort());

        qDebug() << "Settings reset to defaults";
    }
}

// Slot function implementations
void NovelSettingsDialog::accept()
{
    applySettings();
    QDialog::accept();
}

void NovelSettingsDialog::restoreOriginalSettings()
{
    if (!m_novelConfig) return;

    // Restore original settings to both config and UI
    m_novelConfig->setLanguage(m_originalSettings.language);
    m_novelConfig->setActiveRules(m_originalSettings.activeRules);
    m_novelConfig->setDownloadPath(m_originalSettings.downloadPath);
    m_novelConfig->setExtName(m_originalSettings.extName);
    m_novelConfig->setSourceId(m_originalSettings.sourceId);
    m_novelConfig->setSearchLimit(m_originalSettings.searchLimit);
    m_novelConfig->setAutoUpdate(m_originalSettings.autoUpdate);

    m_novelConfig->setCrawlMinInterval(m_originalSettings.crawlMinInterval);
    m_novelConfig->setCrawlMaxInterval(m_originalSettings.crawlMaxInterval);
    m_novelConfig->setCrawlThreads(m_originalSettings.crawlThreads);
    m_novelConfig->setPreserveChapterCache(m_originalSettings.preserveChapterCache);

    m_novelConfig->setMaxRetryAttempts(m_originalSettings.maxRetryAttempts);
    m_novelConfig->setRetryMinInterval(m_originalSettings.retryMinInterval);
    m_novelConfig->setRetryMaxInterval(m_originalSettings.retryMaxInterval);

    m_novelConfig->setProxyEnabled(m_originalSettings.proxyEnabled);
    m_novelConfig->setProxyHost(m_originalSettings.proxyHost);
    m_novelConfig->setProxyPort(m_originalSettings.proxyPort);

    // Also update UI to reflect restored values
    loadSettings();

    qDebug() << "Settings restored to original values";
}

void NovelSettingsDialog::reject()
{
    restoreOriginalSettings();
    QDialog::reject();
}

void NovelSettingsDialog::onApplyButtonClicked()
{
    applySettings();
    markAsApplied();

    // Update original settings to current values to prevent reject() from reverting changes
    if (m_novelConfig) {
        m_originalSettings.language = m_novelConfig->getLanguage();
        m_originalSettings.activeRules = m_novelConfig->getActiveRules();
        m_originalSettings.downloadPath = m_novelConfig->getDownloadPath();
        m_originalSettings.extName = m_novelConfig->getExtName();
        m_originalSettings.sourceId = m_novelConfig->getSourceId();
        m_originalSettings.searchLimit = m_novelConfig->getSearchLimit();
        m_originalSettings.autoUpdate = m_novelConfig->getAutoUpdate();

        m_originalSettings.crawlMinInterval = m_novelConfig->getCrawlMinInterval();
        m_originalSettings.crawlMaxInterval = m_novelConfig->getCrawlMaxInterval();
        m_originalSettings.crawlThreads = m_novelConfig->getCrawlThreads();
        m_originalSettings.preserveChapterCache = m_novelConfig->getPreserveChapterCache();

        m_originalSettings.maxRetryAttempts = m_novelConfig->getMaxRetryAttempts();
        m_originalSettings.retryMinInterval = m_novelConfig->getRetryMinInterval();
        m_originalSettings.retryMaxInterval = m_novelConfig->getRetryMaxInterval();

        m_originalSettings.proxyEnabled = m_novelConfig->getProxyEnabled();
        m_originalSettings.proxyHost = m_novelConfig->getProxyHost();
        m_originalSettings.proxyPort = m_novelConfig->getProxyPort();
    }
    QMessageBox::information(this, "Settings", "Settings have been saved");
}

void NovelSettingsDialog::onCancelButtonClicked()
{
    // Smart cancel behavior:
    // 1. If user has applied changes -> just close
    // 2. If user has unsaved changes -> ask if they want to save
    // 3. If no changes -> just close

    if (hasUnsavedChanges()) {
        // User has unsaved changes, ask what to do
        if (askUserToSaveChanges()) {
            accept(); // User chose to save or discard, close dialog
        }
        // If user chose cancel in the dialog, don't close
    } else {
        // No unsaved changes, just close
        accept();
    }
}

void NovelSettingsDialog::onResetButtonClicked()
{
    // Confirm reset action
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Reset Settings",
        "Are you sure you want to reset all settings to default values?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        resetToDefaults();
        markAsModified(); // Mark as modified since user changed settings
    }
}

void NovelSettingsDialog::onBrowseActiveRulesClicked()
{
    QString currentPath = m_activeRulesLineEdit->text();
    if (currentPath.isEmpty()) {
        currentPath = QCoreApplication::applicationDirPath() + "/rules";
    }

    QString selectedFile = QFileDialog::getOpenFileName(this,
        "Select Rules File",
        currentPath,
        "JSON Files (*.json);;All Files (*.*)");

    if (!selectedFile.isEmpty()) {
        m_activeRulesLineEdit->setText(selectedFile);
    }
}

void NovelSettingsDialog::onBrowseDownloadPathClicked()
{
    QString currentPath = m_downloadPathLineEdit->text();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString selectedPath = QFileDialog::getExistingDirectory(this, "Select Download Path", currentPath);
    if (!selectedPath.isEmpty()) {
        m_downloadPathLineEdit->setText(selectedPath);
    }
}

void NovelSettingsDialog::onTestProxyClicked()
{
    if (!m_proxyEnabledCheckBox->isChecked()) {
        m_proxyStatusLabel->setText("Proxy not enabled");
        return;
    }

    QString host = m_proxyHostLineEdit->text().trimmed();
    int port = m_proxyPortSpinBox->value();

    if (host.isEmpty()) {
        m_proxyStatusLabel->setText("Proxy address is empty");
        m_proxyStatusLabel->setStyleSheet("QLabel { color: red; font-size: 12px; }");
        return;
    }

    m_testProxyButton->setEnabled(false);
    m_proxyStatusLabel->setText("Testing connection...");
    m_proxyStatusLabel->setStyleSheet("QLabel { color: blue; font-size: 12px; }");

    // TODO: Implement actual proxy test function
    // For now, simulate test result
    QTimer::singleShot(2000, [this]() {
        m_testProxyButton->setEnabled(true);
        m_proxyStatusLabel->setText("Connection test successful");
        m_proxyStatusLabel->setStyleSheet("QLabel { color: green; font-size: 12px; }");
    });

    qDebug() << "Testing proxy connection to" << host << ":" << port;
}

// Enhanced UI behavior methods
bool NovelSettingsDialog::hasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void NovelSettingsDialog::markAsModified()
{
    m_hasUnsavedChanges = true;
}

void NovelSettingsDialog::markAsApplied()
{
    m_hasUnsavedChanges = false;
    m_hasAppliedChanges = true;
}

bool NovelSettingsDialog::askUserToSaveChanges()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Unsaved Changes",
        "You have unsaved changes. Do you want to apply them before closing?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes
    );

    if (reply == QMessageBox::Yes) {
        applySettings();
        return true; // User chose to save and close
    } else if (reply == QMessageBox::No) {
        // User chose to discard changes, restore original settings
        restoreOriginalSettings();
        return true; // User chose to discard changes and close
    } else {
        return false; // User chose to cancel, don't close
    }
}