#############################################################################
# Copyright (C) 2020-2024 MEmilio
#
# Authors: Martin J. Kuehn, Wadim Koslow, Daniel Abele, Khoa Nguyen
#
# Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################
import argparse
from datetime import date, datetime

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import math
from scipy.linalg import eigvals
from scipy.sparse.linalg import eigs

from memilio.simulation import AgeGroup, ContactMatrix, Damping, UncertainContactMatrix
from memilio.simulation.secir import Index_InfectionState
from memilio.simulation.secir import InfectionState as State
from memilio.simulation.secir import (Model, Simulation,
                                      interpolate_simulation_result, simulate)

# Checks after a run if stiffness occured and when stiffness occurred first
# Checking for stiffness after every timestep would be too expensive
# Simulation can then be run from first occurence of stiffness with different (f.ex. implicit) integrator
# y: TimeSeries
# stability_func: Stability function of the integrator used
def posteriori_stiffness(stability_func, y,params):
    # Don't check the last timepoint, as we don't step from there
    interior_timepoints = y.get_num_time_points()-1
    for t_idx in range(0,interior_timepoints):
        # Calculate the Jacobian
        num_agegroups = params.get_num_groups()
        pi = math.pi
        test_and_trace_required = 0.0
        icu_occupancy = 0.0
        result = y.as_ndarray()
        group_data = np.transpose(result[1:,:])

        Jacobian = np.zeros((num_agegroups*8,num_agegroups*8))
        
        season_val = (1 + params.Seasonality) *math.sin(pi * (math.fmod((params.StartDay + y.get_time(t_idx)),365.0) /182.5 +0.5))

        def smoother_cosine(x,xleft,xright,yleft,yright):
            if x <= xleft:
                return yleft
            
            if x >= xright:
                return yright
            
            return 0.5 * (yleft - yright) * math.cos(3.14159265358979323846 / (xright - xleft) * (x - xleft)) + 0.5 * (yleft + yright)


        for i in range(0,num_agegroups):
            Ai = AgeGroup(i)
            rateINS = 0.5/(params.IncubationTime[Ai]-params.SerialInterval[Ai])
            test_and_trace_required += (1 - params.RecoveredPerInfectedNoSymptoms[Ai])*rateINS*group_data[t_idx,i*10+2]
            icu_occupancy += group_data[t_idx,i*10+5]

            S_dummy = 0
            for j in range(0,num_agegroups):
                Aj = AgeGroup(j)
                cont_freq_eff = season_val * params.ContactPatterns.cont_freq_mat[t_idx](i,j)
                Nj = group_data[t_idx,j*10+11] # total population in InfectionState::Count
                divNj = 1.0/Nj
                riskFromInfectedSymptomatic = smoother_cosine(test_and_trace_required, params.TestAndTraceCapacity,params.TestAndTraceCapacity * 5, params.RiskOfInfectionFromSymptomatic[Aj],
                params.MaxRiskOfInfectionFromSymptomatic[Aj])
                S_dummy -= cont_freq_eff*divNj*params.TransmissionProbabilityOnContact[Ai]*(params.RelativeTransmissionNoSymptoms[Aj]*group_data[t_idx,j*10+2] + riskFromInfectedSymptomatic*group_data[t_idx,j*10+4])
            Jacobian[i,i] = S_dummy
            Jacobian[i+num_agegroups,i+num_agegroups] = - S_dummy

            for j in range(0,num_agegroups):
                Aj = AgeGroup(j)
                Aj = AgeGroup(j)
                cont_freq_eff = season_val * params.ContactPatterns.cont_freq_mat[t_idx](i,j)
                Nj = group_data[t_idx,j*10+11] # total population in InfectionState::Count
                divNj = 1.0/Nj
                riskFromInfectedSymptomatic = smoother_cosine(test_and_trace_required, params.TestAndTraceCapacity,params.TestAndTraceCapacity * 5, params.RiskOfInfectionFromSymptomatic[Aj],
                params.MaxRiskOfInfectionFromSymptomatic[Aj])
                dummy = 

        eigen_vals = eigvals(Jacobian)
        # Determine the used stepsize: 
        h = y.get_time(t_idx+1)-y.get_time(t_idx) 
        for eigen in eigen_vals:
            if abs(stability_func(eigen*h)) > 1:
                # Return the first index at which an instability occurs:
                return t_idx
    # If no instability occurs, return the last time index
    return y.get_num_time_points()-1

