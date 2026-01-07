#ifndef COLOR_FAMILY_DIALOG_HPP
#define COLOR_FAMILY_DIALOG_HPP

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "color_family_configuration.hpp"

/**
 * @brief The ColorFamilyDialog class provides UI for managing color family configurations
 */
class ColorFamilyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorFamilyDialog(QWidget *parent = nullptr);
    virtual ~ColorFamilyDialog();

private slots:
    // Configuration management
    void onConfigurationChanged(int index);
    void onNewConfiguration();
    void onRenameConfiguration();
    void onDeleteConfiguration();

    // Family list management
    void onFamilySelectionChanged();
    void onAddFamily();
    void onRemoveFamily();
    void onMoveFamilyUp();
    void onMoveFamilyDown();

    // Family editing
    void onFamilyNameChanged();
    void onChooseBaseColor();
    void onPatternsChanged();
    void onDefaultPaletteChanged(int index);

    // Dialog buttons
    void onOk();
    void onCancel();
    void onApply();

    // Refresh UI
    void refreshConfigurationsList();
    void refreshFamiliesList();
    void refreshFamilyEditor();
    void refreshColorPreview();

private:
    // UI Components - Configuration selector
    QComboBox* configComboBox;
    QPushButton* newConfigBtn;
    QPushButton* renameConfigBtn;
    QPushButton* deleteConfigBtn;

    // UI Components - Family list
    QListWidget* familiesListWidget;
    QPushButton* addFamilyBtn;
    QPushButton* removeFamilyBtn;
    QPushButton* moveUpBtn;
    QPushButton* moveDownBtn;

    // UI Components - Family editor
    QLineEdit* familyNameEdit;
    QPushButton* baseColorBtn;
    QLabel* colorPreviewLabel;
    QTextEdit* patternsTextEdit;

    // UI Components - Default palette
    QComboBox* defaultPaletteCombo;

    // Dialog buttons
    QPushButton* okBtn;
    QPushButton* cancelBtn;
    QPushButton* applyBtn;

    // Data
    QString currentConfigName;
    ColorFamilyConfiguration workingConfig;
    int selectedFamilyIndex;

    // Helper methods
    void setupUi();
    void loadConfiguration(QString name);
    void saveCurrentConfiguration();
    QColor currentBaseColor;
};

#endif // COLOR_FAMILY_DIALOG_HPP
