/* 
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Rene Schmieding, Henrik Zunker 
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
#include "load_test_data.h"
#include "memilio/compartments/compartmentalmodel.h"
#include "memilio/utils/compiler_diagnostics.h"
#include "memilio/utils/parameter_set.h"
#include "ode_seir/model.h"
#include "ode_seir/infection_state.h"
#include "ode_seir/parameters.h"
#include "memilio/utils/flow_chart.h"
#include "memilio/compartments/simulation.h"
#include "gtest/gtest.h"
#include "matchers.h"

using I     = mio::oseir::InfectionState;
using Flows = mio::FlowChart<mio::Flow<I, I::Susceptible, I::Exposed>, mio::Flow<I, I::Exposed, I::Infected>,
                             mio::Flow<I, I::Infected, I::Recovered>>;

TEST(TestFlows, FlowChart)
{
    EXPECT_EQ(Flows().size(), 3);
    // Testing get function using the source members.
    auto flow0        = mio::Flow<I, I::Susceptible, I::Exposed>().source;
    auto test_source0 = Flows().get<0>().source;
    EXPECT_EQ(flow0, test_source0);

    auto flow1        = mio::Flow<I, I::Exposed, I::Infected>().source;
    auto test_source1 = Flows().get<1>().source;
    EXPECT_EQ(flow1, test_source1);

    auto flow2        = mio::Flow<I, I::Infected, I::Recovered>().source;
    auto test_source2 = Flows().get<2>().source;
    EXPECT_EQ(flow2, test_source2);

    using Flow0 = mio::Flow<I, I::Susceptible, I::Exposed>;
    EXPECT_EQ(Flows().get<Flow0>(), 0);

    using Flow1 = mio::Flow<I, I::Exposed, I::Infected>;
    EXPECT_EQ(Flows().get<Flow1>(), 1);

    using Flow2 = mio::Flow<I, I::Infected, I::Recovered>;
    EXPECT_EQ(Flows().get<Flow2>(), 2);
}

TEST(TestFlows, SimulationFlows)
{
    mio::set_log_level(mio::LogLevel::off);

    double t0   = 0;
    double tmax = 1;
    double dt   = 0.001;

    mio::oseir::Model model;

    double total_population                                                                            = 10000;
    model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Exposed)}]   = 100;
    model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Infected)}]  = 100;
    model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Recovered)}] = 100;
    model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Susceptible)}] =
        total_population -
        model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Exposed)}] -
        model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Infected)}] -
        model.populations[{mio::Index<mio::oseir::InfectionState>(mio::oseir::InfectionState::Recovered)}];
    // suscetible now set with every other update
    // params.nb_sus_t0   = params.nb_total_t0 - params.nb_exp_t0 - params.nb_inf_t0 - params.nb_rec_t0;
    model.parameters.set<mio::oseir::TimeExposed>(5.2);
    model.parameters.set<mio::oseir::TimeInfected>(6);
    model.parameters.set<mio::oseir::TransmissionProbabilityOnContact>(0.04);
    model.parameters.get<mio::oseir::ContactPatterns>().get_baseline()(0, 0) = 10;

    model.check_constraints();
    auto seir = simulate_flows(t0, tmax, dt, model);

    auto flows_results = seir[1].get_last_value();
    EXPECT_NEAR(flows_results[0], 39.416406382059776, 1e-14);
    EXPECT_NEAR(flows_results[1], 21.032301258673261, 1e-14);
    EXPECT_NEAR(flows_results[2], 16.965940383085815, 1e-14);
}

struct CatA : public mio::Index<CatA> {
    CatA(size_t i)
        : mio::Index<CatA>(i)
    {
    }
};
struct CatB : public mio::Index<CatB> {
    CatB(size_t i)
        : mio::Index<CatB>(i)
    {
    }
};
struct CatC : public mio::Index<CatC> {
    CatC(size_t i)
        : mio::Index<CatC>(i)
    {
    }
};

// class TestModel
//     : public mio::CompartmentalModel<I, mio::Populations<I, CatA, CatB, CatC>, mio::oseir::Parameters, Flows>
// {
//     using Base = CompartmentalModel<I, mio::Populations<I, CatA, CatB, CatC>, mio::oseir::Parameters, Flows>;

// public:
//     TestModel()
//         : Base(Populations({I::Count, CatA(2), CatB(3), CatC(5)}, 0.), mio::oseir::Parameters{})
//     {
//     }
// };

TEST(TestFlows, Compartmentalmodel)
{
    // TestModel m;
    // EXPECT_EQ(m.get_initial_flows().size(), 3);
    // auto idx0 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(1), CatB(0), CatC(0)});
    // EXPECT_EQ(idx0, 2);
    // auto idx1 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(0), CatB(1), CatC(0)});
    // EXPECT_EQ(idx1, 3);
    // auto idx2 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(0), CatB(0), CatC(1)});
    // EXPECT_EQ(idx2, 5);
}