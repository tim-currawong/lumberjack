#include "color_family_dialog.hpp"
#include "color_family_manager.hpp"
#include <QColorDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QSplitter>
#include <QFrame>
#include <QPainter>

ColorFamilyDialog::ColorFamilyDialog(QWidget *parent)
    : QDialog(parent)
    , selectedFamilyIndex(-1)
    , currentBaseColor(Qt::blue)
{
    setWindowTitle("Color Family Preferences");
    setupUi();

    // Load initial configuration
    ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
    QString activeName = cfm->getActiveConfigurationName();

    refreshConfigurationsList();

    if (!activeName.isEmpty())
    {
        int index = configComboBox->findText(activeName);
        if (index >= 0)
        {
            configComboBox->setCurrentIndex(index);
        }
    }
}

ColorFamilyDialog::~ColorFamilyDialog()
{
}

void ColorFamilyDialog::setupUi()
{
    resize(800, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Configuration selector section
    QGroupBox* configGroupBox = new QGroupBox("Active Configuration");
    QHBoxLayout* configLayout = new QHBoxLayout(configGroupBox);

    configComboBox = new QComboBox();
    configComboBox->addItem("(None)");
    configLayout->addWidget(configComboBox);

    newConfigBtn = new QPushButton("New");
    renameConfigBtn = new QPushButton("Rename");
    deleteConfigBtn = new QPushButton("Delete");

    configLayout->addWidget(newConfigBtn);
    configLayout->addWidget(renameConfigBtn);
    configLayout->addWidget(deleteConfigBtn);

    mainLayout->addWidget(configGroupBox);

    // Main content splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Left side - families list
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* familiesLabel = new QLabel("Color Families:");
    leftLayout->addWidget(familiesLabel);

    familiesListWidget = new QListWidget();
    leftLayout->addWidget(familiesListWidget);

    QHBoxLayout* familyButtonsLayout = new QHBoxLayout();
    addFamilyBtn = new QPushButton("+ Add Family");
    removeFamilyBtn = new QPushButton("- Remove");
    moveUpBtn = new QPushButton("↑");
    moveDownBtn = new QPushButton("↓");

    moveUpBtn->setMaximumWidth(40);
    moveDownBtn->setMaximumWidth(40);

    familyButtonsLayout->addWidget(addFamilyBtn);
    familyButtonsLayout->addWidget(removeFamilyBtn);
    familyButtonsLayout->addStretch();
    familyButtonsLayout->addWidget(moveUpBtn);
    familyButtonsLayout->addWidget(moveDownBtn);

    leftLayout->addLayout(familyButtonsLayout);

    splitter->addWidget(leftWidget);

    // Right side - family editor
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* editorGroupBox = new QGroupBox("Edit Selected Family");
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroupBox);

    // Name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    familyNameEdit = new QLineEdit();
    nameLayout->addWidget(familyNameEdit);
    editorLayout->addLayout(nameLayout);

    // Base color
    QHBoxLayout* colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("Base Color:"));
    baseColorBtn = new QPushButton("Choose...");
    baseColorBtn->setMaximumWidth(100);
    colorLayout->addWidget(baseColorBtn);
    colorLayout->addStretch();
    editorLayout->addLayout(colorLayout);

    // Color preview
    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addWidget(new QLabel("Preview Shades:"));
    colorPreviewLabel = new QLabel();
    colorPreviewLabel->setMinimumHeight(40);
    colorPreviewLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
    colorPreviewLabel->setScaledContents(true);
    previewLayout->addWidget(colorPreviewLabel, 1);
    editorLayout->addLayout(previewLayout);

    // Patterns
    editorLayout->addWidget(new QLabel("Match Patterns (one per line, case-insensitive):"));
    patternsTextEdit = new QTextEdit();
    patternsTextEdit->setMaximumHeight(100);
    patternsTextEdit->setPlaceholderText("*current*\n*ampere*\n*_I_*");
    editorLayout->addWidget(patternsTextEdit);

    QLabel* wildcardHelp = new QLabel("Wildcards: * = any characters, ? = single character");
    wildcardHelp->setStyleSheet("color: gray; font-size: 9pt;");
    editorLayout->addWidget(wildcardHelp);

    editorLayout->addStretch();

    rightLayout->addWidget(editorGroupBox);

    // Default palette section
    QGroupBox* defaultPaletteGroupBox = new QGroupBox("Fallback Palette (for unmatched series)");
    QHBoxLayout* paletteLayout = new QHBoxLayout(defaultPaletteGroupBox);
    defaultPaletteCombo = new QComboBox();
    defaultPaletteCombo->addItem("Tableau 10");
    defaultPaletteCombo->addItem("ColorBrewer Set1");
    paletteLayout->addWidget(defaultPaletteCombo);
    paletteLayout->addStretch();

    rightLayout->addWidget(defaultPaletteGroupBox);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    okBtn = new QPushButton("OK");
    cancelBtn = new QPushButton("Cancel");
    applyBtn = new QPushButton("Apply");

    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(applyBtn);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(configComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColorFamilyDialog::onConfigurationChanged);
    connect(newConfigBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onNewConfiguration);
    connect(renameConfigBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onRenameConfiguration);
    connect(deleteConfigBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onDeleteConfiguration);

    connect(familiesListWidget, &QListWidget::currentRowChanged,
            this, &ColorFamilyDialog::onFamilySelectionChanged);
    connect(addFamilyBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onAddFamily);
    connect(removeFamilyBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onRemoveFamily);
    connect(moveUpBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onMoveFamilyUp);
    connect(moveDownBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onMoveFamilyDown);

    connect(familyNameEdit, &QLineEdit::textChanged, this, &ColorFamilyDialog::onFamilyNameChanged);
    connect(baseColorBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onChooseBaseColor);
    connect(patternsTextEdit, &QTextEdit::textChanged, this, &ColorFamilyDialog::onPatternsChanged);
    connect(defaultPaletteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColorFamilyDialog::onDefaultPaletteChanged);

    connect(okBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onOk);
    connect(cancelBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onCancel);
    connect(applyBtn, &QPushButton::clicked, this, &ColorFamilyDialog::onApply);

    // Initial state
    refreshFamilyEditor();
}

