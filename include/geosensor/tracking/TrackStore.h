#pragma once

#include "geosensor/data/SensorMeasurement.h"
#include "geosensor/data/TargetPosition.h"

#include <chrono>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace geosensor::tracking
{

// Identifier for one tracked target.
using TrackId = int;

// Timestamp associated with a tracked point.
using TrackTimestamp = std::chrono::system_clock::time_point;

// One tracked radar point for a specific target.
struct TrackPoint
{
    TrackId targetId {};
    data::SensorMeasurement measurement {};
    data::EnuPosition position {};
    TrackTimestamp timestamp {};
};

// History of tracked points for one target.
class TrackHistory
{
public:
    // Creates an empty history for one target identifier.
    explicit TrackHistory(TrackId targetId) noexcept;

    // Returns the target identifier for this history.
    [[nodiscard]] TrackId trackId() const noexcept;

    // Appends one point to this track history.
    void addPoint(const TrackPoint& point);

    // Returns the stored points for this track.
    [[nodiscard]] const std::vector<TrackPoint>& points() const noexcept;

    // Returns the number of stored points.
    [[nodiscard]] std::size_t pointCount() const noexcept;

    // Trims this history to the most recent point count.
    void trimToLastPoints(std::size_t maxPointCount);

private:
    TrackId targetId_ {};
    std::vector<TrackPoint> points_ {};
};

// Stores tracked points grouped by target identifier.
class TrackStore
{
public:
    // Adds one point to the matching track history.
    void addPoint(const TrackPoint& point);

    // Returns the existing track or creates an empty one.
    [[nodiscard]] TrackHistory& getOrCreateTrack(TrackId targetId);

    // Returns the number of active tracks.
    [[nodiscard]] std::size_t activeTrackCount() const noexcept;

    // Returns the stored points for a track when present.
    [[nodiscard]] const std::vector<TrackPoint>* pointsForTrack(
        TrackId targetId
    ) const noexcept;

    // Returns the currently known track identifiers.
    [[nodiscard]] std::vector<TrackId> trackIds() const;

    // Trims one track history to the most recent point count.
    void trimTrackToLastPoints(TrackId targetId, std::size_t maxPointCount);

    // Removes all stored tracks and points.
    void clear() noexcept;

private:
    std::unordered_map<TrackId, TrackHistory> tracks_ {};
};

} // namespace geosensor::tracking
