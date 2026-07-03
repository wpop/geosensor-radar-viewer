#include "geosensor/ui/RadarView.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QString>
#include <QStringList>

namespace geosensor::ui
{

namespace
{

constexpr qreal kMetersPerSceneUnit = 100.0;
constexpr qreal kMaxDisplayRangeM = 1600.0;
constexpr qreal kScenePadding = 6.0;

} // namespace

RadarView::RadarView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing, true);
    setBackgroundBrush(QColor(226, 234, 236));
    setupScene();
}

void RadarView::setSampleTargets(
    const std::vector<geosensor::data::EnuPosition>& targets
)
{
    sampleTargets_ = targets;
    setupScene();
}

void RadarView::setLiveTargets(
    const std::vector<geosensor::data::EnuPosition>& targets
)
{
    liveTargets_ = targets;
    setupScene();
}

void RadarView::setTargetTrails(
    const std::vector<std::vector<geosensor::data::EnuPosition>>& trails
)
{
    targetTrails_ = trails;
    setupScene();
}

void RadarView::setupScene()
{
    // Use a simple radar-like coordinate space centered on the sensor.
    constexpr qreal sceneRadius = kMaxDisplayRangeM / kMetersPerSceneUnit;

    scene_.clear();
    scene_.setBackgroundBrush(QColor(236, 243, 244));
    scene_.setSceneRect(
        -sceneRadius - kScenePadding,
        -sceneRadius - kScenePadding,
        (sceneRadius + kScenePadding) * 2.0,
        (sceneRadius + kScenePadding) * 2.0
    );

    QPen gridPen(QColor(28, 72, 84), 1.8);
    gridPen.setCosmetic(true);

    QPen ringPen(QColor(44, 95, 108), 1.5, Qt::DashLine);
    ringPen.setCosmetic(true);

    const QBrush sensorBrush(QColor(255, 188, 48));
    QPen sensorPen(QColor(112, 76, 0), 1.8);
    sensorPen.setCosmetic(true);

    QFont labelFont;
    labelFont.setPointSize(9);
    QFont legendFont = labelFont;
    legendFont.setPointSize(8);

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

        auto* label = scene_.addText(
            QString("%1 m").arg(static_cast<int>(rangeM)),
            labelFont
        );
        label->setDefaultTextColor(QColor(56, 76, 84));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        label->setZValue(1.0);
        label->setPos(ringRadius + 0.2, -0.8);
    }

    scene_.addEllipse(-1.0, -1.0, 2.0, 2.0, sensorPen, sensorBrush);

    drawSampleTargets();
    drawTargetTrails();
    drawLiveTargets();

    const qreal legendLeft = -sceneRadius - kScenePadding + 1.2;
    const qreal legendTop = -sceneRadius - kScenePadding + 1.2;
    const QStringList legendLines {
        "Legend:",
        "Sensor: yellow",
        "CSV/sample: gray",
        "Live UDP: red",
        "Trails: dark red",
        "Rings: range"
    };

    for (int i = 0; i < legendLines.size(); ++i) {
        auto* legendLabel = scene_.addText(legendLines[i], legendFont);
        legendLabel->setDefaultTextColor(QColor(56, 76, 84));
        legendLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        legendLabel->setZValue(1.0);
        legendLabel->setPos(legendLeft, legendTop + static_cast<qreal>(i) * 1.2);
    }

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

void RadarView::drawSampleTargets()
{
    const QBrush sampleBrush(QColor(120, 132, 138));
    QPen samplePen(QColor(72, 82, 88), 1.0);
    samplePen.setCosmetic(true);

    for (const auto& target : sampleTargets_) {
        const qreal x = target.eastM / kMetersPerSceneUnit;
        const qreal y = -target.northM / kMetersPerSceneUnit;

        scene_.addEllipse(x - 0.55, y - 0.55, 1.1, 1.1, samplePen, sampleBrush);
    }
}

void RadarView::drawTargetTrails()
{
    QPen trailPen(QColor(144, 36, 27, 180), 1.5);
    trailPen.setCosmetic(true);

    for (const auto& trail : targetTrails_) {
        if (trail.size() < 2) {
            continue;
        }

        for (std::size_t i = 1; i < trail.size(); ++i) {
            const qreal x1 = trail[i - 1].eastM / kMetersPerSceneUnit;
            const qreal y1 = -trail[i - 1].northM / kMetersPerSceneUnit;
            const qreal x2 = trail[i].eastM / kMetersPerSceneUnit;
            const qreal y2 = -trail[i].northM / kMetersPerSceneUnit;

            scene_.addLine(x1, y1, x2, y2, trailPen);
        }
    }
}

void RadarView::drawLiveTargets()
{
    const QBrush liveBrush(QColor(210, 62, 43));
    QPen livePen(QColor(102, 24, 18), 1.2);
    livePen.setCosmetic(true);

    for (const auto& target : liveTargets_) {
        const qreal x = target.eastM / kMetersPerSceneUnit;
        const qreal y = -target.northM / kMetersPerSceneUnit;

        scene_.addEllipse(x - 0.85, y - 0.85, 1.7, 1.7, livePen, liveBrush);
    }
}

} // namespace geosensor::ui
