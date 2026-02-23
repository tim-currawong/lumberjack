#include "math_trace_dialog.hpp"
#include "ui_math_trace_dialog.h"
#include "data_source_manager.hpp"
#include "math_data_source.hpp"
#include "math_expression_parser.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

//=============================================================================
// VariableRow Implementation
//=============================================================================
// Represents a single row in the variable mapping UI:
// [var_name] = [Series Dropdown] [-]
//

VariableRow::VariableRow(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    editVariableName = new QLineEdit(this);
    editVariableName->setPlaceholderText("Variable name (e.g., rpm1)");
    editVariableName->setMaximumWidth(150);

    QLabel* labelEquals = new QLabel("=", this);

    comboSeries = new QComboBox(this);
    comboSeries->setMinimumWidth(300);

    btnDelete = new QPushButton("-", this);
    btnDelete->setMaximumWidth(30);
    btnDelete->setToolTip("Remove this variable");

    layout->addWidget(editVariableName);
    layout->addWidget(labelEquals);
    layout->addWidget(comboSeries);
    layout->addWidget(btnDelete);
    layout->addStretch();

    connect(btnDelete, &QPushButton::clicked, this, &VariableRow::onDeleteClicked);

    populateSeriesComboBox();
}

VariableRow::~VariableRow()
{
}

QString VariableRow::getVariableName() const
{
    return editVariableName->text().trimmed();
}

void VariableRow::setVariableName(const QString& name)
{
    editVariableName->setText(name);
}

DataSeriesPointer VariableRow::getSelectedSeries() const
{
    int index = comboSeries->currentIndex();
    if (index < 0 || !seriesMap.contains(index))
    {
        return DataSeriesPointer();
    }

    return seriesMap[index];
}

void VariableRow::setSelectedSeries(const QString& sourceLabel, const QString& seriesLabel)
{
    QString combinedLabel = sourceLabel + " - " + seriesLabel;

    for (int i = 0; i < comboSeries->count(); ++i)
    {
        if (comboSeries->itemText(i) == combinedLabel)
        {
            comboSeries->setCurrentIndex(i);
            return;
        }
    }
}

/**
 * @brief Populate the series dropdown with all available data series
 *
 * Format: "SourceName - SeriesLabel" (e.g., "CSV File - Voltage", "Telemetry - RPM")
 * Only includes series that have data (non-empty).
 */
void VariableRow::populateSeriesComboBox()
{
    comboSeries->clear();
    seriesMap.clear();

    DataSourceManager* manager = DataSourceManager::getInstance();
    int itemIndex = 0;

    // Iterate through all data sources
    for (int i = 0; i < manager->getSourceCount(); ++i)
    {
        DataSourcePointer source = manager->getSourceByIndex(i);
        if (!source)
        {
            continue;
        }

        QStringList seriesLabels = source->getSeriesLabels();

        // Add each series from this source
        for (const QString& seriesLabel : seriesLabels)
        {
            DataSeriesPointer series = source->getSeriesByLabel(seriesLabel);
            if (series && series->hasData())  // Only non-empty series
            {
                QString displayText = source->getLabel() + " - " + seriesLabel;
                comboSeries->addItem(displayText);
                seriesMap[itemIndex] = series;  // Store pointer for retrieval
                itemIndex++;
            }
        }
    }
}

void VariableRow::onDeleteClicked()
{
    emit deleteRequested(this);
}

//=============================================================================
// MathTraceDialog Implementation
//=============================================================================
// Main dialog for creating and editing math traces
//
// User workflow:
// 1. Define variables (short names like "V", "I")
// 2. Map each variable to a data series from dropdowns
// 3. Enter mathematical expression (e.g., "V * I")
// 4. Name the output trace (e.g., "Power")
// 5. Click OK → Validation → Background computation → Done
//
// Validation ensures:
// - All variable names are unique and non-empty
// - All variables are mapped to series
// - Expression is syntactically valid
// - All expression variables have definitions
// - Trace name is unique
//

