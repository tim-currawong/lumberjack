#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <QColor>
#include <qobject.h>
#include <qvector.h>
#include <QFileInfo>

#include "data_series.hpp"

// Forward declaration
class ColorFamily;


/**
 * @brief The DataSource class defines a "source" of data
 *
 * Each data source contains zero or more TimeSeries data objects,
 * and can also contain other sources, in a hierarchical fashion.
 *
 * This class can be sub-classed by plugins,
 * to enable loading data from custom sources;
 *
 * For example, loading data from non-standard file formats
 */
class DataSource : public QObject
{
    Q_OBJECT

public:

    DataSource(QString source, QString label, QString description = QString());
    virtual ~DataSource();

    QString getIdentifier(void) const;

    QString getSource(void) const { return m_source; }

    QString getLabel(void) const { return m_label; }

    // Descriptive text for this data source (override for custom sources)
    QString getDescription(void) const { return m_description; }

    /* DataSeries access functions */
    int getSeriesCount(void) const { return data_series.size(); }
    QStringList getSeriesLabels(QString filter_string=QString()) const;

    QStringList getGroupLabels(void) const;

    bool addSeries(DataSeriesPointer series, bool auto_color = true);
    bool addSeries(DataSeries* series, bool auto_color=true);
    bool addSeries(QString label, bool auto_color=true);

    DataSeriesPointer getSeriesByIndex(unsigned int index);
    DataSeriesPointer getSeriesByLabel(QString label);

    bool removeSeries(DataSeriesPointer series, bool update = true);
    bool removeSeriesByLabel(QString label, bool update = true);

    void removeAllSeries(bool update = true);

    // Count series in this source that belong to a specific color family
    int countSeriesInFamily(QString familyId) const;

    // Re-apply color family assignments to all series
    void reapplyColorFamilies();

signals:
    void dataChanged(void);

protected:

    //! Data source (e.g plugin name)
    QString m_source;

    //! Text label associated with this DataSource (e.g. filename)
    QString m_label;

    //! Text description associated with this DataSource
    QString m_description;

    //! Circular list of colors to auto-assign to new series
    virtual QList<QColor> getColorWheel(void);

    QColor getNextColor(void);

    //! Cursor for selecting next color
    int color_wheel_cursor = 0;

    // Keep a map of label:series for efficient lookup
    QMap<QString, DataSeriesPointer> data_series;

    // Color family association per series, keyed by series label.
    // Kept here (rather than on DataSeries) because DataSeries is
    // constructed directly by importer plugins - adding fields to it
    // changes its ABI and breaks any plugin not rebuilt against the
    // new layout. See DataSource::countSeriesInFamily/reapplyColorFamilies.
    QMap<QString, QString> seriesColorFamilyId;
};


typedef QSharedPointer<DataSource> DataSourcePointer;


#endif // DATA_SOURCE_H
