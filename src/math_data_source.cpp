#include "math_data_source.hpp"
#include "data_source_manager.hpp"

// Initialize static instance
MathDataSource* MathDataSource::instance = nullptr;
static DataSourcePointer instancePtr;

MathDataSource::MathDataSource()
    : DataSource("Math Traces", "Math Traces", "Computed mathematical traces")
{
}

MathDataSource* MathDataSource::getInstance()
{
    if (instance == nullptr)
    {
        instance = new MathDataSource();
        instancePtr = DataSourcePointer(instance, [](DataSource*){});  // Empty deleter - we manage lifecycle
    }

    // Always ensure we're registered (handles case where source was deleted from UI)
    DataSourceManager* manager = DataSourceManager::getInstance();
    DataSourcePointer existingSource = manager->getSourceByLabel("Math Traces");

    if (!existingSource)
    {
        // Re-register if we were removed
        manager->addSource(instancePtr);
    }

    return instance;
}

bool MathDataSource::addMathSeries(MathDataSeriesPointer series)
{
    if (!series)
    {
        return false;
    }

    // Use the base class method to add the series
    return addSeries(series, true);  // auto_color = true
}

bool MathDataSource::removeMathSeries(const QString& label)
{
    return removeSeriesByLabel(label, true);
}

MathDataSeriesPointer MathDataSource::getMathSeries(const QString& label)
{
    DataSeriesPointer series = getSeriesByLabel(label);
    if (!series)
    {
        return MathDataSeriesPointer();
    }

    // Cast to MathDataSeriesPointer
    return series.dynamicCast<MathDataSeries>();
}

bool MathDataSource::hasMathSeries(const QString& label)
{
    return getSeriesByLabel(label) != nullptr;
}
