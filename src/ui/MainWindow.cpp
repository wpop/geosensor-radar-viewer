#include "geosensor/ui/MainWindow.h"

#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/io/CsvMeasurementLoader.h"
#include "geosensor/networking/UdpMeasurementReceiver.h"
#include "geosensor/ui/RadarView.h"

#include <QHBoxLayout>
#include <QFont>
#include <QObject>
#include <QString>
#include <QWidget>

#include <exception>
#include <filesystem>
#include <vector>

namespace geosensor::ui
{

namespace
{

constexpr std::size_t kMaxLiveTargetCount = 100;

} // namespace

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
    udpReceiver_ = new geosensor::networking::UdpMeasurementReceiver(this);

    try {
        const std::filesystem::path csvPath {
            "data/samples/measurements.csv"
        };
        csvPathText_ = QString::fromStdString(csvPath.string());

        const geosensor::io::CsvMeasurementLoader loader;
        csvMeasurements_ = loader.load(csvPath);
        csvTargets_.reserve(csvMeasurements_.size());

        for (const auto& measurement : csvMeasurements_) {
            csvTargets_.push_back(transform_.transform(measurement).enu);
        }
    } catch (const std::exception& error) {
        udpStatusText_ = QString("CSV load error: %1").arg(error.what());
        csvPathText_.clear();
        csvMeasurements_.clear();
        csvTargets_.clear();
    }

    QObject::connect(
        udpReceiver_,
        &geosensor::networking::UdpMeasurementReceiver::measurementReceived,
        this,
        [this](const geosensor::data::SensorMeasurement& measurement) {
            appendLiveMeasurement(measurement);
        }
    );

    QObject::connect(
        udpReceiver_,
        &geosensor::networking::UdpMeasurementReceiver::invalidPayloadReceived,
        this,
        [this](const QString& payload) {
            lastInvalidPayload_ = payload;
            refreshDisplay();
        }
    );

    if (udpReceiver_->start()) {
        udpStatusText_ = "Listening on 127.0.0.1:5005";
    } else {
        udpStatusText_ = "Failed to listen on 127.0.0.1:5005";
    }

    refreshDisplay();

    layout->addWidget(titleLabel_, 1);
    layout->addWidget(radarView_, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::refreshDisplay()
{
    radarView_->setSampleTargets(csvTargets_);
    radarView_->setLiveTargets(liveTargets_);

    QString measurementRows;

    for (std::size_t i = 0; i < csvMeasurements_.size(); ++i) {
        const auto& measurement = csvMeasurements_[i];

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

    if (!csvMeasurements_.empty()) {
        const geosensor::data::TargetPosition firstTarget =
            transform_.transform(csvMeasurements_.front());

        firstTargetText = QString(
            "First transformed CSV target:\n"
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
        firstTargetText = "No CSV measurements loaded.\n";
    }

    QString latestLiveText = "No live UDP measurements received.\n";

    if (!liveMeasurements_.empty()) {
        const auto& measurement = liveMeasurements_.back();
        const auto target = transform_.transform(measurement);

        latestLiveText = QString(
            "Latest live UDP measurement:\n"
            "  range=%1 m, azimuth=%2 deg, elevation=%3 deg, intensity=%4\n"
            "  ENU East=%5 m, North=%6 m, Up=%7 m\n"
        )
            .arg(measurement.rangeM, 0, 'f', 1)
            .arg(measurement.azimuthDeg, 0, 'f', 1)
            .arg(measurement.elevationDeg, 0, 'f', 1)
            .arg(measurement.intensity, 0, 'f', 2)
            .arg(target.enu.eastM, 0, 'f', 2)
            .arg(target.enu.northM, 0, 'f', 2)
            .arg(target.enu.upM, 0, 'f', 2);
    }

    QString invalidPayloadText = "Last invalid UDP payload: none\n";

    if (!lastInvalidPayload_.isEmpty()) {
        invalidPayloadText = QString(
            "Last invalid UDP payload: %1\n"
        ).arg(lastInvalidPayload_);
    }

    const QString text = QString(
        "GeoSensor Radar Viewer\n"
        "======================\n\n"
        "Loaded CSV file:\n"
        "  Path:         %1\n"
        "  Measurements: %2\n\n"
        "UDP status:\n"
        "  Receiver:     %3\n"
        "  CSV targets:  %4\n"
        "  Live targets: %5 / %6\n"
        "  %7\n"
        "Raw CSV measurements:\n"
        "%8"
        "%9"
        "%10"
        "%11"
    )
        .arg(csvPathText_.isEmpty() ? "Unavailable" : csvPathText_)
        .arg(static_cast<int>(csvMeasurements_.size()))
        .arg(udpStatusText_)
        .arg(static_cast<int>(csvTargets_.size()))
        .arg(static_cast<int>(liveTargets_.size()))
        .arg(static_cast<int>(kMaxLiveTargetCount))
        .arg(invalidPayloadText)
        .arg(measurementRows)
        .arg(firstTargetText)
        .arg(latestLiveText)
        .arg("Targets are displayed on the radar scene.");

    titleLabel_->setText(text);
}

void MainWindow::appendLiveMeasurement(
    const geosensor::data::SensorMeasurement& measurement
)
{
    liveMeasurements_.push_back(measurement);
    liveTargets_.push_back(transform_.transform(measurement).enu);

    if (liveMeasurements_.size() > kMaxLiveTargetCount) {
        liveMeasurements_.erase(liveMeasurements_.begin());
    }

    if (liveTargets_.size() > kMaxLiveTargetCount) {
        liveTargets_.erase(liveTargets_.begin());
    }

    refreshDisplay();
}

} // namespace geosensor::ui
