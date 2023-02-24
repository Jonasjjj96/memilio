from matplotlib import ticker
import matplotlib.pyplot as plt
import copy
import datetime
import os
import h5py
import numpy as np
import pandas as pd
plt.rcParams['figure.facecolor'] = 'w'
plt.rcParams['axes.facecolor'] = 'w'


def open_files(
        path_sim, path_rki, scenario_list, spec_str_sim, spec_str_rki1,
        spec_str_rki2='', percentiles=['p50', 'p25', 'p75', 'p05', 'p95'],
        read_casereports_extrapolation=False):
    """! Open files from an given folder containing simulation files.
    @param[in] path_sim Path containing the simulation files.
    @param[in] path_rki Path where extrapolated real data was written.
    @param[in] scenario_list List of string indicators for scenarios to be plotted.
    @param[in] spec_str_sim Specified string after results (e.g. date) that points to a specific set of scenario folders
    @param[in] spec_str_rki1 Specified string in results folder (e.g. date) that points to a specific RKI data folder
    @param[in] spec_str_rki2 [Default: ""] Specified string in results file that points to a specific RKI data file
    @param[in] percentiles [Default: ['p50', 'p25', 'p75', 'p05', 'p95']] List of percentiles to be printed (sublist from ['p50','p25','p75','p05','p95'])
    @param[in] read_casereports_extrapolation [Default: False] Defines if extrapolated reporting data (from RKI) will be loaded
    @return Dictionary containing data
    """

    files = {}
    for scenario in scenario_list:
        files[scenario] = {}
        for p in percentiles:
            files[scenario][p] = h5py.File(
                os.path.join(path_sim, p, 'Results_sum.h5'), 'r')
        if read_casereports_extrapolation:
            files[scenario]['RKI'] = h5py.File(
                path_rki + spec_str_rki1 + '/Results_rki_sum' + spec_str_rki2 +
                '.h5', 'r')
    return files


def close_files(files):
    """! Closes file handles in @files.
    @param[in] files File handles of open HDF5 files
    """
    for group in files:
        for file in files[group]:
            files[group][file].close()


# def get_cmap(n, name):
#     """! Returns a function that maps each index in 0, 1, ..., n-1 to a distinct
#     RGB color; the keyword argument name must be a standard mpl colormap name.
#     """
#     return plt.cm.get_cmap(name, n)


