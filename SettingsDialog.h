#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>

// Forward declarations to keep compile times fast
class QFormLayout;
class QTabWidget;
class QWidget;
class QLineEdit;
class QTextEdit;
class QLabel;
class QTextStream;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;
    void loadSettings();
    bool save(const QString& filename = QString());
    bool saveDefaults(const QString& filename = QString());

signals:
    void restartAndRestoreRequested();
    
private:
    void setupUi();
    void createScrollTab(QFormLayout*& layout, QTabWidget* tabs, const QString& title);
    void addRow(QFormLayout* l, const QString& key, const QString& type,
                const QString& label, const QString& helpText = "");
    QString helpForSetting(const QString& key, const QString& fallback = "") const;
    void createKeyAssignmentsTab(QTabWidget* tabs);

    void updateWidgetValue(const QString& key, const QString& val);
    QString getGameValue(const QString& key);
    QString currentValue(const QString& key, const QString& fallback = "") const;
    void writeSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment = "", bool quote = false) const;
    void writeOptionalSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment = "", bool quote = false) const;
    bool backupSettingsFile(const QString& filename);
    QString keyAssignmentsText() const;
    void rememberLoadedValues();

    QMap<QString, QWidget*> valueWidgetMap;
    QMap<QString, QLineEdit*> subValueWidgetMap;
    QMap<QString, QString> fileValueMap;
    QMap<QString, bool> fileActiveMap;
    QMap<QString, QString> loadedValueMap;
    bool savingBuiltInDefaults = false;
};

#endif // SETTINGSDIALOG_H
