#include "geosensor/tracking/TrackStore.h"

namespace geosensor::tracking
{

TrackHistory::TrackHistory(TrackId targetId) noexcept
    : targetId_(targetId)
{
}

TrackId TrackHistory::trackId() const noexcept
{
    return targetId_;
}

void TrackHistory::addPoint(const TrackPoint& point)
{
    points_.push_back(point);
}

const std::vector<TrackPoint>& TrackHistory::points() const noexcept
{
    return points_;
}

std::size_t TrackHistory::pointCount() const noexcept
{
    return points_.size();
}

void TrackHistory::trimToLastPoints(std::size_t maxPointCount)
{
    if (points_.size() <= maxPointCount) {
        return;
    }

    points_.erase(
        points_.begin(),
        points_.end() - static_cast<std::ptrdiff_t>(maxPointCount)
    );
}

void TrackStore::addPoint(const TrackPoint& point)
{
    getOrCreateTrack(point.targetId).addPoint(point);
}

TrackHistory& TrackStore::getOrCreateTrack(TrackId targetId)
{
    const auto [iterator, inserted] =
        tracks_.try_emplace(targetId, targetId);
    (void)inserted;
    return iterator->second;
}

std::size_t TrackStore::activeTrackCount() const noexcept
{
    return tracks_.size();
}

const std::vector<TrackPoint>* TrackStore::pointsForTrack(
    TrackId targetId
) const noexcept
{
    const auto iterator = tracks_.find(targetId);
    if (iterator == tracks_.end()) {
        return nullptr;
    }

    return &iterator->second.points();
}

std::vector<TrackId> TrackStore::trackIds() const
{
    std::vector<TrackId> trackIds;
    trackIds.reserve(tracks_.size());

    for (const auto& [trackId, trackHistory] : tracks_) {
        (void)trackHistory;
        trackIds.push_back(trackId);
    }

    return trackIds;
}

void TrackStore::trimTrackToLastPoints(
    TrackId targetId,
    std::size_t maxPointCount
)
{
    const auto iterator = tracks_.find(targetId);
    if (iterator == tracks_.end()) {
        return;
    }

    iterator->second.trimToLastPoints(maxPointCount);
}

void TrackStore::clear() noexcept
{
    tracks_.clear();
}

} // namespace geosensor::tracking
