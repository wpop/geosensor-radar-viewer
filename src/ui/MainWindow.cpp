#include "geosensor/ui/MainWindow.h"

#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/io/CsvMeasurementLoader.h"
#include "geosensor/networking/UdpMeasurementReceiver.h"
#include "geosensor/storage/MeasurementDatabase.h"
#include "geosensor/tracking/TrackStore.h"
#include "geosensor/ui/RadarView.h"

#include <QDateTime>
#include <QHeaderView>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <chrono>
#include <optional>
#include <vector>

namespace geosensor::ui
{

namespace
{

constexpr std::size_t kMaxLiveTargetCount = 100;
constexpr std::size_t kMaxTrailPointCount = 20;
constexpr int kTrackColumnTargetId = 0;
constexpr int kTrackColumnPoints = 1;
constexpr int kTrackColumnLastRange = 2;
constexpr int kTrackColumnLastAzimuth = 3;
constexpr int kTrackColumnLastIntensity = 4;
constexpr int kTrackColumnLastUpdateTime = 5;

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
    auto* leftPanel = new QWidget(centralWidget);
    auto* leftLayout = new QVBoxLayout(leftPanel);

    titleLabel_ = new QLabel(leftPanel);
    titleLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    titleLabel_->setContentsMargins(24, 24, 24, 24);
    titleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QFont font;
    font.setFamily("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(10);
    titleLabel_->setFont(font);

    startUdpButton_ = new QPushButton("Start UDP", leftPanel);
    stopUdpButton_ = new QPushButton("Stop UDP", leftPanel);
    clearLiveTargetsButton_ = new QPushButton("Clear Live Targets", leftPanel);
    clearStoredMeasurementsButton_ = new QPushButton(
        "Clear Stored Measurements",
        leftPanel
    );
    exportMeasurementsGeoJsonButton_ = new QPushButton(
        "Export Measurements GeoJSON",
        leftPanel
    );
    auto* exportMeasurementsCsvButton = new QPushButton(
        "Export Measurements CSV",
        leftPanel
    );
    trackStatisticsLabel_ = new QLabel("Track statistics", leftPanel);
    trackStatisticsTable_ = new QTableWidget(leftPanel);

    QFont sectionFont;
    sectionFont.setBold(true);
    trackStatisticsLabel_->setFont(sectionFont);

    trackStatisticsTable_->setColumnCount(6);
    trackStatisticsTable_->setHorizontalHeaderLabels(
        {
            "ID",
            "Points",
            "Range, m",
            "Azimuth, deg",
            "Intensity",
            "Updated"
        }
    );
    trackStatisticsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trackStatisticsTable_->setSelectionMode(QAbstractItemView::NoSelection);
    trackStatisticsTable_->setFocusPolicy(Qt::NoFocus);
    trackStatisticsTable_->verticalHeader()->setVisible(false);
    trackStatisticsTable_->horizontalHeader()->setStretchLastSection(false);
    trackStatisticsTable_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    trackStatisticsTable_->horizontalHeader()->setMinimumSectionSize(40);
    trackStatisticsTable_->setColumnWidth(kTrackColumnLastUpdateTime, 110);

    leftLayout->addWidget(titleLabel_, 1);
    leftLayout->addWidget(startUdpButton_);
    leftLayout->addWidget(stopUdpButton_);
    leftLayout->addWidget(clearLiveTargetsButton_);
    leftLayout->addWidget(clearStoredMeasurementsButton_);
    leftLayout->addWidget(exportMeasurementsGeoJsonButton_);
    leftLayout->addWidget(exportMeasurementsCsvButton);
    leftLayout->addWidget(trackStatisticsLabel_);
    leftLayout->addWidget(trackStatisticsTable_, 1);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    radarView_ = new RadarView(centralWidget);
    udpReceiver_ = new geosensor::networking::UdpMeasurementReceiver(this);

    try {
        const std::filesystem::path databasePath {
            "data/geosensor_live_measurements.sqlite"
        };
        std::filesystem::create_directories(databasePath.parent_path());

        if (measurementDatabase_.open(databasePath)) {
            databaseStatusText_ = "Enabled";
        } else {
            databaseStatusText_ = QString(
                "Open failed: %1"
            ).arg(QString::fromStdString(databasePath.string()));
        }
    } catch (const std::exception& error) {
        databaseStatusText_ = QString("Storage error: %1").arg(error.what());
    }

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
        &geosensor::networking::UdpMeasurementReceiver::packetReceived,
        this,
        [this](const geosensor::io::UdpMeasurementPacket& packet) {
            appendLiveMeasurement(packet.measurement, packet.targetId);
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

    QObject::connect(
        startUdpButton_,
        &QPushButton::clicked,
        this,
        [this]() { startUdpReceiver(); }
    );

    QObject::connect(
        stopUdpButton_,
        &QPushButton::clicked,
        this,
        [this]() { stopUdpReceiver(); }
    );

    QObject::connect(
        clearLiveTargetsButton_,
        &QPushButton::clicked,
        this,
        [this]() { clearLiveTargets(); }
    );

    QObject::connect(
        clearStoredMeasurementsButton_,
        &QPushButton::clicked,
        this,
        [this]() { clearStoredMeasurements(); }
    );

    QObject::connect(
        exportMeasurementsGeoJsonButton_,
        &QPushButton::clicked,
        this,
        [this]() { exportStoredMeasurementsToGeoJson(); }
    );

    QObject::connect(
        exportMeasurementsCsvButton,
        &QPushButton::clicked,
        this,
        [this]() {
            const QString filePath = QFileDialog::getSaveFileName(
                this,
                "Export Measurements CSV",
                "data/geosensor_measurements.csv",
                "CSV Files (*.csv);;All Files (*)"
            );

            if (filePath.isEmpty()) {
                return;
            }

            if (
                measurementDatabase_.exportMeasurementsToCsv(
                    std::filesystem::path(filePath.toStdString())
                )
            ) {
                databaseStatusText_ = QString("Exported CSV: %1").arg(filePath);
            } else {
                databaseStatusText_ = "Export CSV failed";
            }

            refreshDisplay();
        }
    );

    startUdpReceiver();

    refreshDisplay();

    layout->addWidget(leftPanel, 1);
    layout->addWidget(radarView_, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::refreshDisplay()
{
    radarView_->setSampleTargets(csvTargets_);
    radarView_->setLiveTargets(liveTargets_);
    radarView_->setTargetTrails(buildTargetTrails());
    updateTrackStatisticsTable();
    updateUdpControlStates();

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

    QString storedMeasurementText = "Stored measurements: unavailable\n";
    const auto storedMeasurementCount = measurementDatabase_.measurementCount();

    if (storedMeasurementCount.has_value()) {
        storedMeasurementText = QString(
            "Stored measurements: %1\n"
        ).arg(static_cast<qlonglong>(*storedMeasurementCount));
    }

    const QString text = QString(
        "GeoSensor Radar Viewer\n"
        "======================\n\n"
        "Loaded CSV file:\n"
        "  Path:         %1\n"
        "  Measurements: %2\n\n"
        "UDP status:\n"
        "  Receiver:     %3\n"
        "  Total valid UDP packets: %4\n"
        "  CSV targets:  %5\n"
        "  Live targets: %6 / %7\n"
        "  %8"
        "Database status:\n"
        "  Storage:      %9\n"
        "  %10"
        "Raw CSV measurements:\n"
        "%11"
        "%12"
        "%13"
    )
        .arg(csvPathText_.isEmpty() ? "Unavailable" : csvPathText_)
        .arg(static_cast<int>(csvMeasurements_.size()))
        .arg(udpStatusText_)
        .arg(static_cast<qulonglong>(totalValidUdpPackets_))
        .arg(static_cast<int>(csvTargets_.size()))
        .arg(static_cast<int>(liveTargets_.size()))
        .arg(static_cast<int>(kMaxLiveTargetCount))
        .arg(invalidPayloadText)
        .arg(databaseStatusText_)
        .arg(storedMeasurementText)
        .arg(measurementRows)
        .arg(firstTargetText)
        .arg(latestLiveText)
        .arg("Targets are displayed on the radar scene.");

    titleLabel_->setText(text);
}

void MainWindow::appendLiveMeasurement(
    const geosensor::data::SensorMeasurement& measurement,
    const std::optional<geosensor::tracking::TrackId>& targetId
)
{
    ++totalValidUdpPackets_;
    liveMeasurements_.push_back(measurement);
    const auto target = transform_.transform(measurement);
    liveTargets_.push_back(target.enu);

    if (targetId.has_value()) {
        trackStore_.addPoint(
            geosensor::tracking::TrackPoint {
                .targetId = *targetId,
                .measurement = measurement,
                .position = target.enu,
                .timestamp = std::chrono::system_clock::now(),
            }
        );
        trackStore_.trimTrackToLastPoints(*targetId, kMaxTrailPointCount);
    }

    const std::optional<std::int64_t> databaseTargetId =
        targetId.has_value()
            ? std::optional<std::int64_t>(*targetId)
            : std::nullopt;

    if (
        !measurementDatabase_.insertTrackMeasurement(
            measurement,
            databaseTargetId,
            std::chrono::system_clock::now()
        )
    ) {
        databaseStatusText_ = "Insert failed";
    } else if (databaseStatusText_ != "Enabled") {
        databaseStatusText_ = "Enabled";
    }

    if (liveMeasurements_.size() > kMaxLiveTargetCount) {
        liveMeasurements_.erase(liveMeasurements_.begin());
    }

    if (liveTargets_.size() > kMaxLiveTargetCount) {
        liveTargets_.erase(liveTargets_.begin());
    }

    refreshDisplay();
}

void MainWindow::startUdpReceiver()
{
    if (udpReceiver_->start()) {
        udpStatusText_ = "Listening on 127.0.0.1:5005";
    } else {
        udpStatusText_ = "Failed to listen on 127.0.0.1:5005";
    }

    refreshDisplay();
}

void MainWindow::stopUdpReceiver()
{
    udpReceiver_->stop();
    udpStatusText_ = "Stopped";
    refreshDisplay();
}

void MainWindow::clearLiveTargets()
{
    liveMeasurements_.clear();
    liveTargets_.clear();
    trackStore_.clear();
    refreshDisplay();
}

void MainWindow::clearStoredMeasurements()
{
    if (!measurementDatabase_.clearMeasurements()) {
        databaseStatusText_ = "Clear failed";
    } else if (databaseStatusText_ != "Enabled") {
        databaseStatusText_ = "Enabled";
    }

    refreshDisplay();
}

void MainWindow::exportStoredMeasurementsToGeoJson()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Measurements GeoJSON",
        "data/geosensor_measurements.geojson",
        "GeoJSON Files (*.geojson);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    if (
        measurementDatabase_.exportMeasurementsToGeoJson(
            std::filesystem::path(filePath.toStdString()),
            sensorOrigin_
        )
    ) {
        databaseStatusText_ = QString("Exported GeoJSON: %1").arg(filePath);
    } else {
        databaseStatusText_ = "Export GeoJSON failed";
    }

    refreshDisplay();
}

void MainWindow::updateUdpControlStates()
{
    const bool isListening = udpReceiver_->isListening();

    startUdpButton_->setEnabled(!isListening);
    stopUdpButton_->setEnabled(isListening);
    clearLiveTargetsButton_->setEnabled(!liveTargets_.empty());
    clearStoredMeasurementsButton_->setEnabled(
        measurementDatabase_.measurementCount().has_value()
    );
}

void MainWindow::updateTrackStatisticsTable()
{
    if (trackStatisticsTable_ == nullptr) {
        return;
    }

    const auto trackIds = trackStore_.trackIds();
    std::vector<geosensor::tracking::TrackId> sortedTrackIds = trackIds;
    std::sort(sortedTrackIds.begin(), sortedTrackIds.end());

    trackStatisticsTable_->setSortingEnabled(false);
    trackStatisticsTable_->setRowCount(0);

    for (const auto trackId : sortedTrackIds) {
        const auto* points = trackStore_.pointsForTrack(trackId);
        if (points == nullptr || points->empty()) {
            continue;
        }

        const auto& lastPoint = points->back();
        const int rowIndex = trackStatisticsTable_->rowCount();
        trackStatisticsTable_->insertRow(rowIndex);

        const auto timestampMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                lastPoint.timestamp.time_since_epoch()
            ).count();

        const QString timestampText = QDateTime::fromMSecsSinceEpoch(
            static_cast<qint64>(timestampMs)
        ).toUTC().toString("HH:mm:ss 'UTC'");

        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnTargetId,
            new QTableWidgetItem(QString::number(trackId))
        );
        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnPoints,
            new QTableWidgetItem(QString::number(points->size()))
        );
        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnLastRange,
            new QTableWidgetItem(
                QString::number(lastPoint.measurement.rangeM, 'f', 1)
            )
        );
        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnLastAzimuth,
            new QTableWidgetItem(
                QString::number(lastPoint.measurement.azimuthDeg, 'f', 1)
            )
        );
        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnLastIntensity,
            new QTableWidgetItem(
                QString::number(lastPoint.measurement.intensity, 'f', 2)
            )
        );
        trackStatisticsTable_->setItem(
            rowIndex,
            kTrackColumnLastUpdateTime,
            new QTableWidgetItem(timestampText)
        );
    }

    trackStatisticsTable_->setSortingEnabled(true);
}

std::vector<std::vector<geosensor::data::EnuPosition>> MainWindow::buildTargetTrails(
) const
{
    std::vector<std::vector<geosensor::data::EnuPosition>> trails;
    const auto trackIds = trackStore_.trackIds();
    trails.reserve(trackIds.size());

    for (const auto trackId : trackIds) {
        const auto* points = trackStore_.pointsForTrack(trackId);
        if (points == nullptr || points->size() < 2) {
            continue;
        }

        const std::size_t firstPointIndex =
            points->size() > kMaxTrailPointCount
                ? points->size() - kMaxTrailPointCount
                : 0;

        std::vector<geosensor::data::EnuPosition> trail;
        trail.reserve(points->size() - firstPointIndex);

        for (std::size_t i = firstPointIndex; i < points->size(); ++i) {
            trail.push_back((*points)[i].position);
        }

        trails.push_back(std::move(trail));
    }

    return trails;
}

} // namespace geosensor::ui
