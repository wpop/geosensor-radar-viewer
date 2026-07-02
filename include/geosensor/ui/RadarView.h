#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>

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

private:
    void setupScene();

    QGraphicsScene scene_ {};
};

} // namespace geosensor::ui
