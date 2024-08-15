import subprocess
import os
import pandas as pd

from get_lognormal_parameters import get_lognormal_parameters
from plot_real_scenario import plot_new_infections, plot_infectedsymptoms_deaths, plot_icu, get_scale_contacts


def run_real_scenario(data_dir, save_dir, start_date, simulation_time, timestep, mu_UD, T_UD, T_UR, std_UD, std_UR, scale_contacts):

    year = start_date.split("-")[0]
    month = start_date.split("-")[1]
    day = start_date.split("-")[2]

    shape_UD, scale_UD = get_lognormal_parameters(T_UD, std_UD)
    shape_UR, scale_UR = get_lognormal_parameters(T_UR, std_UR)

    subprocess.call([f"./build/bin/ide_real_scenario", data_dir, save_dir, f"{mu_UD}", f"{T_UD}", f"{T_UR}", f"{shape_UD}", f"{scale_UD}", f"{shape_UR}", f"{scale_UR}",
                     f"{year}", f"{month}", f"{day}", f"{simulation_time}", f"{timestep}", f"{scale_contacts}"])


def contact_scaling(save_dir, start_date, simulation_time, timestep, T_UD):
    scale_contacts = get_scale_contacts([os.path.join(
        save_dir, f"ide_{start_date}_{simulation_time}_{timestep}_flows")], pd.Timestamp(start_date), simulation_time, T_UD)

    return scale_contacts


def plot_real_scenario(save_dir, plot_dir, start_date, simulation_time, timestep, T_UD):
    plot_new_infections([os.path.join(save_dir, f"ode_{start_date}_{simulation_time}_{timestep}_flows"),
                        os.path.join(save_dir, f"ide_{start_date}_{simulation_time}_{timestep}_flows")],
                        pd.Timestamp(start_date), simulation_time, T_UD,
                        fileending=f"{start_date}_{simulation_time}_{timestep}", save=True, save_dir=plot_dir)

    plot_infectedsymptoms_deaths([os.path.join(save_dir, f"ode_{start_date}_{simulation_time}_{timestep}_compartments"),
                                  os.path.join(save_dir, f"ide_{start_date}_{simulation_time}_{timestep}_compartments")],
                                 pd.Timestamp(
                                     start_date), simulation_time, T_UD,
                                 fileending=f"{start_date}_{simulation_time}_{timestep}", save=True, save_dir=plot_dir)

    plot_icu([os.path.join(save_dir, f"ode_{start_date}_{simulation_time}_{timestep}_compartments"),
              os.path.join(save_dir, f"ide_{start_date}_{simulation_time}_{timestep}_compartments")],
             pd.Timestamp(start_date), simulation_time,  fileending=f"{start_date}_{simulation_time}_{timestep}", save=True, save_dir=plot_dir)


def october_scenario():
    start_date = '2020-10-1'
    simulation_time = 45
    timestep = "0.1000"

    # Try differen parameters regarding U.
    # Assessment
    mu_UD_assessment = 0.217177
    # Covasim
    mu_UD_covasim = 0.387803
    probs = [mu_UD_covasim]  # mu_UD_assessment,
    T_UD = 10.7
    T_UDs = [10.7]
    # T_UDs = [T_UD-i*0.5 for i in range(5)]
    std_UD = 4.8
    T_UR = 18.1
    T_URs = [18.1]
    # T_URs = [T_UR-i*0.5 for i in range(5)]
    std_UR = 6.3

    for T_UD in T_UDs:
        for T_UR in T_URs:
            for mu_UD in probs:
                data_dir = "./data"
                save_dir = f"./results/real/{start_date}/tryU/{mu_UD}_{T_UD}_{T_UR}/"
                plot_dir = f"plots/real/{start_date}/tryU/{mu_UD}_{T_UD}_{T_UR}/"

                # First run the simulation with a contact scaling of 1.
                scale_contacts = 1
                run_real_scenario(data_dir, save_dir, start_date, simulation_time,
                                  timestep, mu_UD, T_UD, T_UR, std_UD, std_UR, scale_contacts)
                # Then determine contact scaling such that IDE results and RKI new infections match at first timestep.
                scale_contacts = contact_scaling(
                    save_dir, start_date, simulation_time, timestep, T_UD)
                # Run simulation with new contact scaling.
                run_real_scenario(data_dir, save_dir, start_date, simulation_time,
                                  timestep, mu_UD, T_UD, T_UR, std_UD, std_UR, scale_contacts)
                plot_real_scenario(save_dir, plot_dir, start_date,
                                   simulation_time, timestep, T_UD)


def june_scenario():
    start_date = '2020-6-1'
    simulation_time = 45
    timestep = "0.1000"

    mu_UD = 0.217177
    T_UD = 10.7
    std_UD = 4.8
    T_UR = 18.1
    std_UR = 6.3

    data_dir = "./data"
    save_dir = f"./results/real/{start_date}/tryU/{mu_UD}_{T_UD}_{T_UR}/"
    plot_dir = f"plots/real/{start_date}/tryU/{mu_UD}_{T_UD}_{T_UR}/"

    # First run the simulation with a contact scaling of 1.
    scale_contacts = 1
    run_real_scenario(data_dir, save_dir, start_date, simulation_time,
                      timestep, mu_UD, T_UD, T_UR, std_UD, std_UR, scale_contacts)
    # Then determine contact scaling such that IDE results and RKI new infections match at first timestep.
    scale_contacts = contact_scaling(
        save_dir, start_date, simulation_time, timestep, T_UD)
    # Run simulation with new contact scaling.
    run_real_scenario(data_dir, save_dir, start_date, simulation_time,
                      timestep, mu_UD, T_UD, T_UR, std_UD, std_UR, scale_contacts)
    plot_real_scenario(save_dir, plot_dir, start_date,
                       simulation_time, timestep, T_UD)


def main():
    october_scenario()

    # june_scenario()


if __name__ == "__main__":

    main()