void ColorFamilyDialog::onConfigurationChanged(int index)
{
    QString name = configComboBox->currentText();

    if (name == "(None)" || name.isEmpty())
    {
        currentConfigName = "";
        workingConfig = ColorFamilyConfiguration();
        refreshFamiliesList();
        refreshFamilyEditor();
        return;
    }

    loadConfiguration(name);
}

void ColorFamilyDialog::onNewConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Configuration",
                                         "Configuration name:",
                                         QLineEdit::Normal,
                                         "", &ok);

    if (ok && !name.isEmpty())
    {
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();

        if (cfm->hasConfiguration(name))
        {
            QMessageBox::warning(this, "Name Exists",
                               "A configuration with this name already exists.");
            return;
        }

        // Create new empty configuration
        ColorFamilyConfiguration newConfig(name);
        cfm->saveConfiguration(name, newConfig);

        refreshConfigurationsList();

        int index = configComboBox->findText(name);
        if (index >= 0)
        {
            configComboBox->setCurrentIndex(index);
        }
    }
}

void ColorFamilyDialog::onRenameConfiguration()
{
    QString oldName = configComboBox->currentText();

    if (oldName == "(None)" || oldName.isEmpty())
    {
        QMessageBox::information(this, "No Configuration",
                               "Please select a configuration to rename.");
        return;
    }

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Configuration",
                                           "New name:",
                                           QLineEdit::Normal,
                                           oldName, &ok);

    if (ok && !newName.isEmpty() && newName != oldName)
    {
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();

        if (cfm->hasConfiguration(newName))
        {
            QMessageBox::warning(this, "Name Exists",
                               "A configuration with this name already exists.");
            return;
        }

        cfm->renameConfiguration(oldName, newName);
        refreshConfigurationsList();

        int index = configComboBox->findText(newName);
        if (index >= 0)
        {
            configComboBox->setCurrentIndex(index);
        }
    }
}

