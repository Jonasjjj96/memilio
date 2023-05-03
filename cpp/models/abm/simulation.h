/* 
* Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
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
#ifndef EPI_ABM_SIMULATOR_H
#define EPI_ABM_SIMULATOR_H

#include "abm/world.h"
#include "abm/time.h"
#include "memilio/utils/time_series.h"
#include "memilio/utils/history.h"

namespace mio
{
namespace abm
{

/**
 * @brief Run the Simulation in discrete steps, evolve the World and report results.
 */
class Simulation
{
    using ResultVector = Eigen::Matrix<int, Eigen::Index(InfectionState::Count), 1>;

public:
    /**
     * @brief Create a simulation.
     * @param[in] t The starting time of the Simulation.
     * @param[in] world The World to simulate.
     */
    Simulation(TimePoint t, World&& world);

    /**
     * @brief Create a Simulation with an empty World.
     * World needs to be filled later.
     * @see Simulation::get_world
     * @param[in] t The starting time of the Simulation.
     */
    Simulation(TimePoint t)
        : Simulation(t, World())
    {
    }

    /** 
     * @brief Run the Simulation from the current time to tmax.
     * @param[in] tmax Time to stop.
     */
    void advance(TimePoint tmax);

    /** 
     * @brief Run the Simulation from the current time to tmax.
     * @param[in] tmax Time to stop.
     * @param[in] history History to store the result of the Simulation.
     */
    template <typename History>
    void advance(TimePoint tmax, History& history)
    {
        //log initial system state
        history.log(*this);

        while (m_t < tmax) {
            auto dt = std::min(m_dt, tmax - m_t);
            m_world.evolve(m_t, dt);
            m_t += m_dt;
            history.log(*this);
            store_result_at(m_t);
        }
    }

    /**
     * @brief Get the result of the Simulation.
     * Sum over all Location%s of the number of Person%s in an #InfectionState.
     */
    const TimeSeries<double>& get_result() const
    {
        return m_result;
    }

    /**
     * @brief Get the current time of the Simulation.
     */
    TimePoint get_time() const
    {
        return m_t;
    }

    /**
     * @brief Get the World that this Simulation evolves.
     */
    World& get_world()
    {
        return m_world;
    }
    const World& get_world() const
    {
        return m_world;
    }

private:
    void store_result_at(TimePoint t);

    World m_world; ///< The World to simulate.
    TimeSeries<double> m_result; ///< The result of the Simulation.
    TimePoint m_t; ///< The current TimePoint of the Simulation.
    TimeSpan m_dt; ///< The length of the time steps.
};

} // namespace abm
} // namespace mio

#endif
