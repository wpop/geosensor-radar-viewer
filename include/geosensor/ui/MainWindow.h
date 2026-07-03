#pragma once

#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/io/UdpMeasurementParser.h"
#include "geosensor/data/SensorMeasurement.h"
#include "geosensor/storage/MeasurementDatabase.h"
#include "geosensor/tracking/TrackStore.h"

#include <QLabel>
#include <QMainWindow>

#include <optional>
#include <vector>

class QPushButton;
class QTableWidget;

namespace geosensor::networking
{

class UdpMeasurementReceiver;

} // namespace geosensor::networking

namespace geosensor::ui
{

class RadarView;

// Main application window for the radar viewer demo.
class MainWindow : public QMainWindow
{
public:
    // Creates the main window and initializes the demo content.
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();
    void refreshDisplay();
    void appendLiveMeasurement(
        const geosensor::data::SensorMeasurement& measurement,
        const std::optional<geosensor::tracking::TrackId>& targetId = std::nullopt
    );
    void startUdpReceiver();
    void stopUdpReceiver();
    void clearLiveTargets();
    void clearStoredMeasurements();
    void exportStoredMeasurementsToGeoJson();
    void exportTrackedMeasurementsToGeoJson();
    void updateUdpControlStates();
    void updateTrackStatisticsTable();
    [[nodiscard]] std::vector<std::vector<geosensor::data::EnuPosition>>
    buildTargetTrails() const;

    QLabel* titleLabel_ {};
    QLabel* trackStatisticsLabel_ {};
    QTableWidget* trackStatisticsTable_ {};
    RadarView* radarView_ {};
    QPushButton* startUdpButton_ {};
    QPushButton* stopUdpButton_ {};
    QPushButton* clearLiveTargetsButton_ {};
    QPushButton* clearStoredMeasurementsButton_ {};
    QPushButton* exportMeasurementsGeoJsonButton_ {};
    QPushButton* exportTracksGeoJsonButton_ {};
    geosensor::networking::UdpMeasurementReceiver* udpReceiver_ {};
    std::vector<geosensor::data::SensorMeasurement> csvMeasurements_ {};
    std::vector<geosensor::data::SensorMeasurement> liveMeasurements_ {};
    std::vector<geosensor::data::EnuPosition> csvTargets_ {};
    std::vector<geosensor::data::EnuPosition> liveTargets_ {};
    geosensor::tracking::TrackStore trackStore_ {};
    geosensor::data::SensorOrigin sensorOrigin_ {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };
    geosensor::coordinates::CoordinateTransform transform_ {sensorOrigin_};
    geosensor::storage::MeasurementDatabase measurementDatabase_ {};
    QString csvPathText_ {};
    QString udpStatusText_ {"Not started."};
    QString databaseStatusText_ {"Disabled"};
    QString lastInvalidPayload_ {};
    std::size_t totalValidUdpPackets_ {};
};

} // namespace geosensor::ui