def plot_results_secir(
        files, comp_idx, title, figsize, colors, tick_range, datelist,
        save_plot=True, plot_percentiles=True, plot_confidence_interval=True,
        plot_RKI=True, factor_relative=1, regionid='0', key='Total',
        line_width=3.5, fontsize=18):
    """! Open files from an given folder containing simulation files.
    @param[in] path_sim Path containing the simulation files.
    @param[in] path_rki Path where extrapolated real data was written.
    @param[in] scenario_list List of string indicators for scenarios to be plotted.
    @param[in] spec_str_sim Specified string after results (e.g. date) that points to a specific set of scenario folders
    @param[in] spec_str_rki1 Specified string in results folder (e.g. date) that points to a specific RKI data folder
    @param[in] spec_str_rki2 [Default: ""] Specified string in results file that points to a specific RKI data file
    @param[in] percentiles [Default: ['p50', 'p25', 'p75', 'p05', 'p95']] List of percentiles to be printed (sublist from ['p50','p25','p75','p05','p95'])
    @param[in] read_casereports_extrapolation [Default: False] Defines if extrapolated reporting data (from RKI) will be loaded
    @return Dictionary containing data
    """
    fig,  ax = plt.subplots(figsize=figsize)

    if 'Group1' not in files['p50'].keys():
        files_plot_p50 = files['p50'][regionid]
        X = files['p50'][regionid]['Time'][:]
    else:  # backward stability for IO as of 2020/2021
        files_plot_p50 = files['p50']
        X = files['p50']['Time'][:]

    ax.plot(X, files_plot_p50[key][:, comp_idx]/factor_relative, label='p50',
            color=colors[key], linewidth=line_width)
    if plot_percentiles:
        if 'Group1' not in files['p25'].keys():
            files_plot_p25 = files['p25'][regionid]
            files_plot_p75 = files['p75'][regionid]
        else:  # backward stability for IO as of 2020/2021
            files_plot_p25 = files['p25']
            files_plot_p75 = files['p75']

        ax.plot(
            X, files_plot_p25[key][:, comp_idx] / factor_relative, '--',
            label='p25', color=colors[key],
            linewidth=line_width)
        ax.plot(
            X, files_plot_p75[key][:, comp_idx] / factor_relative, '--',
            label='p75', color=colors[key],
            linewidth=line_width)
        ax.fill_between(X, files_plot_p25[key][:, comp_idx]/factor_relative,
                        files_plot_p75[key][:, comp_idx]/factor_relative,
                        color=colors[key], alpha=0.15)
    if plot_confidence_interval:
        if 'Group1' not in files['p05'].keys():
            files_plot_p05 = files['p05'][regionid]
            files_plot_p95 = files['p95'][regionid]
        else:  # backward stability for IO as of 2020/2021
            files_plot_p05 = files['p05']
            files_plot_p95 = files['p95']

        ax.plot(
            X, files_plot_p05[key][:, comp_idx] / factor_relative, '--',
            label='p05', color=colors[key],
            linewidth=line_width)
        ax.plot(
            X, files_plot_p95[key][:, comp_idx] / factor_relative, '--',
            label='p95', color=colors[key],
            linewidth=line_width)
        ax.fill_between(X, files_plot_p05[key][:, comp_idx]/factor_relative,
                        files_plot_p95[key][:, comp_idx]/factor_relative,
                        color=colors[key], alpha=0.15)

    if plot_RKI:
        if 'RKI' in files.keys():
            if 'Group1' not in files['p05'].keys():
                files_rki = files['RKI'][regionid]
            else:  # backward stability for IO as of 2020/2021
                files_rki = files['RKI']
            ax.plot(
                X, files_rki[key][:, comp_idx] / factor_relative, '--',
                label='RKI', color='gray', linewidth=line_width)
        else:
            print('Error: Plotting extrapolated real data demanded but not read in.')

    ax.set_title(title, fontsize=18)
    ax.set_xticks(tick_range)
    ax.set_xticklabels(datelist[tick_range], rotation=45, fontsize=fontsize)
    if factor_relative != 1:
        ax.set_ylabel('individuals relative per 100.000', fontsize=fontsize)
    else:
        ax.set_ylabel('number of individuals', fontsize=fontsize)
    ax.legend(fontsize=fontsize)
    plt.yticks(fontsize=fontsize)
    ax.grid(linestyle='dotted')

    formatter = ticker.ScalarFormatter(useMathText=True)
    formatter.set_scientific(True)
    formatter.set_powerlimits((-1, 1))
    ax.yaxis.set_major_formatter(formatter)
    ax.yaxis.offsetText.set_fontsize(fontsize)

    if save_plot:
        fig.savefig('Plots/' + title + '.png')


# Functions for ode_secirvvs

