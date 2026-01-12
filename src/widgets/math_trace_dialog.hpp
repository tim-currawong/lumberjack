#ifndef MATH_TRACE_DIALOG_HPP
#define MATH_TRACE_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QThread>
#include <QMap>

#include "data_series.hpp"
#include "math_data_series.hpp"
#include "math_trace_computer.hpp"

namespace Ui {
class MathTraceDialog;
}

/**
 * @brief Widget for a single variable row (variable name + series selector + delete button)
 */
class VariableRow : public QWidget
{
    Q_OBJECT

public:
    VariableRow(QWidget* parent = nullptr);
    ~VariableRow();

    QString getVariableName() const;
    void setVariableName(const QString& name);

    DataSeriesPointer getSelectedSeries() const;
    void setSelectedSeries(const QString& sourceLabel, const QString& seriesLabel);

    void populateSeriesComboBox();

signals:
    void deleteRequested(VariableRow* row);

private slots:
    void onDeleteClicked();

private:
    QLineEdit* editVariableName;
    QComboBox* comboSeries;
    QPushButton* btnDelete;

    // Store series pointers for combo box items
    QMap<int, DataSeriesPointer> seriesMap;
};

/**
 * @brief The MathTraceDialog class provides UI for creating math traces
 */
class MathTraceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MathTraceDialog(QWidget *parent = nullptr);
    ~MathTraceDialog();

    /**
     * @brief Set the dialog to edit an existing math trace
     * @param series The existing MathDataSeries to edit
     */
    void setEditMode(MathDataSeriesPointer series);

protected:
    void accept() override;

private slots:
    void onAddVariableClicked();
    void onDeleteVariable(VariableRow* row);
    void onExpressionChanged();

    void onComputationStarted();
    void onProgressUpdated(int progress);
    void onComputationComplete();
    void onComputationFailed(QString error);

private:
    bool validateInputs();
    void createMathTrace();
    void setStatus(const QString& message, bool isError = false);

    Ui::MathTraceDialog *ui;

    QList<VariableRow*> variableRows;

    // Background computation
    QThread* computeThread;
    MathTraceComputer* computer;

    // Edit mode
    bool isEditMode;
    MathDataSeriesPointer editingSeries;
};

#endif // MATH_TRACE_DIALOG_HPP
