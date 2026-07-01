#include "geosensor/ui/MainWindow.h"

#include "geosensor/coordinates/CoordinateTransform.h"

#include <QFont>
#include <QString>

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
    titleLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    titleLabel_->setContentsMargins(24, 24, 24, 24);

    QFont font;
    font.setFamily("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(11);
    titleLabel_->setFont(font);

    const geosensor::data::SensorOrigin sensorOrigin {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };

    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    const geosensor::coordinates::CoordinateTransform coordinateTransform(
        sensorOrigin
    );

    const geosensor::data::TargetPosition target =
        coordinateTransform.transform(measurement);

    const QString text = QString(
        "GeoSensor Radar Viewer\n"
        "======================\n\n"
        "Demo radar-like measurement\n\n"
        "Sensor origin WGS84:\n"
        "  Latitude:   %1 deg\n"
        "  Longitude:  %2 deg\n"
        "  Altitude:   %3 m\n\n"
        "Raw measurement:\n"
        "  Range:      %4 m\n"
        "  Azimuth:    %5 deg\n"
        "  Elevation:  %6 deg\n"
        "  Intensity:  %7\n\n"
        "Local ENU target position:\n"
        "  East:       %8 m\n"
        "  North:      %9 m\n"
        "  Up:         %10 m\n\n"
        "Approximate WGS84 target position:\n"
        "  Latitude:   %11 deg\n"
        "  Longitude:  %12 deg\n"
        "  Altitude:   %13 m\n\n"
        "Next step: load measurements from CSV."
    )
        .arg(sensorOrigin.latitudeDeg, 0, 'f', 6)
        .arg(sensorOrigin.longitudeDeg, 0, 'f', 6)
        .arg(sensorOrigin.altitudeM, 0, 'f', 2)
        .arg(measurement.rangeM, 0, 'f', 2)
        .arg(measurement.azimuthDeg, 0, 'f', 2)
        .arg(measurement.elevationDeg, 0, 'f', 2)
        .arg(measurement.intensity, 0, 'f', 2)
        .arg(target.enu.eastM, 0, 'f', 2)
        .arg(target.enu.northM, 0, 'f', 2)
        .arg(target.enu.upM, 0, 'f', 2)
        .arg(target.geographic.latitudeDeg, 0, 'f', 8)
        .arg(target.geographic.longitudeDeg, 0, 'f', 8)
        .arg(target.geographic.altitudeM, 0, 'f', 2);

    titleLabel_->setText(text);

    setCentralWidget(titleLabel_);
}

} // namespace geosensor::ui
