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

class TestModel
    : public mio::CompartmentalModel<I, mio::Populations<I, CatA, CatB, CatC>, mio::oseir::Parameters, Flows>
{
    using Base = CompartmentalModel<I, mio::Populations<I, CatA, CatB, CatC>, mio::oseir::Parameters, Flows>;

public:
    TestModel()
        : Base(Populations({I::Count, CatA(11), CatB(5), CatC(7)}, 0.), mio::oseir::Parameters{})
    {
    }
};

TEST(TestFlows, FlowChart)
{
    EXPECT_EQ(Flows().size(), 3);
    // Testing get (by index) function, verifying with source members.
    auto flow0        = mio::Flow<I, I::Susceptible, I::Exposed>().source;
    auto test_source0 = Flows().get<0>().source;
    EXPECT_EQ(flow0, test_source0);

    auto flow1        = mio::Flow<I, I::Exposed, I::Infected>().source;
    auto test_source1 = Flows().get<1>().source;
    EXPECT_EQ(flow1, test_source1);

    auto flow2        = mio::Flow<I, I::Infected, I::Recovered>().source;
    auto test_source2 = Flows().get<2>().source;
    EXPECT_EQ(flow2, test_source2);

    // Testing get (by Flow) function.
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
    // verify results (computed using flows)
    auto results = seir[0].get_last_value();
    EXPECT_NEAR(results[0], 9660.5835936179428, 1e-14);
    EXPECT_NEAR(results[1], 118.38410512338653, 1e-14);
    EXPECT_NEAR(results[2], 104.06636087558746, 1e-14);
    EXPECT_NEAR(results[3], 116.96594038308582, 1e-14);
    // test flow results
    auto flows_results = seir[1].get_last_value();
    EXPECT_NEAR(flows_results[0], 39.416406382059776, 1e-14);
    EXPECT_NEAR(flows_results[1], 21.032301258673261, 1e-14);
    EXPECT_NEAR(flows_results[2], 16.965940383085815, 1e-14);
}

TEST(TestFlows, GetInitialFlows)
{
    TestModel m;
    EXPECT_EQ(m.get_initial_flows().size(), 3); // 3 == Flows().size()
    EXPECT_EQ(m.get_initial_flows().norm(), 0);
}

TEST(TestFlows, GetFlowIndex)
{
    // test get_flow_index with some prime number products
    TestModel m;
    auto idx0 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(0), CatB(0), CatC(1)});
    EXPECT_EQ(idx0, 3);

    auto idx1 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(0), CatB(1), CatC(0)});
    EXPECT_EQ(idx1, 7 * 3);

    auto idx2 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(1), CatB(0), CatC(0)});
    EXPECT_EQ(idx2, 5 * 7 * 3);

    auto idx3 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(1), CatB(1), CatC(1)});
    EXPECT_EQ(idx3, (5 * 7 * 3) + (7 * 3) + (3));

    auto idx4 = m.get_flow_index<I::Susceptible, I::Exposed>({CatA(10), CatB(4), CatC(6)});
    EXPECT_EQ(idx4, 10 * (5 * 7 * 3) + 4 * (7 * 3) + 6 * (3));
}
