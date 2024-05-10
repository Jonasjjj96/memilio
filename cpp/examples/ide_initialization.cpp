/* 
* Copyright (C) 2020-2024 MEmilio
*
* Authors:  Lena Ploetzke, Anna Wendler
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

#include "ide_secir/model.h"
#include "ide_secir/infection_state.h"
#include "ide_secir/simulation.h"
#include "ide_secir/parameters_io.h"
#include "memilio/config.h"
#include "memilio/utils/time_series.h"
#include "memilio/utils/date.h"
#include "memilio/math/eigen.h"
#include <string>
#include <vector>
#include <iostream>

/**
 * @brief Function to check the parameters provided in the command line.
 */
std::string setup(int argc, char** argv)
{
    if (argc == 2) {
        std::cout << "Using file " << argv[1] << "." << std::endl;
        return (std::string)argv[1];
    }
    else {
        if (argc > 2) {
            mio::log_warning("Too many arguments given.");
        }
        else {
            mio::log_warning("No arguments given.");
        }
        return "";
    }
}

int main(int argc, char** argv)
{
    // This is a simple example to demonstrate how to set initial data for the IDE-SECIR model using real data.
    // A default initialization is used if no filename is provided in the command line.
    // Have a look at the documentation of the set_initial_flows() function in models/ide_secir/parameters_io.h for a
    // description of how to download suitable data.
    // A valid filename could be for example "../../data/pydata/Germany/cases_all_germany_ma7.json" if the functionality to download real data is used.
    // The default parameters of the IDE-SECIR model are used, so that the simulation results are not realistic and are for demonstration purpose only.

    // Initialize model.
    ScalarType total_population = 80 * 1e6;
    ScalarType deaths = 0; // The number of deaths will be overwritten if real data is used for initialization.
    ScalarType dt     = 0.5;
    mio::isecir::Model model(mio::TimeSeries<ScalarType>((int)mio::isecir::InfectionTransition::Count),
                             total_population, deaths);

    // Check provided parameters.
    std::string filename = setup(argc, argv);
    if (filename.empty()) {
        std::cout << "You did not provide a valid filename. A default initialization is used." << std::endl;

        using Vec = mio::TimeSeries<ScalarType>::Vector;
        mio::TimeSeries<ScalarType> init((int)mio::isecir::InfectionTransition::Count);
        init.add_time_point<Eigen::VectorXd>(-7., Vec::Constant((int)mio::isecir::InfectionTransition::Count, 1. * dt));
        while (init.get_last_time() < -dt / 2) {
            init.add_time_point(init.get_last_time() + dt,
                                Vec::Constant((int)mio::isecir::InfectionTransition::Count, 1. * dt));
        }
        model.m_transitions = init;
    }
    else {
        // Use the real data for initialization.
        auto status = mio::isecir::set_initial_flows(model, dt, filename, mio::Date(2020, 12, 24));
        if (!status) {
            std::cout << "Error: " << status.error().formatted_message();
            return -1;
        }
    }

    // Carry out simulation.
    mio::isecir::Simulation sim(model, dt);
    sim.advance(2.);

    // Print results.
    sim.get_transitions().print_table({"S->E", "E->C", "C->I", "C->R", "I->H", "I->R", "H->U", "H->R", "U->D", "U->R"},
                                      16, 8);

    return 0;
}