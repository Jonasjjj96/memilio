/* 
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Anna Wendler, Lena Ploetzke
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
#ifndef IDESECIR_INFECTIONSTATE_H
#define IDESECIR_INFECTIONSTATE_H

namespace mio
{

namespace isecir
{

/**
 * @brief The InfectionState enum describes the possible
 * categories for the infectious state of persons
 */
enum class InfectionState
{
    Susceptible        = 0,
    Exposed            = 1,
    InfectedNoSymptoms = 2,
    InfectedSymptoms   = 3,
    InfectedSevere     = 4,
    InfectedCritical   = 5,
    Recovered          = 6,
    Dead               = 7,
    Count              = 8
};

/**
 * @brief The InfectionTransitions enum describes the possible
 * transitions of the infectious state of persons.
 * InfectionsTransitions ignores the transitions to Recovered because they are not needed for Simulation. 
 */
enum class InfectionTransitions
{ // unordered map!
    SusceptibleToExposed                 = 0,
    ExposedToInfectedNoSymptoms          = 1,
    InfectedNoSymptomsToInfectedSymptoms = 2,
    InfectedSymptomsToInfectedSevere     = 3,
    InfectedSevereToInfectedCritical     = 4,
    InfectedCriticalToDead               = 5,
    Count                                = 6
};

} // namespace isecir
} // namespace mio

#endif

/*
class InfectionTransitions2
{

    InfectionTransitions2()
    {

        std::vector<std::pair<InfectionState, InfectionState>> a;
        // map[;
        // map[InfectionState::Exposed]            = {InfectionState::InfectedNoSymptoms};
        // map[InfectionState::InfectedNoSymptoms] = {InfectionState::InfectedSymptoms, InfectionState::Recovered};
    }

    // size_t idx = 0;
    // for (size_t i = 0; i < InfectionState::Count; i++) {
    //     for (size_t j = 0; j < map[i].size(); j++) {
    //         3 * VECTOR[idx] idx++;
    //     }
    // }

private:
    std::unordered_map<std::pair<InfectionState, InfectionState>, size_t> map;
}

} // namespace isecir
} // namespace mio
*/
