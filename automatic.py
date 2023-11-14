"""Combines automatic download of public generated data via memilio-epidata,
extrapolation into compartments via cpp code and upload to ESID database"""

import os
import datetime
import json
import subprocess

from datetime import date
from zipfile import ZipFile

import click

from memilio.epidata import getCaseData as gcd
from memilio.epidata import getPopulationData as gpd
from memilio.epidata import getDIVIData as gdd
from memilio.epidata import getVaccinationData as gvd
from memilio.epidata import progress_indicator


def read_input_data(end_date, path_to_input_data, username='', password=''):
    """! Reads data needed to compute number of individuals in compartments. 

    Downloads case data, population data, DIVI data and vaccination data.

    @param[in] start_date Start date of data.
    @param[in] path_to_input_data Path where data is saved.
    """
    arg_dict = {'out_folder': "{}/pydata".format(path_to_input_data), 'end_date': end_date,
                'moving_average': 7}
    arg_dict_pop = {'out_folder': "{}/pydata".format(path_to_input_data),
                    "username": username,
                    "password": password}

    progress_indicator.ProgressIndicator.disable_indicators(True)
    gcd.get_case_data(**arg_dict)
    gpd.get_population_data(**arg_dict_pop)
    gdd.get_divi_data(**arg_dict)
    gvd.get_vaccination_data(**arg_dict)


def compute_compartments_from_input_data(start_date, path_to_input_data, num_days_sim):

    # NOT WORKING
    """! Executes script to compute compartment data.

    Creates 'Results.h5' and 'Results_sum.h5' and store them in path_to_input_data.

    @param[in] start_date Start date of data.
    @param[in] path_to_input_data Path where data is saved.
    @param[in] num_days_sim Number of days that are is simulated. 
    """
    year = start_date.year
    month = start_date.month
    day = start_date.day
    subprocess.run(["./build/bin/generate_graph_from_data {} {} {} {} {}".format(
        path_to_input_data, str(year), str(month), str(day), str(num_days_sim))], shell=True, check=False)


def prepare_data_for_backend(start_date, path_to_input_data, path_to_output_data):
    """! Prepares output data for backend. 

    Creates metadata.json and zip-folder containing metadata.json, Results.h5 
    and Results_sum.h5.

    @param[in] start_date Start date of data.
    @param[in] path_to_output_data Path to output data.
    """
    # Define dict with metadata
    metadata_dict = {
        "startDay": "{}".format(start_date.strftime("%Y-%m-%d")),
        "datasets": ["Group1", "Group2", "Group3", "Group4", "Group5", "Group6", "Total"],
        "groupMapping": {
            "Group1": "age_0",
            "Group2": "age_1",
            "Group3": "age_2",
            "Group4": "age_3",
            "Group5": "age_4",
            "Group6": "age_5",
            "Total": "total"
        },
        "compartmentOrder": [
            "MildInfections",
            "Hospitalized",
            "ICU",
            "Dead"
        ]
    }

    # write metadata.json
    with open(os.path.join(path_to_output_data, "metadata.json"), "w") as outfile:
        json.dump(metadata_dict, outfile, indent=4, sort_keys=False)

    # make zip
    with ZipFile(os.path.join(path_to_output_data, "rki.zip"), 'w') as zip_object:
        zip_object.write(os.path.join(path_to_input_data,
                         "metadata.json"),  "metadata.json")
        zip_object.write(os.path.join(
            path_to_input_data, "Results.h5"), "Results.h5")
        zip_object.write(os.path.join(path_to_input_data,
                         "Results_sum.h5"), "Results_sum.h5")


def import_to_backend(path_to_esid, path_to_output_data):
    """! Imports data into backend.

    First, copy output data in backend folder from ESID.
    Then, import it into database. 

    @param[in] path_to_esid Path to ESID repository.
    @param[in] path_to_output_data Path to output data.
    """

    path_to_backend = os.path.join(path_to_esid, "backend")

    subprocess.run(
        ["cp -f {}/rki.zip {}/rki.zip".format(path_to_output_data, path_to_backend)], shell=True, check=False)
    os.chdir(path_to_backend)
    subprocess.run(
        ["USER_ID=$(id -u) GROUP_ID=$(id -g) docker-compose -f docker-compose.dev.yml up -d"], shell=True, check=False)
    subprocess.run(
        ["USER_ID=$(id -u) GROUP_ID=$(id -g) docker-compose -f docker-compose.dev.yml run --rm backend python manage.py import_rki rki.zip"], shell=True, check=False)


@click.command()
@click.option('--path_to_esid', prompt='Path to ESID database', help='Path to ESID database.')
def main(path_to_esid):
    """Main program to execute all steps."""
    # Number of days to take into account
    num_days_sim = 10

    # Set start date relative to current date
    # Add 1 because the data we fetch today only contains data until yesterday,
    # so start_date needs to be computed wrt to yesterday
    start_date = date.today() - datetime.timedelta(days=num_days_sim+1)
    # Set end_date to yesterday because we can only fetch data from the day before
    end_date = date.today() - datetime.timedelta(days=1)

    # Set paths to folder with input data and folder with output data
    path_to_input_data = './data_test'
    path_to_output_data = './data_test'

    read_input_data(end_date, path_to_input_data)

    compute_compartments_from_input_data(
        start_date, path_to_input_data, num_days_sim)

    prepare_data_for_backend(
        start_date, path_to_input_data, path_to_output_data)
    import_to_backend(path_to_esid, path_to_output_data)


if __name__ == "__main__":
    main()