def run_secir_simulation(show_plot=True):
    """
    Runs the c++ secir model using one age group 
    and plots the results
    """

    # Define Comartment names
    compartments = [
        'Susceptible', 'Exposed', 'InfectedNoSymptoms', 'InfectedSymptoms',
        'InfectedSevere', 'InfectedCritical', 'Recovered', 'Dead']
    # Define population of age groups
    populations = [83000]

    days = 100  # number of days to simulate
    start_day = 1
    start_month = 1
    start_year = 2019
    dt = 0.1
    num_groups = 1
    num_compartments = len(compartments)

    # Initialize Parameters
    model = Model(1)

    A0 = AgeGroup(0)

    # Set parameters

    # Compartment transition duration
    model.parameters.IncubationTime[A0] = 5.2
    model.parameters.TimeInfectedSymptoms[A0] = 6.
    # 4-4.4 // R_2^(-1)+0.5*R_3^(-1)
    model.parameters.SerialInterval[A0] = 4.2
    model.parameters.TimeInfectedSevere[A0] = 12.  # 7-16 (=R5^(-1))
    model.parameters.TimeInfectedCritical[A0] = 8.

    # Initial number of people in each compartment
    model.populations[A0, State.Exposed] = 100
    model.populations[A0, State.InfectedNoSymptoms] = 50
    model.populations[A0, State.InfectedNoSymptomsConfirmed] = 0
    model.populations[A0, State.InfectedSymptoms] = 50
    model.populations[A0, State.InfectedSymptomsConfirmed] = 0
    model.populations[A0, State.InfectedSevere] = 20
    model.populations[A0, State.InfectedCritical] = 10
    model.populations[A0, State.Recovered] = 10
    model.populations[A0, State.Dead] = 0
    model.populations.set_difference_from_total(
        (A0, State.Susceptible), populations[0])

    # Compartment transition propabilities
    model.parameters.RelativeTransmissionNoSymptoms[A0] = 0.67
    model.parameters.TransmissionProbabilityOnContact[A0] = 1.0
    model.parameters.RecoveredPerInfectedNoSymptoms[A0] = 0.09  # 0.01-0.16
    model.parameters.RiskOfInfectionFromSymptomatic[A0] = 0.25  # 0.05-0.5
    model.parameters.SeverePerInfectedSymptoms[A0] = 0.2  # 0.1-0.35
    model.parameters.CriticalPerSevere[A0] = 0.25  # 0.15-0.4
    model.parameters.DeathsPerCritical[A0] = 0.3  # 0.15-0.77
    # twice the value of RiskOfInfectionFromSymptomatic
    model.parameters.MaxRiskOfInfectionFromSymptomatic[A0] = 0.5

    model.parameters.StartDay = (
        date(start_year, start_month, start_day) - date(start_year, 1, 1)).days

    # model.parameters.ContactPatterns.cont_freq_mat[0] = ContactMatrix(np.r_[0.5])
    model.parameters.ContactPatterns.cont_freq_mat[0].baseline = np.ones(
        (num_groups, num_groups)) * 1
    model.parameters.ContactPatterns.cont_freq_mat[0].minimum = np.ones(
        (num_groups, num_groups)) * 0
    model.parameters.ContactPatterns.cont_freq_mat.add_damping(
        Damping(coeffs=np.r_[0.9], t=30.0, level=0, type=0))

    # Apply mathematical constraints to parameters
    model.apply_constraints()

    # Run Simulation
    result = simulate(0, days, dt, model)
    # interpolate results
    result = interpolate_simulation_result(result)

    print(result.get_last_value())
    num_time_points = result.get_num_time_points()
    result_array = result.as_ndarray()
    t = result_array[0, :]
    group_data = np.transpose(result_array[1:, :])

    # sum over all groups
    data = np.zeros((num_time_points, num_compartments))
    for i in range(num_groups):
        data += group_data[:, i * num_compartments: (i + 1) * num_compartments]

    # Plot Results
    datelist = np.array(
        pd.date_range(
            datetime(start_year, start_month, start_day),
            periods=days, freq='D').strftime('%m-%d').tolist())

    tick_range = (np.arange(int(days / 10) + 1) * 10)
    tick_range[-1] -= 1
    fig, ax = plt.subplots()
    ax.plot(t, data[:, 0], label='#Susceptible')
    ax.plot(t, data[:, 1], label='#Exposed')
    ax.plot(t, data[:, 2], label='#Carrying')
    ax.plot(t, data[:, 3], label='#InfectedSymptoms')
    ax.plot(t, data[:, 4], label='#Hospitalzed')
    ax.plot(t, data[:, 5], label='#InfectedCritical')
    ax.plot(t, data[:, 6], label='#Recovered')
    ax.plot(t, data[:, 7], label='#Died')
    ax.set_title("SECIR model simulation")
    ax.set_xticks(tick_range)
    ax.set_xticklabels(datelist[tick_range], rotation=45)
    ax.legend()
    fig.tight_layout
    fig.savefig('Secir_simple.pdf')

    if show_plot:
        plt.show()
        plt.close()


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(
        'secir_simple',
        description='Simple example demonstrating the setup and simulation of the SECIR model.')
    arg_parser.add_argument('-p', '--show_plot',
                            action='store_const', const=True, default=False)
    args = arg_parser.parse_args()
    run_secir_simulation(**args.__dict__)
