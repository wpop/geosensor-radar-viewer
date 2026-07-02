#pragma once

#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/data/SensorMeasurement.h"

#include <QLabel>
#include <QMainWindow>

#include <vector>

class QPushButton;

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
    void appendLiveMeasurement(const geosensor::data::SensorMeasurement& measurement);
    void startUdpReceiver();
    void stopUdpReceiver();
    void clearLiveTargets();
    void updateUdpControlStates();

    QLabel* titleLabel_ {};
    RadarView* radarView_ {};
    QPushButton* startUdpButton_ {};
    QPushButton* stopUdpButton_ {};
    QPushButton* clearLiveTargetsButton_ {};
    geosensor::networking::UdpMeasurementReceiver* udpReceiver_ {};
    std::vector<geosensor::data::SensorMeasurement> csvMeasurements_ {};
    std::vector<geosensor::data::SensorMeasurement> liveMeasurements_ {};
    std::vector<geosensor::data::EnuPosition> csvTargets_ {};
    std::vector<geosensor::data::EnuPosition> liveTargets_ {};
    geosensor::data::SensorOrigin sensorOrigin_ {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };
    geosensor::coordinates::CoordinateTransform transform_ {sensorOrigin_};
    QString csvPathText_ {};
    QString udpStatusText_ {"Not started."};
    QString lastInvalidPayload_ {};
    std::size_t totalValidUdpPackets_ {};
};

} // namespace geosensor::ui
