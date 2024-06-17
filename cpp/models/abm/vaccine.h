/*
* Copyright (C) 2020-2024 MEmilio
*
* Authors: David Kerkmann, Khoa Nguyen
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
#ifndef MIO_ABM_VACCINE_H
#define MIO_ABM_VACCINE_H

#include "memilio/io/auto_serialize.h"
#include "abm/time.h"

#include <cstdint>

namespace mio
{
namespace abm
{

/** 
 * @brief #ExposureType in ABM.
 * can be used as 0-based index
 */
enum class ExposureType : std::uint32_t
{
    NoProtection     = 0,
    NaturalInfection = 1,
    GenericVaccine   = 2,
    Count //last!!
};

/**
 * @brief A tuple of #TimePoint and #ExposureType (i.e. type of the Vaccine).
 * The #TimePoint describes the time of administration of the Vaccine.
*/
struct Vaccination {
    Vaccination(ExposureType exposure, TimePoint t)
        : exposure_type(exposure)
        , time(t)
    {
    }

    auto auto_serialize()
    {
        return make_auto_serialization("Vaccination", NVP("exposure_type", exposure_type), NVP("time", time));
    }

    ExposureType exposure_type;
    TimePoint time;
};

} // namespace abm

template <>
struct AutoConstructor<abm::Vaccination> : abm::Vaccination {
    AutoConstructor()
        : Vaccination(abm::ExposureType::Count, abm::TimePoint())
    {
    }
};

} // namespace mio

#endif
