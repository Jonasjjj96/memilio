/* 
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Daniel Abele, Elisabeth Kluth, Khoa Nguyen, Sascha Korf, Carlotta Gerstein
*
* Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef MIO_ABM_LOCATION_TYPE_H
#define MIO_ABM_LOCATION_TYPE_H

#include "memilio/io/auto_serialize.h"
#include "memilio/io/io.h"
#include <cstdint>
#include <limits>
#include <functional>

namespace mio
{
namespace abm
{

/**
 * @brief Type of a Location.
 */
enum class LocationType : std::uint32_t
{
    Home = 0,
    School,
    Work,
    SocialEvent, // TODO: differentiate different kinds
    BasicsShop, // groceries and other necessities
    Hospital,
    ICU,
    Car,
    PublicTransport,
    TransportWithoutContact, // all ways of travel with no contact to other people, e.g. biking or walking
    Cemetery, // Location for all the dead persons. It is created once for the World.

    Count //last!
};

static constexpr uint32_t INVALID_LOCATION_INDEX = std::numeric_limits<uint32_t>::max();

/**
 * LocationId identifies a Location uniquely. It consists of the LocationType of the Location and an Index.
 * The index corresponds to the index into the structure m_locations from world, where all Locations are saved.
 */
struct LocationId {
    uint32_t index;
    LocationType type; // TODO: move to location

    bool operator==(const LocationId& rhs) const
    {
        return (index == rhs.index && type == rhs.type);
    }

    bool operator!=(const LocationId& rhs) const
    {
        return !(index == rhs.index && type == rhs.type);
    }

    bool operator<(const LocationId& rhs) const
    {
        if (type == rhs.type) {
            return index < rhs.index;
        }
        return (type < rhs.type);
    }

    // same interface as location
    uint32_t get_index() const
    {
        return index;
    }

    // same interface as location
    LocationType get_type() const
    {
        return type;
    }

    /**
     * serialize this. 
     * @see mio::serialize
     */
    template <class IOContext>
    void serialize(IOContext& io) const
    {
        auto obj = io.create_object("LocationId");
        obj.add_element("index", index);
        obj.add_element("type", type);
    }

    /**
     * deserialize an object of this class.
     * @see mio::deserialize
     */
    template <class IOContext>
    static IOResult<LocationId> deserialize(IOContext& io)
    {
        auto obj = io.expect_object("LocationId");
        auto i   = obj.expect_element("index", mio::Tag<uint32_t>{});
        auto t   = obj.expect_element("type", mio::Tag<LocationType>{});
        return apply(
            io,
            [](auto&& index_, auto&& type_) {
                return LocationId{index_, type_};
            },
            i, t);
    }
};

struct GeographicalLocation {
    double latitude;
    double longitude;

    /**
     * @brief Compare two Location%s.
     */
    bool operator==(const GeographicalLocation& other) const
    {
        return (latitude == other.latitude && longitude == other.longitude);
    }

    bool operator!=(const GeographicalLocation& other) const
    {
        return !(latitude == other.latitude && longitude == other.longitude);
    }

    auto auto_serialize()
    {
        return make_auto_serialization("GraphicalLocation", NVP("latitude", latitude), NVP("longitude", longitude));
    };
};

} // namespace abm
} // namespace mio

template <>
struct std::hash<mio::abm::LocationId> {
    std::size_t operator()(const mio::abm::LocationId& loc_id) const
    {
        return (std::hash<uint32_t>()(loc_id.index)) ^ (std::hash<uint32_t>()(static_cast<uint32_t>(loc_id.type)));
    }
};

#endif