void ColorFamilyDialog::onDeleteConfiguration()
{
    QString name = configComboBox->currentText();

    if (name == "(None)" || name.isEmpty())
    {
        QMessageBox::information(this, "No Configuration",
                               "Please select a configuration to delete.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete Configuration",
                                  QString("Are you sure you want to delete '%1'?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
        cfm->deleteConfiguration(name);
        refreshConfigurationsList();
    }
}

void ColorFamilyDialog::onFamilySelectionChanged()
{
    selectedFamilyIndex = familiesListWidget->currentRow();
    refreshFamilyEditor();
}

void ColorFamilyDialog::onAddFamily()
{
    if (currentConfigName.isEmpty())
    {
        QMessageBox::information(this, "No Configuration",
                               "Please create or select a configuration first.");
        return;
    }

    ColorFamily newFamily("New Family", Qt::blue, QStringList());
    workingConfig.addFamily(newFamily);
    refreshFamiliesList();

    // Select the new family
    familiesListWidget->setCurrentRow(workingConfig.getFamilyCount() - 1);
}

void ColorFamilyDialog::onRemoveFamily()
{
    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount())
    {
        workingConfig.removeFamily(selectedFamilyIndex);
        refreshFamiliesList();

        if (selectedFamilyIndex >= workingConfig.getFamilyCount())
        {
            selectedFamilyIndex = workingConfig.getFamilyCount() - 1;
        }

        if (selectedFamilyIndex >= 0)
        {
            familiesListWidget->setCurrentRow(selectedFamilyIndex);
        }
        else
        {
            refreshFamilyEditor();
        }
    }
}

void ColorFamilyDialog::onMoveFamilyUp()
{
    if (selectedFamilyIndex > 0)
    {
        workingConfig.reorderFamily(selectedFamilyIndex, selectedFamilyIndex - 1);
        selectedFamilyIndex--;
        refreshFamiliesList();
        familiesListWidget->setCurrentRow(selectedFamilyIndex);
    }
}

void ColorFamilyDialog::onMoveFamilyDown()
{
    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount() - 1)
    {
        workingConfig.reorderFamily(selectedFamilyIndex, selectedFamilyIndex + 1);
        selectedFamilyIndex++;
        refreshFamiliesList();
        familiesListWidget->setCurrentRow(selectedFamilyIndex);
    }
}

void ColorFamilyDialog::onFamilyNameChanged()
{
    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount())
    {
        ColorFamily* family = workingConfig.getFamilyAt(selectedFamilyIndex);
        if (family)
        {
            family->setName(familyNameEdit->text());
            refreshFamiliesList();
            familiesListWidget->setCurrentRow(selectedFamilyIndex);
        }
    }
}

void ColorFamilyDialog::onChooseBaseColor()
{
    QColor color = QColorDialog::getColor(currentBaseColor, this, "Choose Base Color");

    if (color.isValid())
    {
        currentBaseColor = color;

        if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount())
        {
            ColorFamily* family = workingConfig.getFamilyAt(selectedFamilyIndex);
            if (family)
            {
                family->setBaseHue(color);
                refreshColorPreview();
                refreshFamiliesList();
                familiesListWidget->setCurrentRow(selectedFamilyIndex);
            }
        }
    }
}

void ColorFamilyDialog::onPatternsChanged()
{
    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount())
    {
        ColorFamily* family = workingConfig.getFamilyAt(selectedFamilyIndex);
        if (family)
        {
            QString text = patternsTextEdit->toPlainText();
            QStringList patterns = text.split('\n', Qt::SkipEmptyParts);
            family->setPatterns(patterns);
        }
    }
}

void ColorFamilyDialog::onDefaultPaletteChanged(int index)
{
    if (!currentConfigName.isEmpty())
    {
        workingConfig.setDefaultPaletteName(defaultPaletteCombo->currentText());
    }
}

void ColorFamilyDialog::onOk()
{
    onApply();
    accept();
}

void ColorFamilyDialog::onCancel()
{
    reject();
}

void ColorFamilyDialog::onApply()
{
    if (!currentConfigName.isEmpty())
    {
        saveCurrentConfiguration();

        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
        cfm->setActiveConfiguration(currentConfigName);

        // Re-apply colors to all existing series
        cfm->reapplyAllColorFamilies();
    }
}

void ColorFamilyDialog::refreshConfigurationsList()
{
    ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
    QStringList names = cfm->getConfigurationNames();

    configComboBox->blockSignals(true);
    configComboBox->clear();
    configComboBox->addItem("(None)");
    configComboBox->addItems(names);
    configComboBox->blockSignals(false);
}

