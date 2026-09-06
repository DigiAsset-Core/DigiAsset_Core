//
// Created by mctrivia on 21/12/24.
//

#include "PlotManager.h"
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts; //Qt6 moved the charts classes to the global namespace
#endif
using namespace std;

PlotManager::PlotManager(QWidget *parent)
    : QWidget(parent)
{
    // Create the scroll area and a widget to hold the charts
    _scrollArea = new QScrollArea(this);
    _scrollWidget = new QWidget(_scrollArea);
    _vLayout = new QVBoxLayout(_scrollWidget);
    _scrollWidget->setLayout(_vLayout);

    // Make the scroll area resizable
    _scrollArea->setWidget(_scrollWidget);
    _scrollArea->setWidgetResizable(true);

    // Main layout of this widget
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(_scrollArea);
    setLayout(mainLayout);
}

void PlotManager::addGraph(const QVector<double> &xData,
                           const QVector<double> &yData,
                           const QString &xLabel,
                           const QString &yLabel,
                           const QString &title)
{
    // Prepare the data series
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < xData.size() && i < yData.size(); ++i) {
        series->append(xData[i], yData[i]);
    }

    // Create a chart and add the series
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(title);

    // Create X and Y axes
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText(xLabel);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(yLabel);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Display the chart in a chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Add the chart view to the vertical layout
    _vLayout->addWidget(chartView);
}
