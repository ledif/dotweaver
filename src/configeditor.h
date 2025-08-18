#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H

#include <QWidget>

class QVBoxLayout;
class QFormLayout;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QTextEdit;
class QGroupBox;

class ConfigEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigEditor(QWidget *parent = nullptr);
    ~ConfigEditor() override;

    void loadConfiguration();
    void saveConfiguration();

Q_SIGNALS:
    void configurationChanged();

private Q_SLOTS:
    void onConfigValueChanged();

private:
    void setupUI();
    void connectSignals();

    QVBoxLayout *m_mainLayout;
    
    // General settings
    QGroupBox *m_generalGroup;
    QLineEdit *m_sourceDirectoryEdit;
    QLineEdit *m_workingTreeEdit;
    QCheckBox *m_useBuiltinGitCheck;
    
    // Editor settings
    QGroupBox *m_editorGroup;
    QLineEdit *m_editorCommandEdit;
    QCheckBox *m_autoSaveCheck;
    QSpinBox *m_autoSaveIntervalSpin;
    
    // Template settings
    QGroupBox *m_templateGroup;
    QLineEdit *m_templateLeftDelimEdit;
    QLineEdit *m_templateRightDelimEdit;
    
    // Git settings
    QGroupBox *m_gitGroup;
    QLineEdit *m_gitAutoCommitEdit;
    QCheckBox *m_gitAutoPushCheck;
    
    // Advanced settings
    QGroupBox *m_advancedGroup;
    QTextEdit *m_customConfigEdit;
};

#endif // CONFIGEDITOR_H
