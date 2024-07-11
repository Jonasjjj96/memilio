/* 
* Copyright (C) 2020-2024 MEmilio
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
#include "memilio/mobility/metapopulation_mobility_detailed.h"
#include "memilio/io/mobility_io.h"
#include "memilio/compartments/parameter_studies.h"
#include "ode_secir/parameter_space.h"
#include "ode_secir/parameters_io.h"
#include <iostream>

int main()
{
    const auto t0   = 0.;
    const auto dt   = 0.5;
    const auto tmax = 10.;

    ScalarType cont_freq = 10; // see Polymod study

    ScalarType nb_total_t0 = 10000, nb_exp_t0 = 100, nb_inf_t0 = 50, nb_car_t0 = 50, nb_hosp_t0 = 20, nb_icu_t0 = 10,
               nb_rec_t0 = 10, nb_dead_t0 = 0;

    mio::osecir::Model<ScalarType> model(1);
    mio::AgeGroup nb_groups = model.parameters.get_num_groups();
    ScalarType fact         = 1.0 / (ScalarType)(size_t)nb_groups;

    auto& params = model.parameters;

    params.set<mio::osecir::ICUCapacity<ScalarType>>(std::numeric_limits<ScalarType>::max());
    params.set<mio::osecir::StartDay>(0);
    params.set<mio::osecir::Seasonality<ScalarType>>(0);

    for (auto i = mio::AgeGroup(0); i < nb_groups; i++) {
        params.get<mio::osecir::TimeExposed<ScalarType>>()[i]            = 3.2;
        params.get<mio::osecir::TimeInfectedNoSymptoms<ScalarType>>()[i] = 2.0;
        params.get<mio::osecir::TimeInfectedSymptoms<ScalarType>>()[i]   = 6.;
        params.get<mio::osecir::TimeInfectedSevere<ScalarType>>()[i]     = 12;
        params.get<mio::osecir::TimeInfectedCritical<ScalarType>>()[i]   = 8;

        model.populations[{i, mio::osecir::InfectionState::Exposed}]                     = fact * nb_exp_t0;
        model.populations[{i, mio::osecir::InfectionState::InfectedNoSymptoms}]          = fact * nb_car_t0;
        model.populations[{i, mio::osecir::InfectionState::InfectedNoSymptomsConfirmed}] = 0;
        model.populations[{i, mio::osecir::InfectionState::InfectedSymptoms}]            = fact * nb_inf_t0;
        model.populations[{i, mio::osecir::InfectionState::InfectedSymptomsConfirmed}]   = 0;
        model.populations[{i, mio::osecir::InfectionState::InfectedSevere}]              = fact * nb_hosp_t0;
        model.populations[{i, mio::osecir::InfectionState::InfectedCritical}]            = fact * nb_icu_t0;
        model.populations[{i, mio::osecir::InfectionState::Recovered}]                   = fact * nb_rec_t0;
        model.populations[{i, mio::osecir::InfectionState::Dead}]                        = fact * nb_dead_t0;
        model.populations.set_difference_from_group_total<mio::AgeGroup>({i, mio::osecir::InfectionState::Susceptible},
                                                                         fact * nb_total_t0);

        params.get<mio::osecir::TransmissionProbabilityOnContact<ScalarType>>()[i] = 0.05;
        params.get<mio::osecir::RelativeTransmissionNoSymptoms<ScalarType>>()[i]   = 0.67;
        params.get<mio::osecir::RecoveredPerInfectedNoSymptoms<ScalarType>>()[i]   = 0.09;
        params.get<mio::osecir::RiskOfInfectionFromSymptomatic<ScalarType>>()[i]   = 0.25;
        params.get<mio::osecir::SeverePerInfectedSymptoms<ScalarType>>()[i]        = 0.2;
        params.get<mio::osecir::CriticalPerSevere<ScalarType>>()[i]                = 0.25;
        params.get<mio::osecir::DeathsPerCritical<ScalarType>>()[i]                = 0.3;
    }

    params.apply_constraints();

    mio::ContactMatrixGroup& contact_matrix = params.get<mio::osecir::ContactPatterns<ScalarType>>();
    contact_matrix[0] =
        mio::ContactMatrix(Eigen::MatrixXd::Constant((size_t)nb_groups, (size_t)nb_groups, fact * cont_freq));

    mio::GraphDetailed<mio::SimulationNode<mio::Simulation<ScalarType, mio::osecir::Model<ScalarType>>>,
                       mio::MigrationEdgeDetailed<ScalarType>>
        g;
    for (size_t county_id = 0; county_id < 3; county_id++) {
        g.add_node(county_id, model, 0.0);
    }

    // Graph is always complete here
    for (size_t county_idx_i = 0; county_idx_i < g.nodes().size(); ++county_idx_i) {
        for (size_t county_idx_j = 0; county_idx_j < g.nodes().size(); ++county_idx_j) {
            if (county_idx_i == county_idx_j)
                continue;
            g.add_edge(county_idx_i, county_idx_j,
                       Eigen::VectorXd::Constant(
                           (size_t)mio::osecir::InfectionState::Count * static_cast<size_t>(nb_groups), 0.01));
        }
    }

    auto sim = mio::make_migration_sim(t0, dt, std::move(g));
    sim.advance(tmax);

    return 0;
}
