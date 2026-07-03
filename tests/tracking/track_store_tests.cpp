#include "geosensor/tracking/TrackStore.h"

#include <cassert>
#include <chrono>
#include <iostream>

namespace
{

geosensor::tracking::TrackPoint makePoint(
    geosensor::tracking::TrackId targetId,
    double rangeM,
    double eastM
)
{
    return geosensor::tracking::TrackPoint {
        .targetId = targetId,
        .measurement =
            geosensor::data::SensorMeasurement {
                .rangeM = rangeM,
                .azimuthDeg = 45.0,
                .elevationDeg = 2.0,
                .intensity = 0.8,
            },
        .position =
            geosensor::data::EnuPosition {
                .eastM = eastM,
                .northM = 100.0,
                .upM = 10.0,
            },
        .timestamp = std::chrono::system_clock::time_point(
            std::chrono::seconds(targetId)
        ),
    };
}

void testGetOrCreateTrack()
{
    geosensor::tracking::TrackStore store;

    auto& track = store.getOrCreateTrack(42);
    assert(track.trackId() == 42);
    assert(track.pointCount() == 0);
    assert(store.activeTrackCount() == 1);

    auto& sameTrack = store.getOrCreateTrack(42);
    assert(&track == &sameTrack);
    assert(store.activeTrackCount() == 1);
}

void testAddPointGroupsByTargetId()
{
    geosensor::tracking::TrackStore store;

    const auto firstPoint = makePoint(1, 700.0, 50.0);
    const auto secondPoint = makePoint(1, 710.0, 55.0);
    const auto thirdPoint = makePoint(2, 900.0, 80.0);

    store.addPoint(firstPoint);
    store.addPoint(secondPoint);
    store.addPoint(thirdPoint);

    assert(store.activeTrackCount() == 2);

    const auto* firstTrackPoints = store.pointsForTrack(1);
    assert(firstTrackPoints != nullptr);
    assert(firstTrackPoints->size() == 2);
    assert((*firstTrackPoints)[0].targetId == 1);
    assert((*firstTrackPoints)[0].measurement.rangeM == 700.0);
    assert((*firstTrackPoints)[1].position.eastM == 55.0);

    const auto* secondTrackPoints = store.pointsForTrack(2);
    assert(secondTrackPoints != nullptr);
    assert(secondTrackPoints->size() == 1);
    assert((*secondTrackPoints)[0].measurement.rangeM == 900.0);

    const auto trackIds = store.trackIds();
    assert(trackIds.size() == 2);
}

void testPointsForMissingTrackReturnsNull()
{
    geosensor::tracking::TrackStore store;
    assert(store.pointsForTrack(99) == nullptr);
}

void testClearRemovesAllTracks()
{
    geosensor::tracking::TrackStore store;
    store.addPoint(makePoint(1, 700.0, 50.0));
    store.addPoint(makePoint(2, 900.0, 80.0));

    assert(store.activeTrackCount() == 2);
    store.clear();
    assert(store.activeTrackCount() == 0);
    assert(store.pointsForTrack(1) == nullptr);
}

void testTrackHistoryTrimKeepsMostRecentPoints()
{
    geosensor::tracking::TrackHistory history(7);
    history.addPoint(makePoint(7, 700.0, 10.0));
    history.addPoint(makePoint(7, 710.0, 20.0));
    history.addPoint(makePoint(7, 720.0, 30.0));

    history.trimToLastPoints(2);

    assert(history.pointCount() == 2);
    assert(history.points()[0].measurement.rangeM == 710.0);
    assert(history.points()[1].measurement.rangeM == 720.0);
}

void testTrackStoreTrimKeepsMostRecentPointsPerTarget()
{
    geosensor::tracking::TrackStore store;
    store.addPoint(makePoint(1, 700.0, 10.0));
    store.addPoint(makePoint(1, 710.0, 20.0));
    store.addPoint(makePoint(1, 720.0, 30.0));
    store.addPoint(makePoint(2, 900.0, 40.0));

    store.trimTrackToLastPoints(1, 2);

    const auto* firstTrackPoints = store.pointsForTrack(1);
    assert(firstTrackPoints != nullptr);
    assert(firstTrackPoints->size() == 2);
    assert((*firstTrackPoints)[0].measurement.rangeM == 710.0);
    assert((*firstTrackPoints)[1].measurement.rangeM == 720.0);

    const auto* secondTrackPoints = store.pointsForTrack(2);
    assert(secondTrackPoints != nullptr);
    assert(secondTrackPoints->size() == 1);
    assert((*secondTrackPoints)[0].measurement.rangeM == 900.0);
}

} // namespace

int main()
{
    testGetOrCreateTrack();
    testAddPointGroupsByTargetId();
    testPointsForMissingTrackReturnsNull();
    testClearRemovesAllTracks();
    testTrackHistoryTrimKeepsMostRecentPoints();
    testTrackStoreTrimKeepsMostRecentPointsPerTarget();

    std::cout << "All track store tests passed.\n";

    return 0;
}
