#include "geosensor/networking/UdpMeasurementReceiver.h"

#include "geosensor/io/UdpMeasurementParser.h"

#include <QDebug>
#include <QHostAddress>
#include <QUdpSocket>

namespace
{

constexpr quint16 kDefaultUdpPort = 5005;

} // namespace

namespace geosensor::networking
{

UdpMeasurementReceiver::UdpMeasurementReceiver(QObject* parent)
    : QObject(parent)
    , socket_(new QUdpSocket(this))
{
    qRegisterMetaType<geosensor::data::SensorMeasurement>(
        "geosensor::data::SensorMeasurement"
    );
    qRegisterMetaType<geosensor::io::UdpMeasurementPacket>(
        "geosensor::io::UdpMeasurementPacket"
    );

    connect(
        socket_,
        &QUdpSocket::readyRead,
        this,
        &UdpMeasurementReceiver::readPendingDatagrams
    );
}

bool UdpMeasurementReceiver::start()
{
    if (isListening()) {
        return true;
    }

    return socket_->bind(
        QHostAddress::LocalHost,
        kDefaultUdpPort,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
    );
}

void UdpMeasurementReceiver::stop()
{
    socket_->close();
}

bool UdpMeasurementReceiver::isListening() const
{
    return socket_->state() == QAbstractSocket::BoundState;
}

void UdpMeasurementReceiver::readPendingDatagrams()
{
    while (socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(socket_->pendingDatagramSize()));

        const qint64 bytesRead =
            socket_->readDatagram(datagram.data(), datagram.size());

        if (bytesRead < 0) {
            qWarning() << "Failed to read UDP datagram:" << socket_->errorString();
            continue;
        }

        datagram.truncate(static_cast<int>(bytesRead));

        const auto packet = geosensor::io::UdpMeasurementParser::parsePacket(
            std::string_view(
                datagram.constData(),
                static_cast<std::size_t>(datagram.size())
            )
        );

        if (packet.has_value()) {
            emit packetReceived(*packet);
            emit measurementReceived(packet->measurement);
            continue;
        }

        const QString payload = QString::fromUtf8(datagram);
        emit invalidPayloadReceived(payload);
        qWarning() << "Failed to parse UDP measurement payload:" << payload;
    }
}

} // namespace geosensor::networking
