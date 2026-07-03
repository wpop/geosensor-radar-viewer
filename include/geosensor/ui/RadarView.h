#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>

#include <vector>

#include "geosensor/data/TargetPosition.h"

class QResizeEvent;

namespace geosensor::ui
{

// Simple radar-style 2D view.
//
// The first version draws static reference elements:
// - sensor position at the center
// - crosshair axes
// - range rings
//
// Later this widget will display transformed ENU target positions.
class RadarView : public QGraphicsView
{
public:
    // Creates the radar view widget.
    explicit RadarView(QWidget* parent = nullptr);

    // Updates the CSV or sample target layer.
    void setSampleTargets(
        const std::vector<geosensor::data::EnuPosition>& targets
    );

    // Updates the live UDP target layer.
    void setLiveTargets(
        const std::vector<geosensor::data::EnuPosition>& targets
    );

    // Updates the tracked target trail layer.
    void setTargetTrails(
        const std::vector<std::vector<geosensor::data::EnuPosition>>& trails
    );

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupScene();
    void fitSceneInView();
    void drawSampleTargets();
    void drawTargetTrails();
    void drawLiveTargets();

    QGraphicsScene scene_ {};
    std::vector<geosensor::data::EnuPosition> sampleTargets_ {};
    std::vector<geosensor::data::EnuPosition> liveTargets_ {};
    std::vector<std::vector<geosensor::data::EnuPosition>> targetTrails_ {};
};

} // namespace geosensor::ui
