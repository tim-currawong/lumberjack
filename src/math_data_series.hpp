#ifndef MATH_DATA_SERIES_HPP
#define MATH_DATA_SERIES_HPP

#include "data_series.hpp"
#include <QMap>

/**
 * @brief The MathDataSeries class represents a computed data series
 *
 * This class extends DataSeries to store series computed from mathematical
 * expressions applied to other series. It stores:
 * - The mathematical expression used
 * - References to input series
 * - Variable name mappings
 * - The computed result data points
 */
class MathDataSeries : public DataSeries
{
    Q_OBJECT

public:
    /**
     * @brief Create a math data series
     * @param label Name for this computed series
     * @param expression Mathematical expression (e.g., "a - b")
     * @param variableMapping Map of variable names to input series (e.g., {"a": seriesPtr1, "b": seriesPtr2})
     */
    MathDataSeries(QString label,
                   QString expression,
                   QMap<QString, DataSeriesPointer> variableMapping);

    virtual ~MathDataSeries();

    /**
     * @brief Get the mathematical expression
     */
    QString getExpression() const { return mathExpression; }

    /**
     * @brief Get the variable mapping
     */
    QMap<QString, DataSeriesPointer> getVariableMapping() const { return inputSeries; }

    /**
     * @brief Get a specific input series by variable name
     */
    DataSeriesPointer getInputSeries(const QString& variable) const;

    /**
     * @brief Check if this series is computed (always true for MathDataSeries)
     */
    bool isComputed() const { return true; }

private:
    //! The mathematical expression used to compute this series
    QString mathExpression;

    //! Map of variable names to input series
    QMap<QString, DataSeriesPointer> inputSeries;
};

typedef QSharedPointer<MathDataSeries> MathDataSeriesPointer;

#endif // MATH_DATA_SERIES_HPP
