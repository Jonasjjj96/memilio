#ifndef POPULATIONS_H
#define POPULATIONS_H

#include <epidemiology/utils/uncertain_value.h>
#include "epidemiology/utils/custom_index_array.h"
#include "epidemiology/utils/ScalarType.h"
#include "epidemiology/utils/eigen.h"

#include <vector>
#include <array>
#include <numeric>

namespace epi
{

/**
 * @brief A class template for compartment populations
 *
 * Populations can be split up into different Categories, e.g. by
 * age group, yearly income group, gender etc. Compartmental models
 * introduce the additional category of infection type. For the SEIR
 * model these are Susceptible, Exposed, Infected and Removed. Every category
 * is assumed to contain a finite number of groups.
 *
 * This template can be instantiated given an arbitrary set of Categories.
 * The Categories are assumed to be an enum class with a member Count refering to
 * the number of elements in the enum.
 *
 * The class created from this template contains a "flat array" of compartment
 * populations and some functions for retrieving or setting the populations.
 *
 */

template <class... Categories>
class Populations : public CustomIndexArray<UncertainValue, Categories...>
{
public:

    using MultiIndex = typename CustomIndexArray<UncertainValue, Categories...>::MultiIndex;

    template <class... Ts,
              typename std::enable_if_t<std::is_constructible<UncertainValue, Ts...>::value>* = nullptr>
    explicit Populations(MultiIndex const& sizes, Ts... args)
        : CustomIndexArray<UncertainValue, Categories...>(sizes, args...)
    {}


    /**
     * @brief get_num_compartments returns the number of compartments
     *
     * This corresponds to the product of the category sizes
     *
     * @return number of compartments
     */
    size_t get_num_compartments() const
    {
        return this->numel();
    }

    /**
     * @brief get_compartments returns an Eigen copy of the vector of populations. This can be used
     * as initial conditions for the ODE solver
     * @return Eigen::VectorXd  of populations
     */
    inline Eigen::VectorXd get_compartments() const
    {
        return this->array().template cast<double>();
    }

    /**
     * @brief get_from returns the value of a flat container
     * at the flat index corresponding to set of enum values.
     * It is the same as get, except that it takes the values
     * from an outside reference flat container, rather than the
     * initial values stored within this class
     *
     * TODO: It would be better, to have CustomIndexArray be able to
     * operate on shared data. Maybe using an Eigen::Map
     *
     * @param y a reference to a flat container
     * @param cats enum values for each category
     * @return the population of compartment
     */
    template <class Arr>
    decltype(auto) get_from(Arr&& y, MultiIndex const& cats) const
    {
        static_assert(std::is_lvalue_reference<Arr>::value, "get_from is disabled for temporary arrays.");
        return y[this->get_flat_index(cats)];
    }

    /**
     * @brief get_group_total returns the total population of a group in one category
     * @param T enum class for the category
     * @param group_idx enum value of the group
     * @return total population of the group
     */
    template <class T>
    ScalarType get_group_total(Index<T> group_idx) const
    {
        auto const s = this->template slice<T>({(size_t)group_idx, 1});
        return std::accumulate(s.begin(), s.end(), 0.);
    }


    /**
     * @brief set_group_total sets the total population for a given group
     *
     * This function rescales all the compartments populations proportionally. If all compartments
     * have zero population, the total population gets distributed equally over all
     * compartments
     *
     * @param T The enum class of the category of the group
     * @param group_idx The enum value of the group within the category
     * @param value the new value for the total population
     */
    template <class T>
    void set_group_total(Index<T> group_idx, ScalarType value)
    {
        ScalarType current_population = get_group_total(group_idx);
        auto s = this->template slice<T>({(size_t)group_idx, 1});

        if (fabs(current_population) < 1e-12) {
            for (auto& v : s) {
                v = value / s.numel();
            }
        }
        else {
            for (auto& v : s) {
                v *= value / current_population;
            }
        }
    }


    /**
     * @brief get_total returns the total population of all compartments
     * @return total population
     */
    ScalarType get_total() const
    {
        return this->array().sum();
    }


    /**
     * @brief set_difference_from_group_total sets the total population for a given group from a difference
     *
     * This function sets the population size 
     *
     * @param total_population the new value for the total population
     * @param indices The indices of the compartment
     * @param T The enum class of the category of the group
     * @param group_idx The enum of the group within the category
     */
    template <class T>
    void set_difference_from_group_total(MultiIndex const& midx, ScalarType total_group_population)

    {
        auto group_idx = midx.template get<T>();
        ScalarType current_population = get_group_total(group_idx);
        size_t idx                    = this->get_flat_index(midx);
        current_population -= this->array()[idx];

        assert(current_population <= total_group_population);

        this->array()[idx] = total_group_population - current_population;
    }

    /**
     * @brief set_total sets the total population
     *
     * This function rescales all the compartments populations proportionally. If all compartments
     * have zero population, the total population gets distributed equally over all
     * compartments. 
     *
     * @param value the new value for the total population
     */
    void set_total(ScalarType value)
    {
        double current_population = get_total();
        if (fabs(current_population) < 1e-12) {
            double ysize = double(this->array().size());
            for (size_t i = 0; i < this->get_num_compartments(); i++) {
                this->array()[(Eigen::Index)i] = value / ysize;
            }
        }
        else {
            for (size_t i = 0; i < this->get_num_compartments(); i++) {
                this->array()[(Eigen::Index)i] *= value / current_population;
            }
        }
    }

    /**
     * @brief set_difference_from_total takes a total population as input and sets the compartment
     * of a given index to have the difference between the input total population and the current
     * population in all other compartments
     * @param indices the index of the compartment
     * @param total_population the new value for the total population
     */
    void set_difference_from_total(MultiIndex midx, double total_population)
    {
        double current_population = get_total();
        size_t idx                = this->get_flat_index(midx);
        current_population -= this->array()[idx];

        assert(current_population <= total_population);

        this->array()[idx] = total_population - current_population;
    }

    /**
     * @brief checks whether the population Parameters satisfy their corresponding constraints and applys them
     */
    void apply_constraints()
    {
        for (int i = 0; i < this->array().size(); i++) {
            if (this->array()[i] < 0) {
                log_warning("Constraint check: Compartment size {:d} changed from {:.4f} to {:d}", i, this->array()[i], 0);
                this->array()[i] = 0;
            }
        }
    }

    /**
     * @brief checks whether the population Parameters satisfy their corresponding constraints
     */
    void check_constraints() const
    {
        for (int i = 0; i < this->array().size(); i++) {
            if (this->array()[i] < 0) {
                log_error("Constraint check: Compartment size {:d} is {:.4f} and smaller {:d}", i, this->array()[i], 0);
            }
        }
    }


};


} // namespace epi

#endif // POPULATIONS_H
