#include "geosensor/ui/RadarView.h"

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>

namespace geosensor::ui
{

namespace
{

constexpr qreal kMetersPerSceneUnit = 100.0;
constexpr qreal kMaxDisplayRangeM = 1600.0;

} // namespace

RadarView::RadarView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing, true);
    setBackgroundBrush(QColor(226, 234, 236));
    setupScene();
}

void RadarView::setTargets(
    const std::vector<geosensor::data::EnuPosition>& targets
)
{
    targets_ = targets;
    setupScene();
}

void RadarView::setupScene()
{
    // Use a simple radar-like coordinate space centered on the sensor.
    constexpr qreal sceneRadius = kMaxDisplayRangeM / kMetersPerSceneUnit;

    scene_.clear();
    scene_.setBackgroundBrush(QColor(236, 243, 244));
    scene_.setSceneRect(
        -sceneRadius - 1.5,
        -sceneRadius - 1.5,
        (sceneRadius + 1.5) * 2.0,
        (sceneRadius + 1.5) * 2.0
    );

    QPen gridPen(QColor(28, 72, 84), 1.8);
    gridPen.setCosmetic(true);

    QPen ringPen(QColor(44, 95, 108), 1.5, Qt::DashLine);
    ringPen.setCosmetic(true);

    const QBrush sensorBrush(QColor(255, 188, 48));
    QPen sensorPen(QColor(112, 76, 0), 1.8);
    sensorPen.setCosmetic(true);

    scene_.addLine(-sceneRadius, 0.0, sceneRadius, 0.0, gridPen);
    scene_.addLine(0.0, -sceneRadius, 0.0, sceneRadius, gridPen);

    for (qreal rangeM : {500.0, 1000.0, 1500.0}) {
        const qreal ringRadius = rangeM / kMetersPerSceneUnit;
        scene_.addEllipse(
            -ringRadius,
            -ringRadius,
            ringRadius * 2.0,
            ringRadius * 2.0,
            ringPen
        );
    }

    scene_.addEllipse(-1.0, -1.0, 2.0, 2.0, sensorPen, sensorBrush);

    drawTargets();

    fitSceneInView();
}

void RadarView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitSceneInView();
}

void RadarView::fitSceneInView()
{
    centerOn(0.0, 0.0);
    fitInView(scene_.sceneRect(), Qt::KeepAspectRatio);
}

void RadarView::drawTargets()
{
    const QBrush targetBrush(QColor(210, 62, 43));
    QPen targetPen(QColor(102, 24, 18), 1.2);
    targetPen.setCosmetic(true);

    for (const auto& target : targets_) {
        const qreal x = target.eastM / kMetersPerSceneUnit;
        const qreal y = -target.northM / kMetersPerSceneUnit;

        scene_.addEllipse(x - 0.85, y - 0.85, 1.7, 1.7, targetPen, targetBrush);
    }
}

} // namespace geosensor::ui
