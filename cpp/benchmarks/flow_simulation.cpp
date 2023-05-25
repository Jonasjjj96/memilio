/* 
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Rene Schmieding
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
#include "benchmarks/simulation.h"
#include "benchmarks/flow_simulation.h"
#include "ode_secirvvs/model.h"
#include <string>

const std::string config_path = "../../benchmarks/simulation.config";

// simulation without flows (not in Model definition and not calculated by Simulation)
void flowless_sim(::benchmark::State& state)
{
    using Model = mio::benchmark::FlowlessModel;
    // suppress non-critical messages
    mio::set_log_level(mio::LogLevel::critical);
    // load config
    auto cfg = mio::benchmark::SimulationConfig::initialize(config_path);
    // create model
    Model model(cfg.num_agegroups);
    mio::benchmark::setup_model(model);
    // create simulation
    mio::benchmark::Simulation<mio::Simulation<Model>> sim(model, cfg.t0, cfg.dt);
    std::shared_ptr<mio::IntegratorCore> I =
        std::make_shared<mio::ControlledStepperWrapper<boost::numeric::odeint::runge_kutta_cash_karp54>>(
            cfg.abs_tol, cfg.rel_tol, cfg.dt_min, cfg.dt_max);
    // run benchmark
    for (auto _ : state) {
        // This code gets timed
        sim.advance(cfg.t_max);
    }
}

// simulation with flows (in Model definition, but NOT calculated by Simulation)
void flow_sim_comp_only(::benchmark::State& state)
{
    using Model = mio::benchmark::FlowModel;
    // suppress non-critical messages
    mio::set_log_level(mio::LogLevel::critical);
    // load config
    auto cfg = mio::benchmark::SimulationConfig::initialize(config_path);
    // create model
    Model model(cfg.num_agegroups);
    mio::benchmark::setup_model(model);
    // create simulation
    mio::osecirvvs::Simulation<mio::Simulation<Model>> sim(model, cfg.t0, cfg.dt);
    std::shared_ptr<mio::IntegratorCore> I =
        std::make_shared<mio::ControlledStepperWrapper<boost::numeric::odeint::runge_kutta_cash_karp54>>(
            cfg.abs_tol, cfg.rel_tol, cfg.dt_min, cfg.dt_max);
    // run benchmark
    for (auto _ : state) {
        // This code gets timed
        sim.advance(cfg.t_max);
    }
}

// simulation with flows (in Model definition and calculated by Simulation)
void flow_sim(::benchmark::State& state)
{
    using Model = mio::benchmark::FlowModel;
    // suppress non-critical messages
    mio::set_log_level(mio::LogLevel::critical);
    // load config
    auto cfg = mio::benchmark::SimulationConfig::initialize(config_path);
    // create model
    Model model(cfg.num_agegroups);
    mio::benchmark::setup_model(model);
    // create simulation
    mio::osecirvvs::Simulation<mio::FlowSimulation<Model>> sim(model, cfg.t0, cfg.dt);
    std::shared_ptr<mio::IntegratorCore> I =
        std::make_shared<mio::ControlledStepperWrapper<boost::numeric::odeint::runge_kutta_cash_karp54>>(
            cfg.abs_tol, cfg.rel_tol, cfg.dt_min, cfg.dt_max);
    // run benchmark
    for (auto _ : state) {
        // This code gets timed
        sim.advance(cfg.t_max);
    }
}

// register functions as a benchmarks and set a name
BENCHMARK(flowless_sim)->Name("Simulate compartments (without flows)");
BENCHMARK(flow_sim_comp_only)->Name("Simulate compartments (with flows)");
BENCHMARK(flow_sim)->Name("Simulate both compartments and flows");
// run all benchmarks
BENCHMARK_MAIN();