MathTraceDialog::MathTraceDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::MathTraceDialog),
      computeThread(nullptr),
      computer(nullptr),
      isEditMode(false)
{
    ui->setupUi(this);

    connect(ui->btnAddVariable, &QPushButton::clicked, this, &MathTraceDialog::onAddVariableClicked);
    connect(ui->editExpression, &QLineEdit::textChanged, this, &MathTraceDialog::onExpressionChanged);

    // Add initial variable row
    onAddVariableClicked();

    // Set window modality
    setModal(true);
}

MathTraceDialog::~MathTraceDialog()
{
    // Clean up computation thread
    if (computeThread)
    {
        if (computer)
        {
            computer->cancelComputation();
        }

        computeThread->quit();
        computeThread->wait();

        delete computer;
        delete computeThread;
    }

    delete ui;
}

void MathTraceDialog::setEditMode(MathDataSeriesPointer series)
{
    if (!series)
    {
        return;
    }

    isEditMode = true;
    editingSeries = series;

    setWindowTitle("Edit Math Trace");

    // Populate fields from existing series
    ui->editTraceName->setText(series->getLabel());
    ui->editExpression->setText(series->getExpression());

    // Clear existing variable rows
    while (!variableRows.isEmpty())
    {
        VariableRow* row = variableRows.takeFirst();
        ui->variablesLayout->removeWidget(row);
        row->deleteLater();
    }

    // Add variable rows from existing mapping
    QMap<QString, DataSeriesPointer> mapping = series->getVariableMapping();
    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        QString varName = it.key();
        DataSeriesPointer inputSeries = it.value();

        VariableRow* row = new VariableRow(this);
        row->setVariableName(varName);

        // Find the source for this series
        DataSourceManager* manager = DataSourceManager::getInstance();
        for (int i = 0; i < manager->getSourceCount(); ++i)
        {
            DataSourcePointer source = manager->getSourceByIndex(i);
            if (source)
            {
                DataSeriesPointer foundSeries = source->getSeriesByLabel(inputSeries->getLabel());
                if (foundSeries == inputSeries)
                {
                    row->setSelectedSeries(source->getLabel(), inputSeries->getLabel());
                    break;
                }
            }
        }

        connect(row, &VariableRow::deleteRequested, this, &MathTraceDialog::onDeleteVariable);

        variableRows.append(row);
        ui->variablesLayout->addWidget(row);
    }
}

void MathTraceDialog::onAddVariableClicked()
{
    VariableRow* row = new VariableRow(this);
    connect(row, &VariableRow::deleteRequested, this, &MathTraceDialog::onDeleteVariable);

    variableRows.append(row);
    ui->variablesLayout->addWidget(row);
}

void MathTraceDialog::onDeleteVariable(VariableRow* row)
{
    // Don't allow deleting if only one row remains
    if (variableRows.size() <= 1)
    {
        QMessageBox::warning(this, "Cannot Delete", "At least one variable must be defined.");
        return;
    }

    variableRows.removeOne(row);
    ui->variablesLayout->removeWidget(row);
    row->deleteLater();
}

void MathTraceDialog::onExpressionChanged()
{
    setStatus("");
}

bool MathTraceDialog::validateInputs()
{
    // Check trace name
    QString traceName = ui->editTraceName->text().trimmed();
    if (traceName.isEmpty())
    {
        setStatus("Error: Trace name cannot be empty", true);
        return false;
    }

    // Check if name already exists (unless in edit mode with same name)
    MathDataSource* mathSource = MathDataSource::getInstance();
    if (mathSource->hasMathSeries(traceName))
    {
        if (!isEditMode || editingSeries->getLabel() != traceName)
        {
            setStatus("Error: A math trace with this name already exists", true);
            return false;
        }
    }

    // Check expression
    QString expression = ui->editExpression->text().trimmed();
    if (expression.isEmpty())
    {
        setStatus("Error: Expression cannot be empty", true);
        return false;
    }

    // Parse expression to check validity
    MathExpressionParser parser;
    if (!parser.parse(expression))
    {
        setStatus(QString("Error: %1").arg(parser.getError()), true);
        return false;
    }

    // Check variables
    if (variableRows.isEmpty())
    {
        setStatus("Error: At least one variable must be defined", true);
        return false;
    }

    // Build variable mapping
    QMap<QString, DataSeriesPointer> variableMapping;
    QSet<QString> usedNames;

    for (VariableRow* row : variableRows)
    {
        QString varName = row->getVariableName();

        if (varName.isEmpty())
        {
            setStatus("Error: Variable name cannot be empty", true);
            return false;
        }

        // Check for duplicate variable names
        if (usedNames.contains(varName))
        {
            setStatus(QString("Error: Duplicate variable name '%1'").arg(varName), true);
            return false;
        }

        usedNames.insert(varName);

        DataSeriesPointer series = row->getSelectedSeries();
        if (!series)
        {
            setStatus(QString("Error: No series selected for variable '%1'").arg(varName), true);
            return false;
        }

        variableMapping[varName] = series;
    }

    // Check if all expression variables are defined
    QStringList expressionVars = parser.getVariables();
    for (const QString& var : expressionVars)
    {
        if (!variableMapping.contains(var))
        {
            setStatus(QString("Error: Variable '%1' used in expression but not defined").arg(var), true);
            return false;
        }
    }

    return true;
}

