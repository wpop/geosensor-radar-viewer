#include "geosensor/ui/MainWindow.h"

#include <QFont>

namespace geosensor::ui
{

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
}

void MainWindow::setupUi()
{
    setWindowTitle("GeoSensor Radar Viewer");
    resize(900, 600);

    titleLabel_ = new QLabel(this);
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setText(
        "GeoSensor Radar Viewer\n\n"
        "C++20 / Qt6 application for georeferenced radar-like sensor data."
    );

    QFont font;
    font.setPointSize(14);
    titleLabel_->setFont(font);

    setCentralWidget(titleLabel_);
}

} // namespace geosensor::ui
