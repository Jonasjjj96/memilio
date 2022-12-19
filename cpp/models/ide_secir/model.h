/* 
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Anna Wendler, Lena Ploetzke
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

#ifndef IDESECIR_MODEL_H
#define IDESECIR_MODEL_H

#include "ide_secir/parameters.h"
#include "ide_secir/infection_state.h"
#include "memilio/utils/time_series.h"

namespace mio
{
namespace isecir
{
class Model
{
    /* TODO: 
    - Kontaktmatrix ist nicht richtig definiert? Warum wird 0 dafür ausgegeben
    - Beispiel läuft durch, aber Werte passen nicht; haben u.a. keine konstante Bevölkerungszahl
    - Indizes in Funktionen und Output für Testbeispiel genau(!!) überprüfen
    - in parameters: unordered map statt transitions
    - Evtl. allgemeinen Integrator als non-standrd diff. scheme anstatt aktueller Approximation
    - Eigen Indizees für zugriff auf diese Timeseries
    - R berechnen über die Flows (für Reinfection später)
    */
    using Pa = ParametersBase;

public:
    /**
    * @brief Constructor to create an IDE SECIR model.
    *
    * @param[in, out] init TimeSeries with the initial values of the number of individuals,
    *   which transition according to the transitions defined in infection_state.h at associated initial times.
    *   The time steps should be equidistant and equal to the time step used for the simulation. 
    *   A certain history of time steps and values for the transitions is needed. 
    *   A warning is displayed if the condition is violated.
    *   The last time point in this vector should be a time 0.
    * @param[in] dt_init The size of the time step used for numerical simulation.
    * @param[in] N_init The population of the considered region. 
    * @param[in, out] Parameterset_init used Parameters for simulation. 
    */
    Model(TimeSeries<ScalarType>&& init, ScalarType dt_init, size_t N_init, size_t Dead0, Pa Parameterset_init = Pa());

    /**
        * @brief Simulate the evolution of infection numbers with the given IDE SECIR model.
        *
        * The simulation is performed by solving the underlying model equation numerically. 
        * Here, an integro-differential equation is to be solved. The model parameters and the initial data are used.
        *
        * @param[in] t_max Last simulation day. 
        *   If the last point of time of the initial TimeSeries was 0, the simulation will be executed for t_max days.
        * @return The result of the simulation, stored in a TimeSeries with simulation time and 
        *       associated number of susceptibles.
        */
    void simulate(int t_max);
    // Used Parameters for the simulation.
    Pa parameters{};

    // function that computes size of compartments from flows
    // define function more general as a sum of some \mu, some \gamma and some \sigma
    // later insert corresponding function
    void get_size_of_compartments(int t_max);

    void print_transitions() const;
    
    void print_compartments() const;

private:
    // Funktion to update current value for S
    void update_susceptibles();
    void update_forceofinfection();
    void compute_flow(int idx_InfectionTransitions, ScalarType TransitionProbability, int idx_TransitionDistribution);
    void compute_totaldeaths();
    void get_size_of_compartments(int idx_IncomingFlow, ScalarType TransitionProbability,
                                  int idx_TransitionDistribution, int idx_TransitionDistributionToRecov,
                                  int idx_InfectionState);

    // TimeSeries containing points of time and the corresponding number of transitions.
    TimeSeries<ScalarType> m_transitions;
    // TimeSeries containing points of time and the corresponding number of people in defined Infectionstates.
    TimeSeries<ScalarType> m_SECIR;
    /*Attention: m_SECIR and m_transitions do not necessarily have the same number of time points*/

    // Force of infection term needed for numerical scheme, corresponds to phi
    ScalarType m_forceofinfection{0};

    // Timestep used for simulation.
    ScalarType m_dt{0};
    // Population of the considered region.
    size_t m_N{0};

    // Two Indices used for simulation.
    Eigen::Index m_k{0};
    Eigen::Index m_l{0};
};

// define general function to compute size of compartments from flows
//
struct FlowToCompartment {
};

} // namespace isecir
} // namespace mio

#endif // IDESECIR_MODEL_H
