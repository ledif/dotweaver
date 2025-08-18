#include "configeditor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

ConfigEditor::ConfigEditor(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_generalGroup(nullptr)
    , m_sourceDirectoryEdit(nullptr)
    , m_workingTreeEdit(nullptr)
    , m_useBuiltinGitCheck(nullptr)
    , m_editorGroup(nullptr)
    , m_editorCommandEdit(nullptr)
    , m_autoSaveCheck(nullptr)
    , m_autoSaveIntervalSpin(nullptr)
    , m_templateGroup(nullptr)
    , m_templateLeftDelimEdit(nullptr)
    , m_templateRightDelimEdit(nullptr)
    , m_gitGroup(nullptr)
    , m_gitAutoCommitEdit(nullptr)
    , m_gitAutoPushCheck(nullptr)
    , m_advancedGroup(nullptr)
    , m_customConfigEdit(nullptr)
{
    setupUI();
    connectSignals();
    loadConfiguration();
}

ConfigEditor::~ConfigEditor() = default;

void ConfigEditor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // General settings
    m_generalGroup = new QGroupBox(tr("General Settings"), this);
    auto *generalLayout = new QFormLayout(m_generalGroup);
    
    m_sourceDirectoryEdit = new QLineEdit(this);
    m_sourceDirectoryEdit->setPlaceholderText(tr("~/.local/share/chezmoi"));
    auto *sourceDirLayout = new QHBoxLayout();
    sourceDirLayout->addWidget(m_sourceDirectoryEdit);
    auto *sourceDirButton = new QPushButton(tr("Browse..."), this);
    connect(sourceDirButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Source Directory"));
        if (!dir.isEmpty()) {
            m_sourceDirectoryEdit->setText(dir);
        }
    });
    sourceDirLayout->addWidget(sourceDirButton);
    generalLayout->addRow(tr("Source Directory:"), sourceDirLayout);
    
    m_workingTreeEdit = new QLineEdit(this);
    m_workingTreeEdit->setPlaceholderText(tr("~"));
    generalLayout->addRow(tr("Working Tree:"), m_workingTreeEdit);
    
    m_useBuiltinGitCheck = new QCheckBox(tr("Use built-in Git functionality"), this);
    generalLayout->addRow(m_useBuiltinGitCheck);
    
    m_mainLayout->addWidget(m_generalGroup);
    
    // Editor settings
    m_editorGroup = new QGroupBox(tr("Editor Settings"), this);
    auto *editorLayout = new QFormLayout(m_editorGroup);
    
    m_editorCommandEdit = new QLineEdit(this);
    m_editorCommandEdit->setPlaceholderText(tr("kate"));
    editorLayout->addRow(tr("Editor Command:"), m_editorCommandEdit);
    
    m_autoSaveCheck = new QCheckBox(tr("Enable auto-save"), this);
    editorLayout->addRow(m_autoSaveCheck);
    
    m_autoSaveIntervalSpin = new QSpinBox(this);
    m_autoSaveIntervalSpin->setRange(1, 300);
    m_autoSaveIntervalSpin->setValue(30);
    m_autoSaveIntervalSpin->setSuffix(tr(" seconds"));
    editorLayout->addRow(tr("Auto-save Interval:"), m_autoSaveIntervalSpin);
    
    m_mainLayout->addWidget(m_editorGroup);
    
    // Template settings
    m_templateGroup = new QGroupBox(tr("Template Settings"), this);
    auto *templateLayout = new QFormLayout(m_templateGroup);
    
    m_templateLeftDelimEdit = new QLineEdit(this);
    m_templateLeftDelimEdit->setPlaceholderText(tr("{{"));
    templateLayout->addRow(tr("Left Delimiter:"), m_templateLeftDelimEdit);
    
    m_templateRightDelimEdit = new QLineEdit(this);
    m_templateRightDelimEdit->setPlaceholderText(tr("}}"));
    templateLayout->addRow(tr("Right Delimiter:"), m_templateRightDelimEdit);
    
    m_mainLayout->addWidget(m_templateGroup);
    
    // Git settings
    m_gitGroup = new QGroupBox(tr("Git Settings"), this);
    auto *gitLayout = new QFormLayout(m_gitGroup);
    
    m_gitAutoCommitEdit = new QLineEdit(this);
    m_gitAutoCommitEdit->setPlaceholderText(tr("Auto-commit message template"));
    gitLayout->addRow(tr("Auto-commit Message:"), m_gitAutoCommitEdit);
    
    m_gitAutoPushCheck = new QCheckBox(tr("Automatically push changes"), this);
    gitLayout->addRow(m_gitAutoPushCheck);
    
    m_mainLayout->addWidget(m_gitGroup);
    
    // Advanced settings
    m_advancedGroup = new QGroupBox(tr("Advanced Settings"), this);
    auto *advancedLayout = new QVBoxLayout(m_advancedGroup);
    
    auto *customConfigLabel = new QLabel(tr("Custom Configuration (TOML):"), this);
    advancedLayout->addWidget(customConfigLabel);
    
    m_customConfigEdit = new QTextEdit(this);
    m_customConfigEdit->setMaximumHeight(150);
    m_customConfigEdit->setPlainText(tr("# Add custom chezmoi configuration here\n"));
    advancedLayout->addWidget(m_customConfigEdit);
    
    m_mainLayout->addWidget(m_advancedGroup);
    
    m_mainLayout->addStretch();
}

