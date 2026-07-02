#include "geosensor/ui/RadarView.h"

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPen>

namespace geosensor::ui
{

RadarView::RadarView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing, true);
    setupScene();
}

void RadarView::setupScene()
{
    // Use a simple radar-like coordinate space centered on the sensor.
    constexpr qreal sceneRadius = 150.0;
    constexpr qreal centerX = 0.0;
    constexpr qreal centerY = 0.0;

    scene_.clear();
    scene_.setSceneRect(
        -sceneRadius - 20.0,
        -sceneRadius - 20.0,
        (sceneRadius + 20.0) * 2.0,
        (sceneRadius + 20.0) * 2.0
    );

    QPen gridPen(QColor(90, 170, 190), 1.0);
    gridPen.setCosmetic(true);

    QPen ringPen(QColor(90, 170, 190), 1.0, Qt::DashLine);
    ringPen.setCosmetic(true);

    const QBrush sensorBrush(QColor(255, 210, 90));
    QPen sensorPen(QColor(140, 100, 0), 1.5);
    sensorPen.setCosmetic(true);

    scene_.addLine(-sceneRadius, 0.0, sceneRadius, 0.0, gridPen);
    scene_.addLine(0.0, -sceneRadius, 0.0, sceneRadius, gridPen);

    for (qreal ringRadius : {50.0, 100.0, 150.0}) {
        scene_.addEllipse(
            -ringRadius,
            -ringRadius,
            ringRadius * 2.0,
            ringRadius * 2.0,
            ringPen
        );
    }

    scene_.addEllipse(-6.0, -6.0, 12.0, 12.0, sensorPen, sensorBrush);

    centerOn(centerX, centerY);
    fitInView(scene_.sceneRect(), Qt::KeepAspectRatio);
}

} // namespace geosensor::ui
