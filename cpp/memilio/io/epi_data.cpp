/* 
* Copyright (C) 2020-2022 German Aerospace Center (DLR-SC)
*
* Authors: Daniel Abele
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

#include "memilio/io/epi_data.h"

#ifdef MEMILIO_HAS_JSONCPP

namespace mio
{

const std::array<const char*, 6> ConfirmedCasesDataEntry::age_group_names = {"A00-A04", "A05-A14", "A15-A34",
                                                                             "A35-A59", "A60-A79", "A80+"};

const std::array<const char*, 11> PopulationDataEntry::age_group_names = {
    "<3 years",    "3-5 years",   "6-14 years",  "15-17 years", "18-24 years", "25-29 years",
    "30-39 years", "40-49 years", "50-64 years", "65-74 years", ">74 years"};

const std::array<const char*, 6> VaccinationDataEntry::age_group_names = {"0-4",   "5-14",  "15-34",
                                                                          "35-59", "60-79", "80-99"};

IOResult<std::vector<int>> get_county_ids(const std::string& path)
{
    BOOST_OUTCOME_TRY(population_data, read_population_data(path_join(path, "county_current_population.json")));

    std::vector<int> id;
    id.reserve(population_data.size());
    for (auto&& entry : population_data) {
        if (entry.county_id) {
            id.push_back(entry.county_id->get());
        }
        else {
            return failure(StatusCode::InvalidValue, "Population data file is missing county ids.");
        }
    }
    //remove duplicate county ids
    id.erase(std::unique(id.begin(), id.end()), id.end());
    return success(id);
}

} // namespace mio

#endif //MEMILIO_HAS_JSONCPP