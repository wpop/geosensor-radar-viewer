#pragma once

#include <QLabel>
#include <QMainWindow>

namespace geosensor::ui
{

class RadarView;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();

    QLabel* titleLabel_ {};
    RadarView* radarView_ {};
};

} // namespace geosensor::ui
