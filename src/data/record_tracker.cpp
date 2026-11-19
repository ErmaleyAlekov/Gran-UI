#include "gran/data/record_tracker.hpp"

namespace gran::data
{
    RecordTracker::RecordTracker(ListSource& source, std::size_t index)
        : _source(source), _index(index), _alive(index < source.size())
    {
        _source.subscribe(this);
    }

    RecordTracker::~RecordTracker()
    {
        _source.unsubscribe(this);
    }

    ListSource& RecordTracker::source() const noexcept
    {
        return _source;
    }

    std::size_t RecordTracker::index() const noexcept
    {
        return _index;
    }

    bool RecordTracker::alive() const noexcept
    {
        return _alive;
    }
}
