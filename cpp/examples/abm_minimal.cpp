/*
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Khoa Nguyen
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
#include "abm/abm.h"
#include "abm/household.h"
#include <cstdio>
#include "abm/world.h"
#include "memilio/io/io.h"
#include "abm/location_type.h"
#include <fstream>
#include <string>
#include <iostream>

std::string convert_loc_id_to_string(std::tuple<mio::abm::LocationType, uint32_t> tuple_id)
{
    return std::to_string(static_cast<std::uint32_t>(std::get<0>(tuple_id))) + "_" +
           std::to_string(std::get<1>(tuple_id));
}

std::vector<uint32_t> get_agents_per_location(
    std::tuple<mio::abm::LocationType, uint32_t> loc_id,
    std::vector<std::tuple<mio::abm::LocationId, uint32_t, mio::abm::TimeSpan, mio::abm::InfectionState>>& log)
{
    std::vector<uint32_t> agents_per_location;
    for (auto& log_tuple : log) {
        if (std::get<0>(log_tuple).type == std::get<0>(loc_id) && std::get<0>(log_tuple).index == std::get<1>(loc_id)) {
            agents_per_location.push_back(std::get<1>(log_tuple));
        }
    }
    return agents_per_location;
}

int main()
{
    // Set global infection parameters (similar to infection parameters in SECIR model) and initialize the world
    mio::abm::GlobalInfectionParameters infection_params;

    // Set same infection parameter for all age groups. For example, the incubation period is 4 days.
    infection_params.get<mio::abm::IncubationPeriod>() = 4.;

    // Create the world with infection parameters.
    auto world = mio::abm::World(infection_params);

    // There are 3 households for each household group.
    int n_households = 3;

    // For more than 1 family households we need families. These are parents and children and randoms (which are distributed like the data we have for these households).
    auto child = mio::abm::HouseholdMember(); // A child is 50/50% 0-4 or 5-14.
    child.set_age_weight(mio::abm::AgeGroup::Age0to4, 1);
    child.set_age_weight(mio::abm::AgeGroup::Age5to14, 1);

    auto parent = mio::abm::HouseholdMember(); // A parent is 50/50% 15-34 or 35-59.
    parent.set_age_weight(mio::abm::AgeGroup::Age15to34, 1);
    parent.set_age_weight(mio::abm::AgeGroup::Age35to59, 1);

    // Two-person household with one parent and one child.
    auto twoPersonHousehold_group = mio::abm::HouseholdGroup();
    auto twoPersonHousehold_full  = mio::abm::Household();
    twoPersonHousehold_full.add_members(child, 1);
    twoPersonHousehold_full.add_members(parent, 1);
    twoPersonHousehold_group.add_households(twoPersonHousehold_full, n_households);
    add_household_group_to_world(world, twoPersonHousehold_group);

    // Three-person household with two parent and one child.
    auto threePersonHousehold_group = mio::abm::HouseholdGroup();
    auto threePersonHousehold_full  = mio::abm::Household();
    threePersonHousehold_full.add_members(child, 1);
    threePersonHousehold_full.add_members(parent, 2);
    threePersonHousehold_group.add_households(threePersonHousehold_full, n_households);
    add_household_group_to_world(world, threePersonHousehold_group);

    // Assign an infection state to each person.
    // The infection states are chosen randomly.
    auto persons = world.get_persons();
    for (auto& person : persons) {
        uint32_t state = rand() % (uint32_t)mio::abm::InfectionState::Count;
        world.set_infection_state(person, (mio::abm::InfectionState)state);
    }

    // Add one social event with 5 maximum contacts.
    // Maximum contacs limit the number of people that a person can infect while being at this location.
    auto event = world.add_location(mio::abm::LocationType::SocialEvent);
    world.get_individualized_location(event).get_infection_parameters().set<mio::abm::MaximumContacts>(5);
    // Add hospital and ICU with 5 maximum contacs.
    auto hospital = world.add_location(mio::abm::LocationType::Hospital);
    world.get_individualized_location(hospital).get_infection_parameters().set<mio::abm::MaximumContacts>(5);
    auto icu = world.add_location(mio::abm::LocationType::ICU);
    world.get_individualized_location(icu).get_infection_parameters().set<mio::abm::MaximumContacts>(5);
    // Add one supermarket, maximum constacts are assumed to be 20.
    auto shop = world.add_location(mio::abm::LocationType::BasicsShop);
    world.get_individualized_location(shop).get_infection_parameters().set<mio::abm::MaximumContacts>(20);
    // At every school, the maximum contacts are 20.
    auto school = world.add_location(mio::abm::LocationType::School);
    world.get_individualized_location(school).get_infection_parameters().set<mio::abm::MaximumContacts>(20);
    // At every workplace, maximum contacts are 10.
    auto work = world.add_location(mio::abm::LocationType::Work);
    world.get_individualized_location(work).get_infection_parameters().set<mio::abm::MaximumContacts>(10);

    // People can get tested at work (and do this with 0.5 probability) from time point 0 to day 30.
    auto testing_min_time = mio::abm::days(1);
    auto probability      = 0.5;
    auto start_date       = mio::abm::TimePoint(0);
    auto end_date         = mio::abm::TimePoint(0) + mio::abm::days(30);
    auto test_type        = mio::abm::AntigenTest();
    auto test_at_work     = std::vector<mio::abm::LocationType>{mio::abm::LocationType::Work};
    auto testing_criteria_work =
        std::vector<mio::abm::TestingCriteria>{mio::abm::TestingCriteria({}, test_at_work, {})};
    auto testing_scheme_work =
        mio::abm::TestingScheme(testing_criteria_work, testing_min_time, start_date, end_date, test_type, probability);
    world.get_testing_strategy().add_testing_scheme(testing_scheme_work);

    // Assign locations to the people
    for (auto& person : persons) {
        //assign shop and event
        person.set_assigned_location(event);
        person.set_assigned_location(shop);
        //assign hospital and ICU
        person.set_assigned_location(hospital);
        person.set_assigned_location(icu);
        //assign work/school to people depending on their age
        if (person.get_age() == mio::abm::AgeGroup::Age5to14) {
            person.set_assigned_location(school);
        }
        if (person.get_age() == mio::abm::AgeGroup::Age15to34 || person.get_age() == mio::abm::AgeGroup::Age35to59) {
            person.set_assigned_location(work);
        }
    }

    // During the lockdown, social events are closed for 90% of people.
    auto t_lockdown = mio::abm::TimePoint(0) + mio::abm::days(10);
    mio::abm::close_social_events(t_lockdown, 0.9, world.get_migration_parameters());

    auto t0   = mio::abm::TimePoint(0);
    auto tmax = mio::abm::TimePoint(0) + mio::abm::days(30);
    auto sim  = mio::abm::Simulation(t0, std::move(world));

    struct LogTimePoint : LogAlways {
        using Type = double;
        static Type log(const mio::abm::Simulation& sim)
        {
            return sim.get_time().hours();
        }
    };
    struct LogLocationIds : LogOnce {
        using Type = std::vector<std::tuple<mio::abm::LocationType, uint32_t>>;
        static Type log(const mio::abm::Simulation& sim)
        {
            std::vector<std::tuple<mio::abm::LocationType, uint32_t>> location_ids{};
            for (auto&& locations : sim.get_world().get_locations()) {
                for (auto location : locations) {
                    location_ids.push_back(std::make_tuple(location.get_type(), location.get_index()));
                }
            }
            return location_ids;
        }
    };

    struct LogPersonsPerLocationAndInfectionTime : LogAlways {
        using Type =
            std::vector<std::tuple<mio::abm::LocationId, uint32_t, mio::abm::TimeSpan, mio::abm::InfectionState>>;
        static Type log(const mio::abm::Simulation& sim)
        {
            std::vector<std::tuple<mio::abm::LocationId, uint32_t, mio::abm::TimeSpan, mio::abm::InfectionState>>
                location_ids_person{};
            for (auto&& person : sim.get_world().get_persons()) {
                location_ids_person.push_back(std::make_tuple(person.get_location_id(), person.get_person_id(),
                                                              person.get_time_since_transmission(),
                                                              person.get_infection_state()));
            }
            return location_ids_person;
        }
    };

    History<DataWriterToBuffer, LogTimePoint, LogLocationIds, LogPersonsPerLocationAndInfectionTime> history;

    sim.advance(tmax, history);

    auto logg = history.get_log();

    // Write the results to a file.
    auto loc_id = std::get<1>(logg);
    auto rest   = std::get<2>(logg);
    std::string input;
    std::ofstream myfile("julias_output.txt");
    for (auto loc_id_index = 0; loc_id_index < loc_id[0].size(); ++loc_id_index) {
        input  = convert_loc_id_to_string(loc_id[0][loc_id_index]);
        auto a = get_agents_per_location(loc_id[0][loc_id_index], rest[0]);
        // for (auto rest_index = 0; rest_index < rest[0].size(); ++rest_index) {

        // }
        myfile << input << "\n";
    }
    myfile.close();

    // The results are saved in a table with 9 rows.
    // The first row is t = time, the others correspond to the number of people with a certain infection state at this time:
    // S = Susceptible, E = Exposed, C= Carrier, I= Infected, I_s = Infected_Severe,
    // I_c = Infected_Critical, R_C = Recovered_Carrier, R_I = Recovered_Infected, D = Dead
    auto f_abm = fopen("abm_minimal.txt", "w");
    fprintf(f_abm, "# t S E C I I_s I_c R_C R_I D\n");
    for (auto i = 0; i < sim.get_result().get_num_time_points(); ++i) {
        fprintf(f_abm, "%f ", sim.get_result().get_time(i));
        auto v = sim.get_result().get_value(i);
        for (auto j = 0; j < v.size(); ++j) {
            fprintf(f_abm, "%f", v[j]);
            if (j < v.size() - 1) {
                fprintf(f_abm, " ");
            }
        }
        if (i < sim.get_result().get_num_time_points() - 1) {
            fprintf(f_abm, "\n");
        }
    }
    fclose(f_abm);
    std::cout << "Results written to abm_minimal.txt" << std::endl;
}
