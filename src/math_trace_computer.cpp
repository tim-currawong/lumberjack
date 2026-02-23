#include "math_trace_computer.hpp"
#include <QElapsedTimer>
#include <QDebug>
#include <algorithm>
#include <set>

MathTraceComputer::MathTraceComputer()
    : cancelRequested(false)
{
}

MathTraceComputer::~MathTraceComputer()
{
}

/**
 * @brief Set up computation parameters (call from main thread before starting worker thread)
 *
 * @param expression Mathematical expression to evaluate
 * @param variableMapping Map of variable names to input data series
 * @param outputSeries Series to populate with computed results
 * @param maxGapSize Maximum gap in milliseconds to interpolate across (default: 1000ms)
 */
void MathTraceComputer::compute(const QString& expression,
                                const QMap<QString, DataSeriesPointer>& variableMapping,
                                MathDataSeriesPointer outputSeries,
                                double maxGapSize)
{
    QMutexLocker locker(&computeMutex);

    currentExpression = expression;
    currentVariableMapping = variableMapping;
    currentOutputSeries = outputSeries;
    currentMaxGapSize = maxGapSize;
    cancelRequested = false;
}

/**
 * @brief Execute the math trace computation (runs in background thread)
 *
 * Algorithm:
 * 1. Parse and validate expression
 * 2. Collect all unique timestamps from input series (creates timestamp union)
 * 3. For each timestamp:
 *    - Check if it's in a valid region (not in large gap)
 *    - Interpolate values from all input series at that timestamp
 *    - Evaluate expression with interpolated values
 *    - Add result to output series (if valid)
 * 4. Emit progress updates periodically
 *
 * This creates a new series with timestamps from ALL input series combined,
 * preserving maximum data fidelity.
 */
void MathTraceComputer::startComputation()
{
    QElapsedTimer timer;
    timer.start();

    emit computationStarted();

    // Parse the expression
    MathExpressionParser parser;
    if (!parser.parse(currentExpression))
    {
        emit computationFailed(QString("Failed to parse expression: %1").arg(parser.getError()));
        return;
    }

    // Verify all required variables are provided
    QStringList requiredVars = parser.getVariables();
    for (const QString& var : requiredVars)
    {
        if (!currentVariableMapping.contains(var))
        {
            emit computationFailed(QString("Variable '%1' not found in mapping").arg(var));
            return;
        }
    }

    // Check if any input series is empty
    for (const QString& var : requiredVars)
    {
        if (currentVariableMapping[var]->size() == 0)
        {
            emit computationFailed(QString("Input series for variable '%1' is empty").arg(var));
            return;
        }
    }

    // Collect all unique timestamps from input series
    qDebug() << "Collecting timestamps from input series...";
    QVector<double> timestamps = collectTimestamps(currentVariableMapping);

    if (timestamps.isEmpty())
    {
        emit computationFailed("No timestamps found in input series");
        return;
    }

    qDebug() << "Found" << timestamps.size() << "unique timestamps";

    // Clear existing data in output series
    currentOutputSeries->clearData(false);

    // Main computation loop: evaluate expression at each timestamp
    int lastProgress = -1;
    int validPoints = 0;
    int skippedPoints = 0;

    for (int i = 0; i < timestamps.size(); ++i)
    {
        // Check for cancellation request from user
        if (cancelRequested)
        {
            emit computationFailed("Computation cancelled");
            return;
        }

        double timestamp = timestamps[i];

        // Skip timestamps in large gaps (prevents wild interpolation across disconnected regions)
        if (!isTimestampValid(timestamp, currentVariableMapping, currentMaxGapSize))
        {
            skippedPoints++;
            continue;
        }

        // Interpolate values for each variable at this timestamp using linear interpolation
        QMap<QString, double> variableValues;
        bool allValid = true;

        for (const QString& var : requiredVars)
        {
            DataSeriesPointer series = currentVariableMapping[var];
            double value = series->getValueAtTime(timestamp, DataSeries::INTERPOLATE);

            // Check for NaN or Inf (can occur at boundaries or with invalid data)
            if (std::isnan(value) || std::isinf(value))
            {
                allValid = false;
                break;
            }

            variableValues[var] = value;
        }

        if (!allValid)
        {
            skippedPoints++;
            continue;
        }

        // Evaluate the expression
        double result = 0.0;
        if (!parser.evaluate(variableValues, result))
        {
            skippedPoints++;
            continue;
        }

        // Check if result is valid
        if (std::isnan(result) || std::isinf(result))
        {
            skippedPoints++;
            continue;
        }

        // Add computed point to output series
        currentOutputSeries->addData(timestamp, result, false);
        validPoints++;

        // Report progress periodically (every 10%)
        int progress = (i * 100) / timestamps.size();
        if (progress != lastProgress && progress % 10 == 0)
        {
            emit progressUpdated(progress);
            lastProgress = progress;
        }
    }

    // Trigger data update on the output series
    currentOutputSeries->update();

    if (validPoints == 0)
    {
        emit computationFailed(QString("Expression produced no valid results (%1 points skipped) - check for division by zero or invalid operations").arg(skippedPoints));
        return;
    }

    qDebug() << "Math trace computation complete:";
    qDebug() << "  - Time elapsed:" << timer.elapsed() << "ms";
    qDebug() << "  - Valid points:" << validPoints;
    qDebug() << "  - Skipped points:" << skippedPoints;
    qDebug() << "  - Total timestamps:" << timestamps.size();

    emit progressUpdated(100);
    emit computationComplete();
}

