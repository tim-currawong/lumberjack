#ifndef MATH_DATA_SOURCE_HPP
#define MATH_DATA_SOURCE_HPP

#include "data_source.hpp"
#include "math_data_series.hpp"

/**
 * @brief The MathDataSource class is a special data source for computed math traces
 *
 * This is a singleton data source that holds all math traces created by the user.
 * It appears as a separate category in the data view.
 */
class MathDataSource : public DataSource
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     */
    static MathDataSource* getInstance();

    /**
     * @brief Add a computed math series
     * @param series The MathDataSeries to add
     * @return true if added successfully
     */
    bool addMathSeries(MathDataSeriesPointer series);

    /**
     * @brief Remove a math series by label
     * @param label Name of the series to remove
     * @return true if removed successfully
     */
    bool removeMathSeries(const QString& label);

    /**
     * @brief Get a math series by label
     * @param label Name of the series
     * @return Pointer to the series, or null if not found
     */
    MathDataSeriesPointer getMathSeries(const QString& label);

    /**
     * @brief Check if a math series with this label exists
     * @param label Name to check
     * @return true if exists
     */
    bool hasMathSeries(const QString& label);

private:
    // Private constructor for singleton
    MathDataSource();

    // Singleton instance
    static MathDataSource* instance;
};

#endif // MATH_DATA_SOURCE_HPP
