#include "math_data_series.hpp"

MathDataSeries::MathDataSeries(QString label,
                               QString expression,
                               QMap<QString, DataSeriesPointer> variableMapping)
    : DataSeries("Math Traces", label),
      mathExpression(expression),
      inputSeries(variableMapping)
{
}

MathDataSeries::~MathDataSeries()
{
}

DataSeriesPointer MathDataSeries::getInputSeries(const QString& variable) const
{
    if (inputSeries.contains(variable))
    {
        return inputSeries[variable];
    }
    return DataSeriesPointer();
}