void MathTraceComputer::cancelComputation()
{
    QMutexLocker locker(&computeMutex);
    cancelRequested = true;
}

/**
 * @brief Collect all unique timestamps from all input series
 *
 * Creates the timestamp union - the set of all timestamps that appear in ANY input series.
 * This ensures we preserve all data points from all inputs.
 *
 * Example:
 *   Series A timestamps: [0, 10, 20]
 *   Series B timestamps: [5, 15, 25]
 *   Result: [0, 5, 10, 15, 20, 25]  (sorted, unique)
 *
 * @param series Map of input series
 * @return Sorted vector of all unique timestamps (in milliseconds)
 */
QVector<double> MathTraceComputer::collectTimestamps(const QMap<QString, DataSeriesPointer>& series)
{
    // Use std::set for automatic sorting and deduplication (O(log n) insert)
    std::set<double> timestampSet;

    // Collect all timestamps from all input series
    for (auto it = series.begin(); it != series.end(); ++it)
    {
        DataSeriesPointer s = it.value();

        for (uint64_t i = 0; i < s->size(); ++i)
        {
            double timestamp = s->getTimestamp(i);
            timestampSet.insert(timestamp);  // Duplicates automatically ignored
        }
    }

    // Convert set to vector (already sorted by std::set)
    QVector<double> timestamps;
    timestamps.reserve(timestampSet.size());

    for (double timestamp : timestampSet)
    {
        timestamps.append(timestamp);
    }

    return timestamps;
}

/**
 * @brief Check if a timestamp is valid for computation (not in a large gap)
 *
 * This prevents wildly inaccurate interpolation across disconnected data regions.
 *
 * A timestamp is considered INVALID if it falls in a gap larger than maxGapSize
 * in ANY input series. This ensures we only compute where we have reasonable
 * data coverage.
 *
 * Example (maxGapSize = 1000ms):
 *   Series A: [0ms, 10ms, 2000ms, 2010ms]  (gap of 1990ms between 10 and 2000)
 *   Timestamp 1000ms: INVALID (in the large gap)
 *   Timestamp 5ms: VALID (between 0 and 10, gap only 10ms)
 *   Timestamp 2005ms: VALID (between 2000 and 2010, gap only 10ms)
 *
 * @param timestamp The timestamp to check (milliseconds)
 * @param series Map of input series to check
 * @param maxGapSize Maximum acceptable gap size (milliseconds)
 * @return true if timestamp is valid for computation, false if in large gap
 */
bool MathTraceComputer::isTimestampValid(double timestamp,
                                        const QMap<QString, DataSeriesPointer>& series,
                                        double maxGapSize)
{
    // Check each input series - timestamp must be valid in ALL series
    for (auto it = series.begin(); it != series.end(); ++it)
    {
        DataSeriesPointer s = it.value();

        // Find the index where this timestamp would be inserted (binary search)
        uint64_t idx = s->getIndexForTimestamp(timestamp);

        // Handle boundary cases (before first point or after last point)
        if (idx == 0 || idx >= s->size())
        {
            // Check distance to nearest actual data point
            if (idx == 0 && s->size() > 0)
            {
                double nearestTime = s->getTimestamp(0);
                if (fabs(timestamp - nearestTime) > maxGapSize)
                {
                    return false;
                }
            }
            else if (idx >= s->size() && s->size() > 0)
            {
                double nearestTime = s->getTimestamp(s->size() - 1);
                if (fabs(timestamp - nearestTime) > maxGapSize)
                {
                    return false;
                }
            }
        }
        else
        {
            // Check gap size around this timestamp
            double timeBefore = s->getTimestamp(idx - 1);
            double timeAfter = s->getTimestamp(idx);
            double gapSize = timeAfter - timeBefore;

            if (gapSize > maxGapSize)
            {
                return false;
            }
        }
    }

    return true;
}
