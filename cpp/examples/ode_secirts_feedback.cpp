/* 
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Henrik Zunker
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
#include "ode_secirts/analyze_result.h"
#include "ode_secirts/model.h"
#include "ode_secirts/parameters.h"
#include "memilio/compartments/simulation.h"
#include "memilio/utils/logging.h"

int main()
{
    mio::set_log_level(mio::LogLevel::debug);

    double t0   = 0;
    double tmax = 5;
    double dt   = 0.1;

    mio::log_info("Simulating SECIRTS; t={} ... {} with dt = {}.", t0, tmax, dt);

    mio::osecirts::Model model(3);
    auto nb_groups = model.parameters.get_num_groups();

    for (mio::AgeGroup i = 0; i < nb_groups; i++) {
        // population
        model.populations[{i, mio::osecirts::InfectionState::ExposedNaive}]                                = 20;
        model.populations[{i, mio::osecirts::InfectionState::ExposedImprovedImmunity}]                     = 20;
        model.populations[{i, mio::osecirts::InfectionState::ExposedPartialImmunity}]                      = 20;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsNaive}]                     = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsNaiveConfirmed}]            = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsPartialImmunity}]           = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsPartialImmunityConfirmed}]  = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsImprovedImmunity}]          = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedNoSymptomsImprovedImmunityConfirmed}] = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsNaive}]                       = 40;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsNaiveConfirmed}]              = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsPartialImmunity}]             = 40;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsPartialImmunityConfirmed}]    = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsImprovedImmunity}]            = 40;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSymptomsImprovedImmunityConfirmed}]   = 0;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSevereNaive}]                         = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSevereImprovedImmunity}]              = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedSeverePartialImmunity}]               = 30;
        model.populations[{i, mio::osecirts::InfectionState::InfectedCriticalNaive}]                       = 2;
        model.populations[{i, mio::osecirts::InfectionState::InfectedCriticalPartialImmunity}]             = 2;
        model.populations[{i, mio::osecirts::InfectionState::InfectedCriticalImprovedImmunity}]            = 2;
        model.populations[{i, mio::osecirts::InfectionState::SusceptibleNaive}]                            = 1000;
        model.populations[{i, mio::osecirts::InfectionState::SusceptiblePartialImmunity}]                  = 1200;
        model.populations[{i, mio::osecirts::InfectionState::SusceptibleImprovedImmunity}]                 = 1000;
        model.populations[{i, mio::osecirts::InfectionState::TemporaryImmunPartialImmunity}]               = 60;
        model.populations[{i, mio::osecirts::InfectionState::TemporaryImmunImprovedImmunity}]              = 70;
        model.populations[{i, mio::osecirts::InfectionState::DeadNaive}]                                   = 0;
        model.populations[{i, mio::osecirts::InfectionState::DeadPartialImmunity}]                         = 0;
        model.populations[{i, mio::osecirts::InfectionState::DeadImprovedImmunity}]                        = 0;

        // parameters
        //times
        model.parameters.get<mio::osecirts::TimeExposed>()[i]                = 3.33;
        model.parameters.get<mio::osecirts::TimeInfectedNoSymptoms>()[i]     = 1.87;
        model.parameters.get<mio::osecirts::TimeInfectedSymptoms>()[i]       = 7;
        model.parameters.get<mio::osecirts::TimeInfectedSevere>()[i]         = 6;
        model.parameters.get<mio::osecirts::TimeInfectedCritical>()[i]       = 7;
        model.parameters.get<mio::osecirts::TimeTemporaryImmunityPI>()[i]    = 60;
        model.parameters.get<mio::osecirts::TimeTemporaryImmunityPI>()[i]    = 60;
        model.parameters.get<mio::osecirts::TimeTemporaryImmunityII>()[i]    = 60;
        model.parameters.get<mio::osecirts::TimeWaningPartialImmunity>()[i]  = 180;
        model.parameters.get<mio::osecirts::TimeWaningImprovedImmunity>()[i] = 180;

        //probabilities
        model.parameters.get<mio::osecirts::TransmissionProbabilityOnContact>()[i]  = 0.15;
        model.parameters.get<mio::osecirts::RelativeTransmissionNoSymptoms>()[i]    = 0.5;
        model.parameters.get<mio::osecirts::RiskOfInfectionFromSymptomatic>()[i]    = 0.0;
        model.parameters.get<mio::osecirts::MaxRiskOfInfectionFromSymptomatic>()[i] = 0.4;
        model.parameters.get<mio::osecirts::RecoveredPerInfectedNoSymptoms>()[i]    = 0.2;
        model.parameters.get<mio::osecirts::SeverePerInfectedSymptoms>()[i]         = 0.1;
        model.parameters.get<mio::osecirts::CriticalPerSevere>()[i]                 = 0.1;
        model.parameters.get<mio::osecirts::DeathsPerCritical>()[i]                 = 0.1;

        model.parameters.get<mio::osecirts::ReducExposedPartialImmunity>()[i]                     = 0.8;
        model.parameters.get<mio::osecirts::ReducExposedImprovedImmunity>()[i]                    = 0.331;
        model.parameters.get<mio::osecirts::ReducInfectedSymptomsPartialImmunity>()[i]            = 0.65;
        model.parameters.get<mio::osecirts::ReducInfectedSymptomsImprovedImmunity>()[i]           = 0.243;
        model.parameters.get<mio::osecirts::ReducInfectedSevereCriticalDeadPartialImmunity>()[i]  = 0.1;
        model.parameters.get<mio::osecirts::ReducInfectedSevereCriticalDeadImprovedImmunity>()[i] = 0.091;
        model.parameters.get<mio::osecirts::ReducTimeInfectedMild>()[i]                           = 0.9;
    }

    model.parameters.get<mio::osecirts::ICUCapacity>()          = 100;
    model.parameters.get<mio::osecirts::TestAndTraceCapacity>() = 0.0143;
    const size_t daily_vaccinations                             = 10;
    const size_t num_days                                       = 300;
    model.parameters.get<mio::osecirts::DailyPartialVaccination>().resize(mio::SimulationDay(num_days));
    model.parameters.get<mio::osecirts::DailyFullVaccination>().resize(mio::SimulationDay(num_days));
    model.parameters.get<mio::osecirts::DailyBoosterVaccination>().resize(mio::SimulationDay(num_days));
    for (size_t i = 0; i < num_days; ++i) {
        for (mio::AgeGroup j = 0; j < nb_groups; ++j) {
            auto num_vaccinations = static_cast<double>(i * daily_vaccinations);
            model.parameters.get<mio::osecirts::DailyPartialVaccination>()[{j, mio::SimulationDay(i)}] =
                num_vaccinations;
            model.parameters.get<mio::osecirts::DailyFullVaccination>()[{j, mio::SimulationDay(i)}] = num_vaccinations;
            model.parameters.get<mio::osecirts::DailyBoosterVaccination>()[{j, mio::SimulationDay(i)}] =
                num_vaccinations;
        }
    }

    mio::ContactMatrixGroup& contact_matrix = model.parameters.get<mio::osecirts::ContactPatterns>();
    const double cont_freq                  = 10;
    const double fact                       = 1.0 / (double)(size_t)nb_groups;
    contact_matrix[0] =
        mio::ContactMatrix(Eigen::MatrixXd::Constant((size_t)nb_groups, (size_t)nb_groups, fact * cont_freq));

    model.parameters.get<mio::osecirts::ContactReductionMin>() = {0.0};
    model.parameters.get<mio::osecirts::ContactReductionMax>() = {0.3};

    model.parameters.get<mio::osecirts::Seasonality>() = 0.2;

    // set values for daily icu occupancy in the past
    auto& icu_occupancy     = model.parameters.get<mio::osecirts::ICUOccupancyLocal>();
    Eigen::VectorXd icu_day = Eigen::VectorXd::Zero(nb_groups.get());
    for (int t = -50; t < 0; ++t) {
        for (size_t i = 0; i < nb_groups.get(); i++) {
            icu_day(i) = (50 + t);
        }
        icu_occupancy.add_time_point(t, icu_day);
    }
    model.apply_constraints();

    // auto integrator = std::make_shared<mio::EulerIntegratorCore>();
    auto result = simulate_flows(t0, tmax, dt, model);

    bool print_to_terminal = false;

    if (print_to_terminal) {
        auto result_interpolated = mio::interpolate_simulation_result(result[0]);
        for (auto t_indx = 0; t_indx < result_interpolated.get_num_time_points(); t_indx++) {
            double timm_pi = 0.0;
            double timm_ii = 0.0;
            for (mio::AgeGroup i = 0; i < nb_groups; i++) {
                timm_pi += result_interpolated.get_value(t_indx)[model.populations.get_flat_index(
                    {i, mio::osecirts::InfectionState::TemporaryImmunPartialImmunity})];
                timm_ii += result_interpolated.get_value(t_indx)[model.populations.get_flat_index(
                    {i, mio::osecirts::InfectionState::TemporaryImmunImprovedImmunity})];
            }
            printf("t=%i, timm_pi=%f, timm_ii=%f\n", int(result_interpolated.get_time(t_indx)), timm_pi, timm_ii);
        }
    }
}