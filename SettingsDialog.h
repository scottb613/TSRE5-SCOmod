#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>

// Forward declarations to keep compile times fast
class QFormLayout;
class QTabWidget;
class QWidget;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class QLabel;
class QTextStream;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    virtual ~SettingsDialog() = default;
    void loadSettings();
    bool save(const QString& filename = QString());
    bool saveDefaults(const QString& filename = QString());

signals:
    void restartAndRestoreRequested();
    
private:
    // --- UI Setup ---
    void setupUi();
    QWidget* createScrollTab(QFormLayout*& layout, QTabWidget* tabs, const QString& title);
 // SettingsDialog.h
    void addRow(QFormLayout* l, 
                const QString& key, 
                const QString& type, 
                const QString& label, 
                const QString& helpText = ""); // Add = "" here
    QString helpForSetting(const QString& key, const QString& fallback = "") const;
    void createKeyAssignmentsTab(QTabWidget* tabs);

    // --- Data Loading & Mapping ---
    void updateWidgetValue(const QString& key, const QString& val);
    QString getGameValue(const QString& key);
    QString currentValue(const QString& key, const QString& fallback = "") const;
    QString quotedValue(const QString& key, const QString& fallback = "") const;
    void writeSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment = "", bool quote = false) const;
    void writeOptionalSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment = "", bool quote = false) const;
    bool backupSettingsFile(const QString& filename);
    QString keyAssignmentsText() const;
    void rememberLoadedValues();

    // --- State Storage ---
    // Maps the token key (e.g., "camerafov") to the specific UI widget
    QMap<QString, QCheckBox*> enabledMap;      // The "Enabled" column checkboxes
    QMap<QString, QWidget*> valueWidgetMap;    // Primary input (QLineEdit, QCheckBox, or QTextEdit)
    QMap<QString, QLineEdit*> subValueWidgetMap; // Secondary input for "twonumber" types
    QMap<QString, QString> fileValueMap;       // Existing settings values not exposed by the dialog
    QMap<QString, bool> fileActiveMap;         // True when the optional setting exists in settings.json
    QMap<QString, QString> loadedValueMap;     // Values presented when the dialog was loaded
    bool savingBuiltInDefaults = false;
};

#endif // SETTINGSDIALOG_H
