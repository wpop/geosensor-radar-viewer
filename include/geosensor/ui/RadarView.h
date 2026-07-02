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
    explicit RadarView(QWidget* parent = nullptr);

    void setTargets(const std::vector<geosensor::data::EnuPosition>& targets);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupScene();
    void fitSceneInView();
    void drawTargets();

    QGraphicsScene scene_ {};
    std::vector<geosensor::data::EnuPosition> targets_ {};
};

} // namespace geosensor::ui
