#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "data_source.hpp"
#include "color_family_manager.hpp"


DataSource::DataSource(QString source, QString label, QString description) :
    m_source(source),
    m_label(label),
    m_description(description)
{
}


DataSource::~DataSource()
{
    removeAllSeries(false);
}


/*
 * Return a unique identifier string for this data source
 */
QString DataSource::getIdentifier(void) const
{
    // TODO: Maybe a "better" approach to this in the future?
    return m_source + ":" + m_label + ":" + m_description;
}


/**
 * @brief DataSource::getColorWheel creates a list of "distinct" colors
 * @return a list of colors
 */
QList<QColor> DataSource::getColorWheel()
{
    QList<QColor> colors;

    // TODO: Make this ... better? ...

    colors.append(QColor(0, 0, 250));
    colors.append(QColor(0, 250, 0));
    colors.append(QColor(250, 0, 0));
    colors.append(QColor(0, 250, 250));
    colors.append(QColor(250, 250, 0));
    colors.append(QColor(250, 0, 250));
    colors.append(QColor(0, 0, 0));

    return colors;
}


/**
 * @brief DataSource::getNextColor get the next available color
 * @return
 */
QColor DataSource::getNextColor()
{
    auto colors = getColorWheel();

    if (colors.count() == 0)
    {
        return QColor(0, 0, 0);
    }

    int idx = color_wheel_cursor;

    if (idx >= colors.count()) idx = 0;

    QColor color = colors.at(idx);

    color_wheel_cursor = (idx + 1);

    return color;
}


/**
 * @brief DataSource::getSeriesLabels returns a list of labels for the DataSeries contained within this source
 * @param filter_string an (optional) wildcard filter to limit the returned labels
 * @return a list of zero or more labels
 */
QStringList DataSource::getSeriesLabels(QString filter_string) const
{
    QStringList labels;

    QList<QRegularExpression> patterns;

    for (QString pattern : filter_string.trimmed().split(" "))
    {
        if (!pattern.startsWith("*"))
        {
            pattern.prepend("*");
        }

        if (!pattern.endsWith("*"))
        {
            pattern.append("*");
        }

        // Convert from wildcard to regular expression
        QString re_pattern = QRegularExpression::wildcardToRegularExpression(pattern);
        patterns.append(QRegularExpression(re_pattern, QRegularExpression::CaseInsensitiveOption));
    }

    for (auto series : data_series)
    {
        if (series.isNull())
        {
            continue;
        }

        QString label = (*series).getLabel();

        // Test if the label matches *ALL* the filters
        bool matches_all = true;

        for (auto pattern : patterns)
        {
            if (!pattern.match(label).hasMatch())
            {
                matches_all = false;
                break;
            }
        }

        if (matches_all)
        {
            labels.append(label);
        }
    }

    return labels;
}


/*
 * Add the provided DataSeries to the list of series associated with this source.
 * If the series is null, or already present, it will not be added.
 *
 * Returns true if the series was added, else false
 */
bool DataSource::addSeries(DataSeriesPointer series, bool auto_color)
{
    if (series.isNull())
    {
        qWarning() << "Attempting to add null DataSeries";
        return false;
    }

    for (auto s : data_series.values())
    {
        if (s == series)
        {
            qWarning() << "Attempting to add duplicate DataSeries";
            return false;
        }
    }

    data_series[series->getLabel()] = series;

    if (auto_color)
    {
        // Try to assign color using color family system
        ColorFamilyManager* cfm = ColorFamilyManager::getInstance();
        ColorFamily* family = cfm->matchSeries(series->getLabel());

        if (family)
        {
            // Count how many series from this source are already in this family
            int seriesIndexInFamily = countSeriesInFamily(family->getId());

            // Get color from family
            QColor color = cfm->assignColor(series->getLabel(), this, seriesIndexInFamily);
            series->setColor(color);
            series->setColorFamily(family->getId(), seriesIndexInFamily);
        }
        else
        {
            // No family match - use traditional color wheel or default palette
            ColorFamilyConfiguration* config = cfm->getActiveConfiguration();
            if (config)
            {
                // Use default palette from configuration
                QString paletteName = config->getDefaultPaletteName();
                QList<QColor> palette = cfm->getDefaultPaletteColors(paletteName);

                if (!palette.isEmpty())
                {
                    int index = color_wheel_cursor % palette.count();
                    series->setColor(palette.at(index));
                    color_wheel_cursor++;
                }
                else
                {
                    series->setColor(getNextColor());
                }
            }
            else
            {
                // No configuration active - fall back to original color wheel
                series->setColor(getNextColor());
            }
        }
    }

    emit dataChanged();

    return true;
}


bool DataSource::addSeries(DataSeries *series, bool auto_color)
{
    return addSeries(DataSeriesPointer(series), auto_color);
}


bool DataSource::addSeries(QString label, bool auto_color)
{
    if (!getSeriesByLabel(label).isNull())
    {
        qWarning() << "Attempted to add duplicate series with label" << label;
        return false;
    }

    DataSeries* series = new DataSeries(label);

    return addSeries(series, auto_color);
}


/*
 * Return a pointer to the DataSeries at the specified index.
 *
 * If the index is out of bounds, return a null DataSeries pointer
 */
DataSeriesPointer DataSource::getSeriesByIndex(unsigned int index)
{
    QList<QString> keys = data_series.keys();

    if (index < (unsigned int) keys.length())
    {
        return data_series[keys[index]];
    }

    // No match found
    return DataSeriesPointer(nullptr);
}


bool DataSource::removeSeries(DataSeriesPointer series, bool update)
{
    for (QString label : data_series.keys())
    {
        if (data_series.value(label) == series)
        {
            data_series.remove(label);

            if (update)
            {
                emit dataChanged();
            }

            return true;
        }
    }

    // No matching series found
    return false;
}


/*
 * Return a pointer to the DataSeries with the specified label.
 *
 * If the label is not found, return a null DataSeries pointer
 */
DataSeriesPointer DataSource::getSeriesByLabel(QString label)
{
    if (data_series.contains(label))
    {
        return data_series[label];
    }

    // No match found - return a null series
    return DataSeriesPointer(nullptr);
}


bool DataSource::removeSeriesByLabel(QString label, bool update)
{
    if (data_series.contains(label))
    {
        data_series.remove(label);

        if (update)
        {
            emit dataChanged();
        }

        return true;
    }
    else
    {
        return false;
    }
}


void DataSource::removeAllSeries(bool update)
{
    data_series.clear();

    if (update)
    {
        emit dataChanged();
    }
}


/*
 * Construct a list of all "group" labels within this dataset
 */
QStringList DataSource::getGroupLabels() const
{
    QStringList labels;

    for (auto series : data_series)
    {
        if (!series.isNull() && !labels.contains(series->getGroup()))
        {
            labels.append(series->getGroup());
        }
    }

    return labels;
}

/*
 * Count how many series from this source belong to a specific color family
 */
int DataSource::countSeriesInFamily(QString familyId) const
{
    int count = 0;

    for (auto series : data_series.values())
    {
        if (!series.isNull() && series->getColorFamilyId() == familyId)
        {
            count++;
        }
    }

    return count;
}


