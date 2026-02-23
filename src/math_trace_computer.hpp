#ifndef MATH_TRACE_COMPUTER_HPP
#define MATH_TRACE_COMPUTER_HPP

#include <QObject>
#include <QThread>
#include <QMutex>
#include "data_series.hpp"
#include "math_data_series.hpp"
#include "math_expression_parser.hpp"

/**
 * @brief The MathTraceComputer class performs background computation of math traces
 *
 * This class runs in a background thread and:
 * 1. Merges timestamps from all input series
 * 2. Interpolates values at each timestamp
 * 3. Evaluates the mathematical expression
 * 4. Populates the output MathDataSeries with computed points
 *
 * Large gaps in data are handled by not interpolating across them.
 */
class MathTraceComputer : public QObject
{
    Q_OBJECT

public:
    MathTraceComputer();
    virtual ~MathTraceComputer();

    /**
     * @brief Compute a math trace from input series and expression
     * @param expression Mathematical expression to evaluate
     * @param variableMapping Map of variable names to input series
     * @param outputSeries The series to populate with computed results
     * @param maxGapSize Maximum gap (in ms) to interpolate across. Larger gaps are left empty.
     */
    void compute(const QString& expression,
                 const QMap<QString, DataSeriesPointer>& variableMapping,
                 MathDataSeriesPointer outputSeries,
                 double maxGapSize = 1000.0);  // Default: 1 second

signals:
    /**
     * @brief Emitted when computation starts
     */
    void computationStarted();

    /**
     * @brief Emitted periodically during computation to report progress
     * @param progress Progress percentage (0-100)
     */
    void progressUpdated(int progress);

    /**
     * @brief Emitted when computation completes successfully
     */
    void computationComplete();

    /**
     * @brief Emitted when computation fails
     * @param error Error message
     */
    void computationFailed(QString error);

public slots:
    /**
     * @brief Start the computation (call this from the worker thread)
     */
    void startComputation();

    /**
     * @brief Cancel ongoing computation
     */
    void cancelComputation();

private:
    /**
     * @brief Collect all unique timestamps from input series
     * @return Sorted list of timestamps
     */
    QVector<double> collectTimestamps(const QMap<QString, DataSeriesPointer>& series);

    /**
     * @brief Check if a timestamp should be skipped due to large gaps
     * @param timestamp Current timestamp
     * @param series Input series to check
     * @param maxGapSize Maximum allowed gap
     * @return true if timestamp is within valid data regions
     */
    bool isTimestampValid(double timestamp,
                         const QMap<QString, DataSeriesPointer>& series,
                         double maxGapSize);

    QString currentExpression;
    QMap<QString, DataSeriesPointer> currentVariableMapping;
    MathDataSeriesPointer currentOutputSeries;
    double currentMaxGapSize;

    QMutex computeMutex;
    bool cancelRequested;
};

#endif // MATH_TRACE_COMPUTER_HPP
