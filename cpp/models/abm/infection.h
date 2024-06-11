/*
* Copyright (C) 2020-2024 MEmilio
*
* Authors: David Kerkmann, Sascha Korf, Khoa Nguyen
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
#ifndef MIO_ABM_INFECTION_H
#define MIO_ABM_INFECTION_H

#include "abm/personal_rng.h"
#include "abm/rs.h"
#include "abm/time.h"
#include "abm/infection_state.h"
#include "abm/virus_variant.h"
#include "abm/parameters.h"

#include <vector>

namespace mio
{
namespace abm
{

/**
 * @brief Models the ViralLoad for an Infection, modelled on a log_10 scale.
 * Based on https://www.science.org/doi/full/10.1126/science.abi5273
 * Examplary ViralLoad courses can be seen for example in Fig. 4 B.
 */
struct ViralLoad {
    TimePoint start_date; ///< Start date of the ViralLoad concentration in the Person.
    TimePoint end_date; ///< End date of the ViralLoad concentration in the Person.
    ScalarType peak; ///< Peak amplitude of the ViralLoad.
    ScalarType incline; ///< Incline of the ViralLoad during incline phase in log_10 scale per day (always positive).
    ScalarType decline; ///< Decline of the ViralLoad during decline phase in log_10 scale per day (always negative).

    auto auto_serialize()
    {
        return make_auto_serialization("ViralLoad", NVP("start_date", start_date), NVP("end_date", end_date),
                                       NVP("peak", peak), NVP("incline", incline), NVP("decline", decline));
    }
};

class Infection
{
private:
    friend AutoConstructor<Infection>;
    Infection()
    {
    }

public:
    /**
     * @brief Create an Infection for a single Person.
     * Draws a random infection course.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person.
     * @param[in] virus Virus type of the Infection.
     * @param[in] age AgeGroup to determine the ViralLoad course.
     * @param[in] params Parameters of the Model.
     * @param[in] init_date Date of initializing the Infection.
     * @param[in] init_state [Default: InfectionState::Exposed] #InfectionState at time of initializing the Infection.
     * @param[in] latest_exposure [Default: {ExposureType::NoProtection, TimePoint(0)}] The pair value of last ExposureType (previous Infection/Vaccination) and TimePoint of that protection.
     * @param[in] detected [Default: false] If the Infection is detected.     
     */
    Infection(PersonalRandomNumberGenerator& rng, VirusVariant virus, AgeGroup age, const Parameters& params,
              TimePoint start_date, InfectionState start_state = InfectionState::Exposed,
              std::pair<ExposureType, TimePoint> latest_exposure = {ExposureType::NoProtection, TimePoint(0)},
              bool detected                                      = false);

    /**
     * @brief Gets the ViralLoad of the Infection at a given TimePoint.
     * @param[in] t TimePoint of querry.
     */
    ScalarType get_viral_load(TimePoint t) const;

    /**
     * @brief Get infectivity at a given time.
     * Computed depending on current ViralLoad and individual invlogit function of each Person
     * corresponding to https://www.science.org/doi/full/10.1126/science.abi5273
     * The mapping corresponds to Fig. 2 C.
     * Formula of invlogit function can be found here:
     * https://github.com/VirologyCharite/SARS-CoV-2-VL-paper/tree/main
     * in ExtendedMethods.html, Section 3.1.2.1.
     * @param[in] t TimePoint of the querry.
     * @return Infectivity at given TimePoint.
     */
    ScalarType get_infectivity(TimePoint t) const;

    /**
     * @brief: Get VirusVariant.
     * @return VirusVariant of the Infection.
     */
    VirusVariant get_virus_variant() const;

    /**
     * @brief Get the #InfectionState of the Infection.
     * @param[in] t TimePoint of the querry.
     * @return #InfectionState at the given TimePoint.
     */
    InfectionState get_infection_state(TimePoint t) const;

    /**
     * @brief Set the Infection to detected.
     */
    void set_detected();

    /**
     * @return Get the detected state.
     */
    bool is_detected() const;

    /**
     * @returns Get the start date of the infection.
    */
    TimePoint get_start_date() const;

    auto auto_serialize()
    {
        return make_auto_serialization("Infection", NVP("infection_course", m_infection_course),
                                       NVP("virus_variant", m_virus_variant), NVP("viral_load", m_viral_load),
                                       NVP("log_norm_alpha", m_log_norm_alpha), NVP("log_norm_beta", m_log_norm_beta),
                                       NVP("detected", m_detected));
    }

private:
    /**
     * @brief Determine ViralLoad course and Infection course based on init_state.
     * Calls draw_infection_course_backward for all #InfectionState%s prior and draw_infection_course_forward for all
     * subsequent #InfectionState%s.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person.
     * @param[in] age AgeGroup of the Person.
     * @param[in] params Parameters of the Model.
     * @param[in] init_date Date of initializing the Infection.
     * @param[in] init_state #InfectionState at time of initializing the Infection.
     * @return The starting date of the Infection.
     */
    TimePoint draw_infection_course(PersonalRandomNumberGenerator& rng, AgeGroup age, const Parameters& params,
                                    TimePoint init_date, InfectionState start_state,
                                    std::pair<ExposureType, TimePoint> latest_protection);

    /**
     * @brief Determine ViralLoad course and Infection course prior to the given start_state.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person.
     * @param[in] age AgeGroup of the Person.
     * @param[in] params Parameters of the Model.
     * @param[in] init_date Date of initializing the Infection.
     * @param[in] init_state #InfectionState at time of initializing the Infection.
     */
    void draw_infection_course_forward(PersonalRandomNumberGenerator& rng, AgeGroup age, const Parameters& params,
                                       TimePoint init_date, InfectionState start_state,
                                       std::pair<ExposureType, TimePoint> latest_protection);

    /**
     * @brief Determine ViralLoad course and Infection course subsequent to the given start_state.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person.
     * @param[in] age AgeGroup of the person.
     * @param[in] params Parameters of the Model.
     * @param[in] init_date Date of initializing the Infection.
     * @param[in] init_state InfectionState at time of initializing the Infection.
     * @return The starting date of the Infection.
     */
    TimePoint draw_infection_course_backward(PersonalRandomNumberGenerator& rng, AgeGroup age, const Parameters& params,
                                             TimePoint init_date, InfectionState init_state);

    std::vector<std::pair<TimePoint, InfectionState>> m_infection_course; ///< Start date of each #InfectionState.
    VirusVariant m_virus_variant; ///< Variant of the Infection.
    ViralLoad m_viral_load; ///< ViralLoad of the Infection.
    ScalarType m_log_norm_alpha,
        m_log_norm_beta; ///< Parameters for the infectivity mapping, which is modelled through an invlogit function.
    bool m_detected; ///< Whether an Infection is detected or not.
};

} // namespace abm
} // namespace mio

#endif
