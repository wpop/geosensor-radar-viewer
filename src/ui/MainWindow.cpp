#include "geosensor/ui/MainWindow.h"

#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/io/CsvMeasurementLoader.h"
#include "geosensor/ui/RadarView.h"

#include <QHBoxLayout>
#include <QFont>
#include <QString>
#include <QWidget>

#include <exception>
#include <filesystem>

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
    resize(1000, 700);

    auto* centralWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(centralWidget);

    titleLabel_ = new QLabel(centralWidget);
    titleLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    titleLabel_->setContentsMargins(24, 24, 24, 24);
    titleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QFont font;
    font.setFamily("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(10);
    titleLabel_->setFont(font);

    radarView_ = new RadarView(centralWidget);

    const geosensor::data::SensorOrigin sensorOrigin {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };

    try {
        const std::filesystem::path csvPath {
            "data/samples/measurements.csv"
        };

        const geosensor::io::CsvMeasurementLoader loader;
        const auto measurements = loader.load(csvPath);

        QString measurementRows;

        for (std::size_t i = 0; i < measurements.size(); ++i) {
            const auto& measurement = measurements[i];

            measurementRows += QString(
                "%1) range=%2 m, azimuth=%3 deg, elevation=%4 deg, intensity=%5\n"
            )
                .arg(static_cast<int>(i + 1))
                .arg(measurement.rangeM, 0, 'f', 1)
                .arg(measurement.azimuthDeg, 0, 'f', 1)
                .arg(measurement.elevationDeg, 0, 'f', 1)
                .arg(measurement.intensity, 0, 'f', 2);
        }

        QString firstTargetText;

        if (!measurements.empty()) {
            const geosensor::coordinates::CoordinateTransform transform(
                sensorOrigin
            );

            const geosensor::data::TargetPosition firstTarget =
                transform.transform(measurements.front());

            firstTargetText = QString(
                "First transformed target:\n"
                "  ENU East:     %1 m\n"
                "  ENU North:    %2 m\n"
                "  ENU Up:       %3 m\n"
                "  Latitude:     %4 deg\n"
                "  Longitude:    %5 deg\n"
                "  Altitude:     %6 m\n"
            )
                .arg(firstTarget.enu.eastM, 0, 'f', 2)
                .arg(firstTarget.enu.northM, 0, 'f', 2)
                .arg(firstTarget.enu.upM, 0, 'f', 2)
                .arg(firstTarget.geographic.latitudeDeg, 0, 'f', 8)
                .arg(firstTarget.geographic.longitudeDeg, 0, 'f', 8)
                .arg(firstTarget.geographic.altitudeM, 0, 'f', 2);
        } else {
            firstTargetText = "No measurements loaded.\n";
        }

        const QString text = QString(
            "GeoSensor Radar Viewer\n"
            "======================\n\n"
            "Loaded CSV file:\n"
            "  Path:         %1\n"
            "  Measurements: %2\n\n"
            "Raw CSV measurements:\n"
            "%3\n"
            "%4\n"
            "Next step: display these targets on a 2D radar scene."
        )
            .arg(QString::fromStdString(csvPath.string()))
            .arg(static_cast<int>(measurements.size()))
            .arg(measurementRows)
            .arg(firstTargetText);

        titleLabel_->setText(text);
    } catch (const std::exception& error) {
        titleLabel_->setText(
            QString("GeoSensor Radar Viewer\n\nError: %1")
                .arg(error.what())
        );
    }

    layout->addWidget(titleLabel_, 1);
    layout->addWidget(radarView_, 1);

    setCentralWidget(centralWidget);
}

} // namespace geosensor::ui
