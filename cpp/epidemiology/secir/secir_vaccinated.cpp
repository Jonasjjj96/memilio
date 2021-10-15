/* 
* Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
*
* Authors: Daniel Abele, Jan Kleinert, Martin J. Kuehn
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
#include "epidemiology/secir/secir.h"
#include "epidemiology/math/euler.h"
#include "epidemiology/math/adapt_rk.h"
#include "epidemiology/math/smoother.h"
#include "epidemiology/utils/eigen_util.h"
#include "epidemiology_io/secir_result_io.h"

#include <cmath>
#include <cassert>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace epi
{
void SecirParams::set_start_day(double tstart)
{
    m_tstart = tstart;
}

double SecirParams::get_start_day() const
{
    return m_tstart;
}

void SecirParams::set_seasonality(UncertainValue const& seasonality)
{
    m_seasonality = seasonality;
}

void SecirParams::set_seasonality(double seasonality)
{
    m_seasonality = seasonality;
}

void SecirParams::set_seasonality(ParameterDistribution const& seasonality)
{
    m_seasonality.set_distribution(seasonality);
}

const UncertainValue& SecirParams::get_seasonality() const
{
    return m_seasonality;
}

UncertainValue& SecirParams::get_seasonality()
{
    return m_seasonality;
}

void SecirParams::set_icu_capacity(UncertainValue const& icu_capacity)
{
    m_icu_capacity = icu_capacity;
}

void SecirParams::set_icu_capacity(double icu_capacity)
{
    m_icu_capacity = icu_capacity;
}

void SecirParams::set_icu_capacity(ParameterDistribution const& icu_capacity)
{
    m_icu_capacity.set_distribution(icu_capacity);
}

const UncertainValue& SecirParams::get_icu_capacity() const
{
    return m_icu_capacity;
}

UncertainValue& SecirParams::get_icu_capacity()
{
    return m_icu_capacity;
}

SecirParams::StageTimes::StageTimes()
    : m_tinc{1.0}
    , m_tinfmild{1.0}
    , m_tserint{1.0}
    , m_thosp2home{1.0}
    , m_thome2hosp{1.0}
    , m_thosp2icu{1.0}
    , m_ticu2home{1.0}
    , m_tinfasy{1.0}
    , m_ticu2death{1.0}
{
}

void SecirParams::StageTimes::set_incubation(UncertainValue const& tinc)
{
    m_tinc = tinc;
}

void SecirParams::StageTimes::set_incubation(double tinc)
{
    m_tinc = tinc;
}

void SecirParams::StageTimes::set_incubation(ParameterDistribution const& tinc)
{
    m_tinc.set_distribution(tinc);
}

void SecirParams::StageTimes::set_infectious_mild(UncertainValue const& tinfmild)
{
    m_tinfmild = tinfmild;
}

void SecirParams::StageTimes::set_infectious_mild(double tinfmild)
{
    m_tinfmild = tinfmild;
}

void SecirParams::StageTimes::set_infectious_mild(ParameterDistribution const& tinfmild)
{
    m_tinfmild.set_distribution(tinfmild);
}

void SecirParams::StageTimes::set_serialinterval(UncertainValue const& tserint)
{
    m_tserint = tserint;
}

void SecirParams::StageTimes::set_serialinterval(double tserint)
{
    m_tserint = tserint;
}

void SecirParams::StageTimes::set_serialinterval(ParameterDistribution const& tserint)
{
    m_tserint.set_distribution(tserint);
}

void SecirParams::StageTimes::set_hospitalized_to_home(UncertainValue const& thosp2home)
{
    m_thosp2home = thosp2home;
}

void SecirParams::StageTimes::set_hospitalized_to_home(double thosp2home)
{
    m_thosp2home = thosp2home;
}

void SecirParams::StageTimes::set_hospitalized_to_home(ParameterDistribution const& thosp2home)
{
    m_thosp2home.set_distribution(thosp2home);
}

void SecirParams::StageTimes::set_home_to_hospitalized(UncertainValue const& thome2hosp)
{
    m_thome2hosp = thome2hosp;
}

void SecirParams::StageTimes::set_home_to_hospitalized(double thome2hosp)
{
    m_thome2hosp = thome2hosp;
}

void SecirParams::StageTimes::set_home_to_hospitalized(ParameterDistribution const& thome2hosp)
{
    m_thome2hosp.set_distribution(thome2hosp);
}

void SecirParams::StageTimes::set_hospitalized_to_icu(UncertainValue const& thosp2icu)
{
    m_thosp2icu = thosp2icu;
}

void SecirParams::StageTimes::set_hospitalized_to_icu(double thosp2icu)
{
    m_thosp2icu = thosp2icu;
}

void SecirParams::StageTimes::set_hospitalized_to_icu(ParameterDistribution const& thosp2icu)
{
    m_thosp2icu.set_distribution(thosp2icu);
}

void SecirParams::StageTimes::set_icu_to_home(UncertainValue const& ticu2home)
{
    m_ticu2home = ticu2home;
}

void SecirParams::StageTimes::set_icu_to_home(double ticu2home)
{
    m_ticu2home = ticu2home;
}

void SecirParams::StageTimes::set_icu_to_home(ParameterDistribution const& ticu2home)
{
    m_ticu2home.set_distribution(ticu2home);
}

void SecirParams::StageTimes::set_infectious_asymp(UncertainValue const& tinfasy)
{
    log_warning("The parameter of the asymptomatic infectious period is meant to be defined by the other parameters of "
                "the system. Do you really want to set it?");
    m_tinfasy = tinfasy;
}

void SecirParams::StageTimes::set_infectious_asymp(double tinfasy)
{
    log_warning("The parameter of the asymptomatic infectious period is meant to be defined by the other parameters of "
                "the system. Do you really want to set it?");
    m_tinfasy = tinfasy;
}

void SecirParams::StageTimes::set_infectious_asymp(ParameterDistribution const& tinfasy)
{
    m_tinfasy.set_distribution(tinfasy);
}

void SecirParams::StageTimes::set_icu_to_death(UncertainValue const& ticu2death)
{
    m_ticu2death = ticu2death;
}

void SecirParams::StageTimes::set_icu_to_death(double ticu2death)
{
    m_ticu2death = ticu2death;
}

void SecirParams::StageTimes::set_icu_to_death(ParameterDistribution const& ticu2death)
{
    m_ticu2death.set_distribution(ticu2death);
}

const UncertainValue& SecirParams::StageTimes::get_incubation() const
{
    return m_tinc;
}

UncertainValue& SecirParams::StageTimes::get_incubation()
{
    return m_tinc;
}

const UncertainValue& SecirParams::StageTimes::get_infectious_mild() const
{
    return m_tinfmild;
}

UncertainValue& SecirParams::StageTimes::get_infectious_mild()
{
    return m_tinfmild;
}

const UncertainValue& SecirParams::StageTimes::get_serialinterval() const
{
    return m_tserint;
}

UncertainValue& SecirParams::StageTimes::get_serialinterval()
{
    return m_tserint;
}

const UncertainValue& SecirParams::StageTimes::get_hospitalized_to_home() const
{
    return m_thosp2home;
}

UncertainValue& SecirParams::StageTimes::get_hospitalized_to_home()
{
    return m_thosp2home;
}

const UncertainValue& SecirParams::StageTimes::get_home_to_hospitalized() const
{
    return m_thome2hosp;
}

UncertainValue& SecirParams::StageTimes::get_home_to_hospitalized()
{
    return m_thome2hosp;
}

const UncertainValue& SecirParams::StageTimes::get_hospitalized_to_icu() const
{
    return m_thosp2icu;
}

UncertainValue& SecirParams::StageTimes::get_hospitalized_to_icu()
{
    return m_thosp2icu;
}

const UncertainValue& SecirParams::StageTimes::get_icu_to_home() const
{
    return m_ticu2home;
}

UncertainValue& SecirParams::StageTimes::get_icu_to_home()
{
    return m_ticu2home;
}

const UncertainValue& SecirParams::StageTimes::get_infectious_asymp() const
{
    return m_tinfasy;
}

UncertainValue& SecirParams::StageTimes::get_infectious_asymp()
{
    return m_tinfasy;
}

const UncertainValue& SecirParams::StageTimes::get_icu_to_dead() const
{
    return m_ticu2death;
}

UncertainValue& SecirParams::StageTimes::get_icu_to_dead()
{
    return m_ticu2death;
}

void SecirParams::StageTimes::apply_constraints()
{

    if (m_tinc < 2.0) {
        log_warning("Constraint check: Parameter m_tinc changed from {:.4f} to {:.4f}", m_tinc, 2.0);
        m_tinc = 2.0;
    }

    if (2 * m_tserint < m_tinc + 1.0) {
        log_warning("Constraint check: Parameter m_tserint changed from {:.4f} to {:.4f}", m_tserint,
                    0.5 * m_tinc + 0.5);
        m_tserint = 0.5 * m_tinc + 0.5;
    }
    else if (m_tserint > m_tinc - 0.5) {
        log_warning("Constraint check: Parameter m_tserint changed from {:.4f} to {:.4f}", m_tserint, m_tinc - 0.5);
        m_tserint = m_tinc - 0.5;
    }

    if (m_tinfmild < 1.0) {
        log_warning("Constraint check: Parameter m_tinfmild changed from {:.4f} to {:.4f}", m_tinfmild, 1.0);
        m_tinfmild = 1.0;
    }

    if (m_thosp2home < 1.0) {
        log_warning("Constraint check: Parameter m_thosp2home changed from {:.4f} to {:.4f}", m_thosp2home, 1.0);
        m_thosp2home = 1.0;
    }

    if (m_thome2hosp < 1.0) {
        log_warning("Constraint check: Parameter m_thome2hosp changed from {:.4f} to {:.4f}", m_thome2hosp, 1.0);
        m_thome2hosp = 1.0;
    }

    if (m_thosp2icu < 1.0) {
        log_warning("Constraint check: Parameter m_thosp2icu changed from {:.4f} to {:.4f}", m_thosp2icu, 1.0);
        m_thosp2icu = 1.0;
    }

    if (m_ticu2home < 1.0) {
        log_warning("Constraint check: Parameter m_ticu2home changed from {:.4f} to {:.4f}", m_ticu2home, 1.0);
        m_ticu2home = 1.0;
    }

    if (m_tinfasy != 1.0 / (0.5 / (m_tinc - m_tserint)) + 0.5 * m_tinfmild) {
        log_info("Constraint check: Parameter m_tinfasy set as fully dependent on tinc, tserint and tinfmild. See HZI "
                 "paper.");
        m_tinfasy = 1.0 / (0.5 / (m_tinc - m_tserint)) + 0.5 * m_tinfmild;
    }

    if (m_ticu2death < 1.0) {
        log_warning("Constraint check: Parameter m_ticu2death changed from {:.4f} to {:.4f}", m_ticu2death, 1.0);
        m_ticu2death = 1.0;
    }
}

void SecirParams::StageTimes::check_constraints() const
{

    if (m_tinc < 2.0) {
        log_error("Constraint check: Parameter m_tinc {:.4f} smaller {:.4f}", m_tinc, 2.0);
    }

    if (2 * m_tserint < m_tinc + 1.0) {
        log_error("Constraint check: Parameter m_tserint {:.4f} smaller {:.4f}", m_tserint, 0.5 * m_tinc + 0.5);
    }
    else if (m_tserint > m_tinc - 0.5) {
        log_error("Constraint check: Parameter m_tserint {:.4f} smaller {:.4f}", m_tserint, m_tinc - 0.5);
    }

    if (m_tinfmild < 1.0) {
        log_error("Constraint check: Parameter m_tinfmild {:.4f} smaller {:.4f}", m_tinfmild, 1.0);
    }

    if (m_thosp2home < 1.0) {
        log_error("Constraint check: Parameter m_thosp2home {:.4f} smaller {:.4f}", m_thosp2home, 1.0);
    }

    if (m_thome2hosp < 1.0) {
        log_error("Constraint check: Parameter m_thome2hosp {:.4f} smaller {:.4f}", m_thome2hosp, 1.0);
    }

    if (m_thosp2icu < 1.0) {
        log_error("Constraint check: Parameter m_thosp2icu {:.4f} smaller {:.4f}", m_thosp2icu, 1.0);
    }

    if (m_ticu2home < 1.0) {
        log_error("Constraint check: Parameter m_ticu2home {:.4f} smaller {:.4f}", m_ticu2home, 1.0);
    }

    if (m_tinfasy != 1.0 / (0.5 / (m_tinc - m_tserint)) + 0.5 * m_tinfmild) {
        log_error("Constraint check: Parameter m_tinfasy not set as fully dependent on tinc, tserint and tinfmild. See "
                  "HZI paper.");
    }

    if (m_ticu2death < 1.0) {
        log_error("Constraint check: Parameter m_ticu2death {:.4f} smaller {:.4f}", m_ticu2death, 1.0);
    }
}

SecirParams::Probabilities::Probabilities()
    : m_infprob{1}
    , m_carrinf{1}
    , m_asympinf{0}
    , m_risksymp{0}
    , m_hospinf{0}
    , m_icuhosp{0}
    , m_deathicu{0}
    , m_tnt_max_risksymp{0}
{
}

void SecirParams::Probabilities::set_infection_from_contact(UncertainValue const& infprob)
{
    m_infprob = infprob;
}

void SecirParams::Probabilities::set_infection_from_contact(double infprob)
{
    m_infprob = infprob;
}

void SecirParams::Probabilities::set_infection_from_contact(ParameterDistribution const& infprob)
{
    m_infprob.set_distribution(infprob);
}

void SecirParams::Probabilities::set_carrier_infectability(UncertainValue const& carrinf)
{
    m_carrinf = carrinf;
}

void SecirParams::Probabilities::set_carrier_infectability(double carrinf)
{
    m_carrinf = carrinf;
}

void SecirParams::Probabilities::set_carrier_infectability(ParameterDistribution const& carrinf)
{
    m_carrinf.set_distribution(carrinf);
}

void SecirParams::Probabilities::set_asymp_per_infectious(UncertainValue const& asympinf)
{
    m_asympinf = asympinf;
}

void SecirParams::Probabilities::set_asymp_per_infectious(double asympinf)
{
    m_asympinf = asympinf;
}

void SecirParams::Probabilities::set_asymp_per_infectious(ParameterDistribution const& asympinf)
{
    m_asympinf.set_distribution(asympinf);
}

void SecirParams::Probabilities::set_risk_from_symptomatic(UncertainValue const& risksymp)
{
    m_risksymp = risksymp;
}

void SecirParams::Probabilities::set_risk_from_symptomatic(double risksymp)
{
    m_risksymp = risksymp;
}

void SecirParams::Probabilities::set_risk_from_symptomatic(ParameterDistribution const& risksymp)
{
    m_risksymp.set_distribution(risksymp);
}

void SecirParams::Probabilities::set_test_and_trace_max_risk_from_symptomatic(UncertainValue const& value)
{
    m_tnt_max_risksymp = value;
}

void SecirParams::Probabilities::set_test_and_trace_max_risk_from_symptomatic(double value)
{
    m_tnt_max_risksymp = value;
}

void SecirParams::Probabilities::set_test_and_trace_max_risk_from_symptomatic(ParameterDistribution const& distribution)
{
    m_tnt_max_risksymp.set_distribution(distribution);
}

void SecirParams::Probabilities::set_hospitalized_per_infectious(UncertainValue const& hospinf)
{
    m_hospinf = hospinf;
}

void SecirParams::Probabilities::set_hospitalized_per_infectious(double hospinf)
{
    m_hospinf = hospinf;
}

void SecirParams::Probabilities::set_hospitalized_per_infectious(ParameterDistribution const& hospinf)
{
    m_hospinf.set_distribution(hospinf);
}

void SecirParams::Probabilities::set_icu_per_hospitalized(UncertainValue const& icuhosp)
{
    m_icuhosp = icuhosp;
}

void SecirParams::Probabilities::set_icu_per_hospitalized(double icuhosp)
{
    m_icuhosp = icuhosp;
}

void SecirParams::Probabilities::set_icu_per_hospitalized(ParameterDistribution const& icuhosp)
{
    m_icuhosp.set_distribution(icuhosp);
}

void SecirParams::Probabilities::set_dead_per_icu(UncertainValue const& deathicu)
{
    m_deathicu = deathicu;
}

void SecirParams::Probabilities::set_dead_per_icu(double deathicu)
{
    m_deathicu = deathicu;
}

void SecirParams::Probabilities::set_dead_per_icu(ParameterDistribution const& deathicu)
{
    m_deathicu.set_distribution(deathicu);
}

const UncertainValue& SecirParams::Probabilities::get_infection_from_contact() const
{
    return m_infprob;
}

UncertainValue& SecirParams::Probabilities::get_infection_from_contact()
{
    return m_infprob;
}

const UncertainValue& SecirParams::Probabilities::get_carrier_infectability() const
{
    return m_carrinf;
}

UncertainValue& SecirParams::Probabilities::get_carrier_infectability()
{
    return m_carrinf;
}

const UncertainValue& SecirParams::Probabilities::get_asymp_per_infectious() const
{
    return m_asympinf;
}

UncertainValue& SecirParams::Probabilities::get_asymp_per_infectious()
{
    return m_asympinf;
}

const UncertainValue& SecirParams::Probabilities::get_risk_from_symptomatic() const
{
    return m_risksymp;
}

UncertainValue& SecirParams::Probabilities::get_risk_from_symptomatic()
{
    return m_risksymp;
}

const UncertainValue& SecirParams::Probabilities::get_test_and_trace_max_risk_from_symptomatic() const
{
    return m_tnt_max_risksymp;
}

UncertainValue& SecirParams::Probabilities::get_test_and_trace_max_risk_from_symptomatic()
{
    return m_tnt_max_risksymp;
}

const UncertainValue& SecirParams::Probabilities::get_hospitalized_per_infectious() const
{
    return m_hospinf;
}

UncertainValue& SecirParams::Probabilities::get_hospitalized_per_infectious()
{
    return m_hospinf;
}

const UncertainValue& SecirParams::Probabilities::get_icu_per_hospitalized() const
{
    return m_icuhosp;
}

UncertainValue& SecirParams::Probabilities::get_icu_per_hospitalized()
{
    return m_icuhosp;
}

const UncertainValue& SecirParams::Probabilities::get_dead_per_icu() const
{
    return m_deathicu;
}

UncertainValue& SecirParams::Probabilities::get_dead_per_icu()
{
    return m_deathicu;
}

void SecirParams::Probabilities::apply_constraints()
{
    if (m_infprob < 0.0) {
        log_warning("Constraint check: Parameter m_asympinf changed from {:0.4f} to {:d} ", m_infprob, 0);
        m_infprob = 0;
    }

    if (m_carrinf < 0.0) {
        log_warning("Constraint check: Parameter m_asympinf changed from {:0.4f} to {:d} ", m_carrinf, 0);
        m_carrinf = 0;
    }

    if (m_asympinf < 0.0 || m_asympinf > 1.0) {
        log_warning("Constraint check: Parameter m_asympinf changed from {:0.4f} to {:d} ", m_asympinf, 0);
        m_asympinf = 0;
    }

    if (m_risksymp < 0.0 || m_risksymp > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp changed from {:0.4f} to {:d}", m_risksymp, 0);
        m_risksymp = 0;
    }

    if (m_hospinf < 0.0 || m_hospinf > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp changed from {:0.4f} to {:d}", m_hospinf, 0);
        m_hospinf = 0;
    }

    if (m_icuhosp < 0.0 || m_icuhosp > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp changed from {:0.4f} to {:d}", m_icuhosp, 0);
        m_icuhosp = 0;
    }

    if (m_deathicu < 0.0 || m_deathicu > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp changed from {:0.4f} to {:d}", m_deathicu, 0);
        m_deathicu = 0;
    }
}

void SecirParams::Probabilities::check_constraints() const
{
    if (m_infprob < 0.0) {
        log_warning("Constraint check: Parameter m_infprob smaller {:d}", 0);
    }

    if (m_carrinf < 0.0) {
        log_warning("Constraint check: Parameter m_carrinf smaller {:d}", 0);
    }

    if (m_asympinf < 0.0 || m_asympinf > 1.0) {
        log_warning("Constraint check: Parameter m_asympinf smaller {:d} or larger {:d}", 0, 1);
    }

    if (m_risksymp < 0.0 || m_risksymp > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp smaller {:d} or larger {:d}", 0, 1);
    }

    if (m_hospinf < 0.0 || m_hospinf > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp smaller {:d} or larger {:d}", 0, 1);
    }

    if (m_icuhosp < 0.0 || m_icuhosp > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp smaller {:d} or larger {:d}", 0, 1);
    }

    if (m_deathicu < 0.0 || m_deathicu > 1.0) {
        log_warning("Constraint check: Parameter m_risksymp smaller {:d} or larger {:d}", 0, 1);
    }
}

void SecirParams::set_contact_patterns(UncertainContactMatrix contact_patterns)
{
    m_contact_patterns = contact_patterns;
}

UncertainContactMatrix& SecirParams::get_contact_patterns()
{
    return m_contact_patterns;
}

UncertainContactMatrix const& SecirParams::get_contact_patterns() const
{
    return m_contact_patterns;
}

UncertainValue& SecirParams::get_test_and_trace_capacity()
{
    return m_test_and_trace_capacity;
}

const UncertainValue& SecirParams::get_test_and_trace_capacity() const
{
    return m_test_and_trace_capacity;
}

void SecirParams::set_test_and_trace_capacity(double value)
{
    m_test_and_trace_capacity = value;
}

void SecirParams::set_test_and_trace_capacity(const UncertainValue& value)
{
    m_test_and_trace_capacity = value;
}

void SecirParams::set_test_and_trace_capacity(const ParameterDistribution& distribution)
{
    m_test_and_trace_capacity.set_distribution(distribution);
}

void SecirParams::apply_constraints()
{

    if (m_seasonality < 0.0 || m_seasonality > 0.5) {
        log_warning("Constraint check: Parameter m_seasonality changed from {:0.4f} to {:d}", m_seasonality, 0);
        m_seasonality = 0;
    }

    if (m_icu_capacity < 0.0) {
        log_warning("Constraint check: Parameter m_icu_capacity changed from {:0.4f} to {:d}", m_icu_capacity, 0);
        m_icu_capacity = 0;
    }

    for (size_t i = 0; i < times.size(); i++) {
        populations.apply_constraints();
        times[i].apply_constraints();
        probabilities[i].apply_constraints();
    }
}

void SecirParams::check_constraints() const
{
    if (m_seasonality < 0.0 || m_seasonality > 0.5) {
        log_warning("Constraint check: Parameter m_seasonality smaller {:d} or larger {:d}", 0, 0.5);
    }

    if (m_icu_capacity < 0.0) {
        log_warning("Constraint check: Parameter m_icu_capacity smaller {:d}", 0);
    }

    for (size_t i = 0; i < times.size(); i++) {
        populations.check_constraints();
        times[i].check_constraints();
        probabilities[i].check_constraints();
    }
}

void secir_get_derivatives(SecirParams const& params, Eigen::Ref<const Eigen::VectorXd> pop,
                           Eigen::Ref<const Eigen::VectorXd> y, double t, Eigen::Ref<Eigen::VectorXd> dydt)
{
    // alpha  // percentage of asymptomatic cases
    // beta // risk of infection from the infected symptomatic patients
    // rho   // hospitalized per infectious
    // theta // icu per hospitalized
    // delta  // deaths per ICUs
    // 0: S,      1: E,     2: C,     3: I,     4: H,     5: U,     6: R,     7: D
    size_t n_agegroups = params.get_num_groups();

    ContactMatrixGroup const& contact_matrix = params.get_contact_patterns();
    
    auto test_and_trace_required = 0;
    for (size_t i = 0; i < n_agegroups; i++) {
        size_t Ci = params.populations.get_flat_index({i, C});
        double R3 =
            0.5 / (params.times[i].get_incubation() - params.times[i].get_serialinterval());
        test_and_trace_required += (1 - params.probabilities[i].get_asymp_per_infectious()) * R3 * pop[Ci];
    }

    for (size_t i = 0; i < n_agegroups; i++) {

        size_t Si = params.populations.get_flat_index({i, S});
        size_t Ei = params.populations.get_flat_index({i, E});
        size_t Ci = params.populations.get_flat_index({i, C});
        size_t Ii = params.populations.get_flat_index({i, I});
        size_t Hi = params.populations.get_flat_index({i, H});
        size_t Ui = params.populations.get_flat_index({i, U});
        size_t Ri = params.populations.get_flat_index({i, R});
        size_t Di = params.populations.get_flat_index({i, D});

        dydt[Si] = 0;
        dydt[Ei] = 0;

        //symptomatic are less well quarantined when testing and tracing is overwhelmed so they infect more people
        auto risk_from_symptomatic   = smoother_cosine(
            test_and_trace_required, params.get_test_and_trace_capacity(), params.get_test_and_trace_capacity() * 5,
            params.probabilities[i].get_risk_from_symptomatic(),
            params.probabilities[i].get_test_and_trace_max_risk_from_symptomatic());

        double icu_occupancy = 0;

        for (size_t j = 0; j < n_agegroups; j++) {
            size_t Sj = params.populations.get_flat_index({j, S});
            size_t Ej = params.populations.get_flat_index({j, E});
            size_t Cj = params.populations.get_flat_index({j, C});
            size_t Ij = params.populations.get_flat_index({j, I});
            size_t Hj = params.populations.get_flat_index({j, H});
            size_t Uj = params.populations.get_flat_index({j, U});
            size_t Rj = params.populations.get_flat_index({j, R});

            // effective contact rate by contact rate between groups i and j and damping j
            double season_val =
                (1 + params.get_seasonality() *
                         sin(3.141592653589793 * (std::fmod((params.get_start_day() + t), 365.0) / 182.5 + 0.5)));
            double cont_freq_eff = season_val * contact_matrix.get_matrix_at(t)(static_cast<Eigen::Index>(i),
                                                                                static_cast<Eigen::Index>(j));
            double Nj      = pop[Sj] + pop[Ej] + pop[Cj] + pop[Ij] + pop[Hj] + pop[Uj] + pop[Rj]; // without died people
            double divNj   = 1.0 / Nj; // precompute 1.0/Nj
            double dummy_S = y[Si] * cont_freq_eff * divNj * params.probabilities[i].get_infection_from_contact() *
                             (params.probabilities[j].get_carrier_infectability() * pop[Cj] +
                              risk_from_symptomatic * pop[Ij]);

            dydt[Si] -= dummy_S; // -R1*(C+beta*I)*S/N0
            dydt[Ei] += dummy_S; // R1*(C+beta*I)*S/N0-R2*E

            icu_occupancy += y[Uj];
        }

        double dummy_R2 =
            1.0 / (2 * params.times[i].get_serialinterval() - params.times[i].get_incubation()); // R2 = 1/(2SI-TINC)
        double dummy_R3 =
            0.5 / (params.times[i].get_incubation() - params.times[i].get_serialinterval()); // R3 = 1/(2(TINC-SI))
            
        // ICU capacity shortage is close
        double prob_hosp2icu =
            smoother_cosine(icu_occupancy, 0.90 * params.get_icu_capacity(), params.get_icu_capacity(),
                            params.probabilities[i].get_icu_per_hospitalized(), 0);

        double prob_hosp2dead = params.probabilities[i].get_icu_per_hospitalized() - prob_hosp2icu;

        dydt[Ei] -= dummy_R2 * y[Ei]; // only exchange of E and C done here
        dydt[Ci] = dummy_R2 * y[Ei] -
                   ((1 - params.probabilities[i].get_asymp_per_infectious()) * dummy_R3 +
                    params.probabilities[i].get_asymp_per_infectious() / params.times[i].get_infectious_asymp()) *
                       y[Ci];
        dydt[Ii] =
            (1 - params.probabilities[i].get_asymp_per_infectious()) * dummy_R3 * y[Ci] -
            ((1 - params.probabilities[i].get_hospitalized_per_infectious()) / params.times[i].get_infectious_mild() +
             params.probabilities[i].get_hospitalized_per_infectious() / params.times[i].get_home_to_hospitalized()) *
                y[Ii];
        dydt[Hi] =
            params.probabilities[i].get_hospitalized_per_infectious() / params.times[i].get_home_to_hospitalized() *
                y[Ii] -
            ((1 - params.probabilities[i].get_icu_per_hospitalized()) / params.times[i].get_hospitalized_to_home() +
             params.probabilities[i].get_icu_per_hospitalized() / params.times[i].get_hospitalized_to_icu()) *
                y[Hi];
        dydt[Ui] = -((1 - params.probabilities[i].get_dead_per_icu()) / params.times[i].get_icu_to_home() +
                     params.probabilities[i].get_dead_per_icu() / params.times[i].get_icu_to_dead()) *
                   y[Ui];
        // add flow from hosp to icu according to potentially adjusted probability due to ICU limits
        dydt[Ui] += prob_hosp2icu / params.times[i].get_hospitalized_to_icu() * y[Hi];

        dydt[Ri] = params.probabilities[i].get_asymp_per_infectious() / params.times[i].get_infectious_asymp() * y[Ci] +
                   (1 - params.probabilities[i].get_hospitalized_per_infectious()) /
                       params.times[i].get_infectious_mild() * y[Ii] +
                   (1 - params.probabilities[i].get_icu_per_hospitalized()) /
                       params.times[i].get_hospitalized_to_home() * y[Hi] +
                   (1 - params.probabilities[i].get_dead_per_icu()) / params.times[i].get_icu_to_home() * y[Ui];

        dydt[Di] = params.probabilities[i].get_dead_per_icu() / params.times[i].get_icu_to_dead() * y[Ui];
        // add potential, additional deaths due to ICU overflow
        dydt[Di] += prob_hosp2dead / params.times[i].get_hospitalized_to_icu() * y[Hi];
    }
}

TimeSeries<double> simulate(double t0, double tmax, double dt, SecirParams const& params)
{
    params.check_constraints();
    SecirSimulation sim(params, t0, dt);
    sim.advance(tmax);
    return sim.get_result();
}

SecirSimulation::SecirSimulation(SecirParams const& params, double t0, double dt)
    : m_integratorCore(std::make_shared<RKIntegratorCore>(1e-3, 1.))
    , m_integrator(
          [params](auto&& y, auto&& t, auto&& dydt) {
              secir_get_derivatives(params, y, t, dydt);
          },
          t0, params.populations.get_compartments(), dt, m_integratorCore)
    , m_params(params)
{
    m_integratorCore->set_rel_tolerance(1e-4);
    m_integratorCore->set_abs_tolerance(1e-1);
}

Eigen::Ref<Eigen::VectorXd> SecirSimulation::advance(double tmax)
{
    double threshhold = 0.8;
    size_t num_groups = m_params.get_num_groups();
    double t          = m_integrator.get_result().get_time(m_integrator.get_result().get_num_time_points() - 1);

    std::vector<bool> saturated(num_groups, false);
    while (t < tmax) {
        bool apply_vaccination = tmax - std::floor(t) >= 1;
        t                      = std::min(t + 1, tmax);

        auto last_value = m_integrator.advance(t);
        if (apply_vaccination) {

            for (size_t i = 0; i < num_groups; ++i) {
                double share_new_variant = std::min(1.0, pow(2, t / 7) / 100.0);
                double new_transmission  = (1 - share_new_variant) * m_params.get<BaseInfB117>()[i] +
                                          share_new_variant * m_params.get<BaseInfB161>()[i];
                m_params.get<InfectionProbabilityFromContact>()[i] = new_transmission;
            }
            double leftover = 0;
            size_t count    = 0;
            for (size_t i = 2; i < num_groups; ++i) {
                std::vector<double> daily_first_vaccinations = m_params.get_daily_first_vaccinations();
                std::vector<double> daily_first_vaccinations = m_params.get_daily_full_vaccinations();
                double new_vaccinated_full  = daily_first_vaccinations[daily_first_vaccinations.size() - 42];
                double new_vaccinated_first = m_params.get<VaccineGrowthFirst>()[i] +
                                              m_params.get<VaccineGrowthFull>()[i] - new_vaccinated_full;
                if (last_value(InfectionStateV::Count * i + InfectionStateV::Susceptible) - new_vaccinated_first >
                    (1 - threshhold) * m_params.populations.get_group_total(i)) {
                    last_value(InfectionStateV::Count * i + InfectionStateV::Susceptible) -= new_vaccinated_first;
                    last_value(InfectionStateV::Count * i + InfectionStateV::SusceptibleV1) += new_vaccinated_first;
                    m_params.add_daily_first_vaccinations(new_vaccinated_first);
                }
                else {
                    m_params.add_daily_first_vaccinations(0);
                    saturated[i] = true;
                    count++;
                    leftover += new_vaccinated_first;
                }

                if (last_value(InfectionStateV::Count * i + InfectionStateV::SusceptibleV1) - new_vaccinated_full > 0) {
                    last_value(InfectionStateV::Count * i + InfectionStateV::SusceptibleV1) -= new_vaccinated_full;
                    last_value(InfectionStateV::Count * i + InfectionStateV::Recovered) += new_vaccinated_full;
                }
                else {
                    leftover += new_vaccinated_full;
                }
            }
            for (size_t i = 2; i < num_groups; ++i) {
                if (!saturated[i] && count > 0) {
                    last_value(InfectionStateV::Count * i + InfectionStateV::Susceptible) -= leftover / count;
                    last_value(InfectionStateV::Count * i + InfectionStateV::SusceptibleV1) += leftover / count;
                    m_params.get<DailyFirstVaccination>()[i]
                        [m_params.get<DailyFirstVaccination>()[i].size() - 1] += leftover / count;
                }
            }
            //m_integrator.get_result().get_last_value() = last_value;
        }
    }

    return m_integrator.get_result().get_last_value();

}

} // namespace epi