void ColorFamilyDialog::refreshFamiliesList()
{
    familiesListWidget->blockSignals(true);
    familiesListWidget->clear();

    QList<ColorFamily> families = workingConfig.getFamilies();
    for (int i = 0; i < families.count(); i++)
    {
        const ColorFamily& family = families.at(i);

        // Create colored icon for the family
        QPixmap pixmap(60, 20);
        QColor baseColor = family.getBaseHue();

        // Generate shades for preview
        ColorFamily tempFamily = family;
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
        tempFamily.generateShades(cfm->getPlotBackgroundColor());
        QList<QColor> shades = tempFamily.getShades();

        // Draw shades side by side, covering the full pixmap width exactly
        QPainter painter(&pixmap);
        int totalWidth = pixmap.width();
        for (int j = 0; j < shades.count(); j++)
        {
            int xStart = totalWidth * j / shades.count();
            int xEnd = totalWidth * (j + 1) / shades.count();
            painter.fillRect(xStart, 0, xEnd - xStart, pixmap.height(), shades.at(j));
        }

        QListWidgetItem* item = new QListWidgetItem(family.getName());
        item->setIcon(QIcon(pixmap));
        familiesListWidget->addItem(item);
    }

    familiesListWidget->blockSignals(false);

    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < familiesListWidget->count())
    {
        familiesListWidget->setCurrentRow(selectedFamilyIndex);
    }
}

void ColorFamilyDialog::refreshFamilyEditor()
{
    bool hasSelection = (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount());

    familyNameEdit->setEnabled(hasSelection);
    baseColorBtn->setEnabled(hasSelection);
    patternsTextEdit->setEnabled(hasSelection);

    if (hasSelection)
    {
        ColorFamily* family = workingConfig.getFamilyAt(selectedFamilyIndex);
        if (family)
        {
            familyNameEdit->blockSignals(true);
            patternsTextEdit->blockSignals(true);

            familyNameEdit->setText(family->getName());
            currentBaseColor = family->getBaseHue();

            QStringList patterns = family->getPatterns();
            patternsTextEdit->setPlainText(patterns.join('\n'));

            familyNameEdit->blockSignals(false);
            patternsTextEdit->blockSignals(false);

            refreshColorPreview();
        }
    }
    else
    {
        familyNameEdit->clear();
        patternsTextEdit->clear();
        colorPreviewLabel->clear();
    }
}

void ColorFamilyDialog::refreshColorPreview()
{
    if (selectedFamilyIndex >= 0 && selectedFamilyIndex < workingConfig.getFamilyCount())
    {
        ColorFamily* family = workingConfig.getFamilyAt(selectedFamilyIndex);
        if (family)
        {
            // Generate shades
            ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
            family->generateShades(cfm->getPlotBackgroundColor());
            QList<QColor> shades = family->getShades();

            // Create preview pixmap at a fixed resolution, independent of the
            // label's current on-screen size. The label has scaledContents
            // enabled and stretches this to fit; drawing at the label's live
            // width/height here would feed the rendered size back into the
            // next pixmap, growing on every refresh.
            const int width = 400;
            const int height = 60;

            QPixmap pixmap(width, height);
            QPainter painter(&pixmap);

            if (shades.isEmpty())
            {
                painter.fillRect(0, 0, width, height, Qt::white);
            }
            else
            {
                for (int i = 0; i < shades.count(); i++)
                {
                    int xStart = width * i / shades.count();
                    int xEnd = width * (i + 1) / shades.count();
                    painter.fillRect(xStart, 0, xEnd - xStart, height, shades.at(i));
                }
            }

            colorPreviewLabel->setPixmap(pixmap);
        }
    }
}

void ColorFamilyDialog::loadConfiguration(QString name)
{
    ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
    ColorFamilyConfiguration* config = cfm->getConfiguration(name);

    if (config)
    {
        currentConfigName = name;
        workingConfig = *config;

        // Set default palette combo
        int paletteIndex = defaultPaletteCombo->findText(workingConfig.getDefaultPaletteName());
        if (paletteIndex >= 0)
        {
            defaultPaletteCombo->setCurrentIndex(paletteIndex);
        }

        refreshFamiliesList();
        refreshFamilyEditor();
    }
}

void ColorFamilyDialog::saveCurrentConfiguration()
{
    if (!currentConfigName.isEmpty())
    {
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
        cfm->saveConfiguration(currentConfigName, workingConfig);
    }
}