# Plots one compartment for an individual scenario
def plot_results_secirvvs(
        files, comp_idx, title, figsize, colors, tick_range, days_plot,
        datelist, plot_RKI=True, plot_percentiles=True,
        plot_confidence_interval=True, ylim=None, filename='', key='Total',
        save_plot=True, plotLegend=True, addVal=0, fontsize=28,
        factor_relative=1, regionid='0', line_width=3.5):
    fig, ax = plt.subplots(figsize=figsize)

    # define x-ticks for plots
    X = np.arange(days_plot)
    tick_range = (np.arange(int(days_plot / 10) + 1) * 10)
    tick_range[-1] -= 1

    ax.plot(
        X, (addVal + files['p50'][regionid][key][: days_plot, comp_idx]) /
        factor_relative, label='p50', color=colors[key],
        linewidth=line_width)
    if plot_percentiles:
        ax.plot(
            X, (addVal + files['p25'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, '--', label='p25', color=colors[key],
            linewidth=days_plot)
        ax.plot(
            X, (addVal + files['p75'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, '--', label='p75', color=colors[key],
            linewidth=line_width)
        ax.fill_between(
            X, (addVal + files['p25'][regionid][key][: days_plot, comp_idx]) /
            factor_relative,
            (addVal + files['p75'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, color=colors[key],
            alpha=0.15)
    if plot_confidence_interval:
        ax.plot(
            X, (addVal + files['p05'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, '--', label='p05', color=colors[key],
            linewidth=line_width)
        ax.plot(
            X, (addVal + files['p95'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, '--', label='p95', color=colors[key],
            linewidth=line_width)
        ax.fill_between(
            X, (addVal + files['p05'][regionid][key][: days_plot, comp_idx]) /
            factor_relative,
            (addVal + files['p95'][regionid][key][: days_plot, comp_idx]) /
            factor_relative, color=colors[key],
            alpha=0.15)

    if plot_RKI:
        if 'RKI' in files.keys():
            ax.plot(
                X, files['RKI'][regionid][key][: days_plot, comp_idx] /
                factor_relative, '--', label='extrapolated real data',
                color='gray', linewidth=line_width)
        else:
            print('Error: Plotting extrapolated real data demanded but not read in.')

    ax.set_title(title, fontsize=fontsize)
    ax.set_xticks(tick_range)
    ax.set_xticklabels(
        datelist[tick_range],
        rotation=45, fontsize=fontsize)
    if factor_relative != 1:
        ax.set_ylabel('individuals relative per 100.000', fontsize=fontsize)
    else:
        ax.set_ylabel('number of individuals', fontsize=fontsize)
    if plotLegend:
        ax.legend(fontsize=fontsize, loc='upper left')
    plt.yticks(fontsize=fontsize)
    ax.grid(linestyle='dotted')

    if str(ylim) != 'None':
        if '_high' in filename:
            ylim[filename.replace('_high', '')] = ax.get_ylim()[1]
        else:
            ax.set_ylim(top=ylim[filename])

    formatter = ticker.ScalarFormatter(useMathText=True)
    formatter.set_scientific(True)
    formatter.set_powerlimits((-1, 1))
    ax.yaxis.set_major_formatter(formatter)
    ax.yaxis.offsetText.set_fontsize(fontsize)

    fig.tight_layout()

    if save_plot:
        if plot_RKI:
            fig.savefig(
                'Plots/RKI_' + title.replace(' ', '_') + filename + '.png')
        else:
            fig.savefig('Plots/' + title.replace(' ', '_') + filename + '.png')

    return ylim


# Plots one compartment for all scenarios
def plot_all_results_secirvvs(
        all_files, comp_idx, title, figsize, colors, tick_range, days_plot,
        datelist, plot_RKI=True, plot_percentiles=True,
        plot_confidence_interval=True, ylim=None, filename='', key='Total',
        save_plot=True, plotLegend=True, addVal=0, fontsize=28,
        factor_relative=1, regionid='0', line_width=3.5):
    fig, ax = plt.subplots(figsize=figsize)

    for scenario, color in zip(all_files, list(colors.values())[1:]):
        files = all_files[scenario]
        plot_results_secirvvs(
            files, comp_idx, title, figsize, color, tick_range, days_plot,
            datelist, plot_RKI=plot_RKI, plot_percentiles=plot_percentiles,
            plot_confidence_interval=plot_confidence_interval, ylim=ylim,
            filename=filename, key=key, save_plot=save_plot,
            plotLegend=plotLegend, addVal=addVal, fontsize=figsize,
            factor_relative=factor_relative, regionid=regionid,
            line_width=line_width)


def plot_bars(
        show_perc, relative_dict, name, files, columns, rows, lim=100,
        rel=False, compart=9, regionid='0', fontsize=20):
    color = plt.rcParams['axes.prop_cycle'].by_key()['color']

    n_rows = len(rows)
    num_groups = 6
    bar_width = 1/len(files)
    index = np.arange(n_rows)*bar_width
    scen_width = 8/len(files)

    keys = list(files.keys())

    cell_text = []
    fig, ax = plt.subplots(figsize=(20, 10))
    # ax = fig.add_axes([0,0,1,1])
    for i in range(n_rows):
        key = 'Group' + str(i+1)
        for j in range(len(files)):
            factor = relative_dict[key]
            ax.bar(
                index[i] + j * scen_width,
                (files[keys[j]]['p50'][regionid][key][-1, compart] -
                 files[keys[j]]['p50'][regionid][key][0, compart]) / factor,
                color=color[i],
                width=bar_width, edgecolor='black')
            if show_perc and not (j == 0 or j == 4):
                ax.bar(
                    index[i] + j * scen_width,
                    (files[keys[j]]['p75'][regionid][key][-1, compart] -
                     files[keys[j]]['p75'][regionid][key][0, compart]) /
                    factor, color=color[i],
                    width=bar_width, edgecolor='black', alpha=0.6)

        cell_text.append(
            ['%1.1f' %
             ((files[keys[x]]['p50'][regionid][key][-1, compart] -
               files[keys[x]]['p50'][regionid][key][0, compart]) / factor)
             for x in range(len(files))])

    if len(files) == 8:
        ax.set_xlim(-0.2, 7.8)
    else:
        ax.set_xlim(-0.5, 7.5)
    '''_, top = ax.get_ylim()
    if top > lim:
        ax.set_ylim(top=lim)'''

    # Add a table at the bottom of the axes
    the_table = plt.table(cellText=cell_text,
                          rowLabels=rows,
                          rowColours=color,
                          colLabels=columns,
                          fontsize=fontsize+10,
                          loc='bottom',
                          cellLoc='center')
    the_table.auto_set_font_size(False)
    the_table.scale(1, 2)
    the_table.set_fontsize(fontsize-2)
    # Adjust layout to make room for the table:
    # plt.subplots_adjust(left=0.2, bottom=0.8)

    if rel:
        ax.set_ylabel('age distributed infections [%]', fontsize=fontsize)
        plt.title(
            'Age distributed ratios of infected on September 3',
            fontsize=fontsize + 10)
    else:
        ax.set_ylabel('individuals relative per 100.000', fontsize=fontsize)
        plt.title('Cumulative number of infections per 100.000',
                  fontsize=fontsize + 10)
    # plt.yticks(values * value_increment, ['%d' % val for val in values])
    plt.xticks([])
    plt.yticks(fontsize=fontsize)
    plt.tight_layout()
    plt.savefig(name + '.png')


# Combines all compartments of a type into one (e.g. H_s, H_pv, H_v  ->  H)


def concat_comparts(files, comparts, scenario_list, plot_RKI, regionid='0'):
    new_files = {}
    for scenario in scenario_list:
        new_files[scenario] = {}
        percentile_list = ['p50', 'p25', 'p75', 'p05', 'p95']
        if plot_RKI:
            if 'RKI' in files[scenario].keys():
                percentile_list += ['RKI']
            else:
                print(
                    'Error in concat_comparts(). Extrapolated real data demanded but not read in.')
        for p in percentile_list:
            new_files[scenario][p] = {'0': {}}
            for key in [
                    'Group' + str(group + 1) for group in range(6)] + ['Total']:
                new_files[scenario][p][regionid][key] = np.zeros(
                    (len(files[scenario][p][regionid]['Time']), len(comparts)))
                for new_comp in range(len(comparts)):
                    for old_comp in comparts[new_comp]:
                        new_files[scenario][p][regionid][key][:, new_comp] += \
                            files[scenario][p][regionid][key][:, old_comp]

    return new_files
