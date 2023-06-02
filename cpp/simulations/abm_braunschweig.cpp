#include <fstream>
#include <vector>
#include <iostream>
#include "abm/abm.h"
#include "memilio/io/result_io.h"
#include "memilio/utils/uncertain_value.h"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/replace.hpp"
#include "boost/algorithm/string/classification.hpp"

namespace fs = boost::filesystem;

// /**
//  * Set a value and distribution of an UncertainValue.
//  * Assigns average of min and max as a value and UNIFORM(min, max) as a distribution.
//  * @param p uncertain value to set.
//  * @param min minimum of distribution.
//  * @param max minimum of distribution.
//  */
// void assign_uniform_distribution(mio::UncertainValue& p, ScalarType min, ScalarType max)
// {
//     p = mio::UncertainValue(0.5 * (max + min));
//     p.set_distribution(mio::ParameterDistributionUniform(min, max));
// }

/**
 * Determine the infection state of a person at the beginning of the simulation.
 * The infection states are chosen randomly. They are distributed according to the probabilites set in the example.
 * @return random infection state
 */
mio::abm::InfectionState determine_infection_state(ScalarType exposed, ScalarType infected, ScalarType carrier,
                                                   ScalarType recovered)
{
    ScalarType susceptible          = 1 - exposed - infected - carrier - recovered;
    std::vector<ScalarType> weights = {susceptible,  exposed,      carrier,       infected / 3,
                                       infected / 3, infected / 3, recovered / 2, recovered / 2};
    if (weights.size() != (size_t)mio::abm::InfectionState::Count - 1) {
        mio::log_error("Initialization in ABM wrong, please correct vector length.");
    }
    auto state = mio::DiscreteDistribution<size_t>::get_instance()(weights);
    return (mio::abm::InfectionState)state;
}

/**
 * Assign an infection state to each person.
 */
void assign_infection_state(mio::abm::World& world, ScalarType exposed_pct, ScalarType infected_pct,
                            ScalarType carrier_pct, ScalarType recovered_pct)
{
    auto persons = world.get_persons();
    for (auto& person : persons) {
        world.set_infection_state(person,
                                  determine_infection_state(exposed_pct, infected_pct, carrier_pct, recovered_pct));
    }
}

void split_line(std::string string, std::vector<int32_t>* row)
{
    std::vector<std::string> strings;
    boost::replace_all(string, ";;", ";-1;"); // Temporary fix to handle empty cells.
    boost::split(strings, string, boost::is_any_of(";"));
    std::transform(strings.begin(), strings.end(), std::back_inserter(*row), [&](std::string s) {
        return std::stoi(s);
    });
}

mio::abm::LocationType get_location_type(uint32_t acitivity_end)
{
    mio::abm::LocationType type;
    switch (acitivity_end) {
    case 1:
        type = mio::abm::LocationType::Work;
        break;
    case 2:
        type = mio::abm::LocationType::School;
        break;
    case 3:
        type = mio::abm::LocationType::BasicsShop;
        break;
    case 4:
        type = mio::abm::LocationType::SocialEvent; // Freizeit
        break;
    case 5:
        type = mio::abm::LocationType::Home; // Private Erledigung
        break;
    case 6:
        type = mio::abm::LocationType::SocialEvent; // Sonstiges
        break;
    default:
        type = mio::abm::LocationType::Home;
        break;
    }
    return type;
}

void create_world_from_data(mio::abm::World& world, const std::string& filename)
{
    // File pointer
    std::fstream fin;

    // Open an existing file
    fin.open(filename, std::ios::in);
    std::vector<int32_t> row;
    std::vector<std::string> row_string;
    std::string line;

    std::getline(fin, line);
    std::vector<std::string> titles;
    boost::split(titles, line, boost::is_any_of(";"));
    uint32_t count                        = 0;
    std::map<std::string, uint32_t> index = {};
    for (auto const& title : titles) {
        index.insert({title, count});
        row_string.push_back(title);
        count++;
    }
    std::map<uint32_t, mio::abm::LocationId> locations = {};
    std::map<uint32_t, mio::abm::Person&> persons      = {};

    while (std::getline(fin, line)) {
        row.clear();

        // read columns in this row
        split_line(line, &row);

        uint32_t person_id   = row[index["puid"]];
        uint32_t age         = row[index["age"]]; // TODO
        uint32_t location_id = row[index["loc_id_end"]];
        uint32_t home_id     = row[index["huid"]];
        uint32_t activity    = row[index["activity_end"]];

        auto it_home              = locations.find(home_id);
        mio::abm::LocationId home = it_home->second;
        if (it_home == locations.end()) {
            home = world.add_location(mio::abm::LocationType::Home, 0);
            locations.insert({home_id, home});
        }
        auto it_person = persons.find(person_id);
        if (it_person == persons.end()) {
            auto& person =
                world.add_person(home, mio::abm::InfectionState::Susceptible, static_cast<mio::abm::AgeGroup>(age));
            person.set_assigned_location(home);
            persons.insert({person_id, person});
            it_person = persons.find(person_id);
        }
        auto it_location              = locations.find(location_id);
        mio::abm::LocationId location = it_location->second;
        if (get_location_type(activity) != mio::abm::LocationType::Home) {
            if (it_location == locations.end()) {
                location = world.add_location(get_location_type(activity), 0);
                locations.insert({location_id, location});
            }
            it_person->second.set_assigned_location(location);
        }
    }
}

