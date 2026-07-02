#pragma once

#include "geosensor/data/SensorMeasurement.h"

#include <QMetaType>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE

Q_DECLARE_METATYPE(geosensor::data::SensorMeasurement)

namespace geosensor::networking
{

// Receives UDP radar measurement payloads and emits parsed measurements.
class UdpMeasurementReceiver : public QObject
{
    Q_OBJECT

public:
    // Creates a receiver bound to the given QObject parent.
    explicit UdpMeasurementReceiver(QObject* parent = nullptr);

    // Starts listening for UDP measurements on the default localhost port.
    [[nodiscard]] bool start();

    // Stops listening for incoming UDP measurements.
    void stop();

    // Returns true when the receiver is currently bound and listening.
    [[nodiscard]] bool isListening() const;

signals:
    // Emitted when a UDP payload is parsed into a valid measurement.
    void measurementReceived(geosensor::data::SensorMeasurement measurement);

    // Emitted when a UDP payload cannot be parsed as a measurement.
    void invalidPayloadReceived(const QString& payload);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket* socket_ {};
};

} // namespace geosensor::networking