void ConfigEditor::connectSignals()
{
    connect(m_sourceDirectoryEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_workingTreeEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_useBuiltinGitCheck, &QCheckBox::toggled, this, &ConfigEditor::onConfigValueChanged);
    connect(m_editorCommandEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_autoSaveCheck, &QCheckBox::toggled, this, &ConfigEditor::onConfigValueChanged);
    connect(m_autoSaveIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigEditor::onConfigValueChanged);
    connect(m_templateLeftDelimEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_templateRightDelimEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_gitAutoCommitEdit, &QLineEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    connect(m_gitAutoPushCheck, &QCheckBox::toggled, this, &ConfigEditor::onConfigValueChanged);
    connect(m_customConfigEdit, &QTextEdit::textChanged, this, &ConfigEditor::onConfigValueChanged);
    
    // Enable/disable auto-save interval based on auto-save checkbox
    connect(m_autoSaveCheck, &QCheckBox::toggled, m_autoSaveIntervalSpin, &QSpinBox::setEnabled);
}

void ConfigEditor::loadConfiguration()
{
    QSettings settings(QStringLiteral("DotWeaver"), QStringLiteral("DotWeaver"));
    
    // General settings
    QString defaultSourceDir = QDir::homePath() + QStringLiteral("/.local/share/chezmoi");
    m_sourceDirectoryEdit->setText(settings.value(QStringLiteral("sourceDirectory"), defaultSourceDir).toString());
    m_workingTreeEdit->setText(settings.value(QStringLiteral("workingTree"), QDir::homePath()).toString());
    m_useBuiltinGitCheck->setChecked(settings.value(QStringLiteral("useBuiltinGit"), true).toBool());
    
    // Editor settings
    QString defaultEditor = QStringLiteral("kate");
    if (QStandardPaths::findExecutable(QStringLiteral("kwrite")).isEmpty() == false) {
        defaultEditor = QStringLiteral("kwrite");
    } else if (QStandardPaths::findExecutable(QStringLiteral("gedit")).isEmpty() == false) {
        defaultEditor = QStringLiteral("gedit");
    }
    m_editorCommandEdit->setText(settings.value(QStringLiteral("editorCommand"), defaultEditor).toString());
    m_autoSaveCheck->setChecked(settings.value(QStringLiteral("autoSave"), false).toBool());
    m_autoSaveIntervalSpin->setValue(settings.value(QStringLiteral("autoSaveInterval"), 30).toInt());
    
    // Template settings
    m_templateLeftDelimEdit->setText(settings.value(QStringLiteral("templateLeftDelim"), QStringLiteral("{{")).toString());
    m_templateRightDelimEdit->setText(settings.value(QStringLiteral("templateRightDelim"), QStringLiteral("}}")).toString());
    
    // Git settings
    m_gitAutoCommitEdit->setText(settings.value(QStringLiteral("gitAutoCommit"), QStringLiteral("Auto-commit from KChezmoi")).toString());
    m_gitAutoPushCheck->setChecked(settings.value(QStringLiteral("gitAutoPush"), false).toBool());
    
    // Advanced settings
    m_customConfigEdit->setPlainText(settings.value(QStringLiteral("customConfig"), QStringLiteral("# Add custom chezmoi configuration here\n")).toString());
    
    // Update UI state
    m_autoSaveIntervalSpin->setEnabled(m_autoSaveCheck->isChecked());
}

void ConfigEditor::saveConfiguration()
{
    QSettings settings(QStringLiteral("DotWeaver"), QStringLiteral("DotWeaver"));
    
    // General settings
    settings.setValue(QStringLiteral("sourceDirectory"), m_sourceDirectoryEdit->text());
    settings.setValue(QStringLiteral("workingTree"), m_workingTreeEdit->text());
    settings.setValue(QStringLiteral("useBuiltinGit"), m_useBuiltinGitCheck->isChecked());
    
    // Editor settings
    settings.setValue(QStringLiteral("editorCommand"), m_editorCommandEdit->text());
    settings.setValue(QStringLiteral("autoSave"), m_autoSaveCheck->isChecked());
    settings.setValue(QStringLiteral("autoSaveInterval"), m_autoSaveIntervalSpin->value());
    
    // Template settings
    settings.setValue(QStringLiteral("templateLeftDelim"), m_templateLeftDelimEdit->text());
    settings.setValue(QStringLiteral("templateRightDelim"), m_templateRightDelimEdit->text());
    
    // Git settings
    settings.setValue(QStringLiteral("gitAutoCommit"), m_gitAutoCommitEdit->text());
    settings.setValue(QStringLiteral("gitAutoPush"), m_gitAutoPushCheck->isChecked());
    
    // Advanced settings
    settings.setValue(QStringLiteral("customConfig"), m_customConfigEdit->toPlainText());
    
    settings.sync();
}

void ConfigEditor::onConfigValueChanged()
{
    saveConfiguration();
    Q_EMIT configurationChanged();
}