/**
 * Create a sampled simulation with start time t0.
 * @param t0 the start time of the simulation
*/
mio::abm::Simulation create_sampled_simulation(const mio::abm::TimePoint& t0)
{

    // Assumed percentage of infection state at the beginning of the simulation.
    ScalarType exposed_pct = 0.005, infected_pct = 0.001, carrier_pct = 0.001, recovered_pct = 0.0;

    //Set global infection parameters (similar to infection parameters in SECIR model) and initialize the world
    mio::abm::GlobalInfectionParameters infection_params;
    auto world = mio::abm::World(infection_params);

    // Create the world object from statistical data.
    create_world_from_data(world, "../data/mobility/bs.csv");

    // Assign an infection state to each person.
    assign_infection_state(world, exposed_pct, infected_pct, carrier_pct, recovered_pct);

    auto t_lockdown = mio::abm::TimePoint(0) + mio::abm::days(20);

    // During the lockdown, 25% of people work from home and schools are closed for 90% of students.
    // Social events are very rare.
    mio::abm::set_home_office(t_lockdown, 0.25, world.get_migration_parameters());
    mio::abm::set_school_closure(t_lockdown, 0.9, world.get_migration_parameters());
    mio::abm::close_social_events(t_lockdown, 0.9, world.get_migration_parameters());

    auto sim = mio::abm::Simulation(t0, std::move(world));
    return sim;
}

mio::IOResult<void> run(const fs::path& result_dir, size_t num_runs, bool save_single_runs = true)
{

    auto t0               = mio::abm::TimePoint(0); // Start time per simulation
    auto tmax             = mio::abm::TimePoint(0) + mio::abm::days(1); // End time per simulation
    auto ensemble_results = std::vector<std::vector<mio::TimeSeries<ScalarType>>>{}; // Vector of collected results
    ensemble_results.reserve(size_t(num_runs));
    auto run_idx            = size_t(1); // The run index
    auto save_result_result = mio::IOResult<void>(mio::success()); // Variable informing over successful IO operations

    // Loop over a number of runs
    while (run_idx <= num_runs) {

        // Create the sampled simulation with start time t0.
        auto sim = create_sampled_simulation(t0);
        // Collect the id of location in world.
        std::vector<int> loc_ids;
        for (auto&& locations : sim.get_world().get_locations()) {
            for (auto location : locations) {
                loc_ids.push_back(location.get_index());
            }
        }
        // Advance the world to tmax
        sim.advance(tmax);
        // TODO: update result of the simulation to be a vector of location result.
        auto temp_sim_result = std::vector<mio::TimeSeries<ScalarType>>{sim.get_result()};
        // Push result of the simulation back to the result vector
        ensemble_results.push_back(temp_sim_result);
        // Option to save the current run result to file
        if (save_result_result && save_single_runs) {
            auto result_dir_run = result_dir / ("abm_result_run_" + std::to_string(run_idx) + ".h5");
            BOOST_OUTCOME_TRY(save_result(ensemble_results.back(), loc_ids, 1, result_dir_run.string()));
        }
        ++run_idx;
    }
    BOOST_OUTCOME_TRY(save_result_result);
    return mio::success();
}

int main(int argc, char** argv)
{
    mio::set_log_level(mio::LogLevel::warn);

    std::string result_dir = ".";
    size_t num_runs        = 1;
    bool save_single_runs  = true;

    if (argc == 2) {
        num_runs = atoi(argv[1]);
        printf("Number of run is %s.\n", argv[1]);
        printf("Saving results to the current directory.\n");
    }

    else if (argc == 3) {
        num_runs   = atoi(argv[1]);
        result_dir = argv[2];
        printf("Number of run is %s.\n", argv[1]);
        printf("Saving results to \"%s\".\n", result_dir.c_str());
    }
    else {
        printf("Usage:\n");
        printf("abm_example <num_runs>\n");
        printf("\tRun the simulation for <num_runs> time(s).\n");
        printf("\tStore the results in the current directory.\n");
        printf("abm_example <num_runs> <result_dir>\n");
        printf("\tRun the simulation for <num_runs> time(s).\n");
        printf("\tStore the results in <result_dir>.\n");
        // return 0;
    }

    // mio::thread_local_rng().seed({...}); //set seeds, e.g., for debugging
    //printf("Seeds: ");
    //for (auto s : mio::thread_local_rng().get_seeds()) {
    //    printf("%u, ", s);
    //}
    //printf("\n");

    auto result = run(result_dir, num_runs, save_single_runs);
    if (!result) {
        printf("%s\n", result.error().formatted_message().c_str());
        return -1;
    }
    return 0;
}