void MathTraceDialog::accept()
{
    if (!validateInputs())
    {
        return;
    }

    createMathTrace();
}

void MathTraceDialog::createMathTrace()
{
    QString traceName = ui->editTraceName->text().trimmed();
    QString expression = ui->editExpression->text().trimmed();

    // Build variable mapping
    QMap<QString, DataSeriesPointer> variableMapping;
    for (VariableRow* row : variableRows)
    {
        variableMapping[row->getVariableName()] = row->getSelectedSeries();
    }

    // If in edit mode, remove the old series
    MathDataSource* mathSource = MathDataSource::getInstance();
    if (isEditMode && editingSeries)
    {
        mathSource->removeMathSeries(editingSeries->getLabel());
    }

    // Create new MathDataSeries
    MathDataSeriesPointer mathSeries = MathDataSeriesPointer::create(
        traceName,
        expression,
        variableMapping
    );

    // Add to MathDataSource
    mathSource->addMathSeries(mathSeries);
    currentSeries = mathSeries;

    // Set up computation thread
    computeThread = new QThread(this);
    computer = new MathTraceComputer();
    computer->moveToThread(computeThread);

    connect(computer, &MathTraceComputer::computationStarted, this, &MathTraceDialog::onComputationStarted);
    connect(computer, &MathTraceComputer::progressUpdated, this, &MathTraceDialog::onProgressUpdated);
    connect(computer, &MathTraceComputer::computationComplete, this, &MathTraceDialog::onComputationComplete);
    connect(computer, &MathTraceComputer::computationFailed, this, &MathTraceDialog::onComputationFailed);

    connect(computeThread, &QThread::started, computer, &MathTraceComputer::startComputation);

    // Prepare computation
    computer->compute(expression, variableMapping, mathSeries);

    // Disable OK button during computation
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Start computation
    computeThread->start();
}

void MathTraceDialog::onComputationStarted()
{
    setStatus("Computing math trace...");
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
}

void MathTraceDialog::onProgressUpdated(int progress)
{
    ui->progressBar->setValue(progress);
}

void MathTraceDialog::onComputationComplete()
{
    ui->progressBar->setVisible(false);
    setStatus("Math trace created successfully!");

    // Clean up thread
    if (computeThread)
    {
        computeThread->quit();
        computeThread->wait();
    }

    // Close dialog
    QDialog::accept();
}

void MathTraceDialog::onComputationFailed(QString error)
{
    ui->progressBar->setVisible(false);
    setStatus(QString("Computation failed: %1").arg(error), true);

    // Remove the series that was added before computation started
    if (currentSeries)
    {
        MathDataSource::getInstance()->removeMathSeries(currentSeries->getLabel());
        currentSeries.reset();
    }

    // Re-enable OK button
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    // Clean up thread
    if (computeThread)
    {
        computeThread->quit();
        computeThread->wait();
    }
}

void MathTraceDialog::setStatus(const QString& message, bool isError)
{
    ui->labelStatus->setText(message);

    if (isError)
    {
        ui->labelStatus->setStyleSheet("QLabel { color: red; }");
    }
    else
    {
        ui->labelStatus->setStyleSheet("QLabel { color: green; }");
    }
}
