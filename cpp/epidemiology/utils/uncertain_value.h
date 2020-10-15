#ifndef UNCERTAINVALUE_H
#define UNCERTAINVALUE_H

#include "epidemiology/utils/memory.h"
#include "epidemiology/utils/parameter_distributions.h"
#include "epidemiology/utils/ScalarType.h"

#include <memory>
#include <ostream>

namespace epi
{

/**
 * @brief The UncertainValue class consists of a 
 *        scalar value and a Distribution object
 * 
 * The UncertainValue class represents a model parameter that
 * can take a scalar value but that is subjected to a uncertainty.
 * The uncertainty is represented by a distribution object of kind
 * ParameterDistribution and the current scalar value can be 
 * replaced by drawing a new sample from the the distribution
 */
class UncertainValue
{
public:
    UncertainValue(ScalarType v = 0.)
        : m_value(v)
    {
    }

    UncertainValue(UncertainValue&& other) = default;

    /**
    * @brief Create an UncertainValue by cloning scalar value 
    *        and distribution of another UncertainValue
    */
    UncertainValue(const UncertainValue& other)
        : m_value(other.m_value)
    {
        if (other.m_dist) {
            m_dist.reset(other.m_dist->clone());
        }
    }

    /**
    * @brief Set an UncertainValue from another UncertainValue
    *        containing a scalar and a distribution
    */
    UncertainValue& operator=(const UncertainValue& other)
    {
        UncertainValue tmp(other);
        m_value = tmp.m_value;
        std::swap(m_dist, tmp.m_dist);
        return *this;
    }

    /**
     * @brief Conversion to scalar by returning the scalar contained in UncertainValue
     */
    operator ScalarType() const
    {
        return m_value;
    }

    ScalarType value() const
    {
        return m_value;
    }

    /**
     * @brief Conversion to scalar reference by returning the scalar contained in UncertainValue
     */
    operator ScalarType&()
    {
        return m_value;
    }

    /**
     * @brief Set an UncertainValue from a scalar, distribution remains unchanged.
     */
    UncertainValue& operator=(ScalarType v)
    {
        m_value = v;
        return *this;
    }

    /**
     * @brief Sets the distribution of the value.
     *
     * The function uses copy semantics, i.e. it copies
     * the distribution object.
     */
    void set_distribution(const ParameterDistribution& dist);

    /**
     * @brief Returns the parameter distribution.
     *
     * If it is not set, a nullptr is returned.
     */
    observer_ptr<const ParameterDistribution> get_distribution() const;

    /**
     * @brief Returns the parameter distribution.
     *
     * If it is not set, a nullptr is returned.
     */
    observer_ptr<ParameterDistribution> get_distribution();

    /**
     * @brief Sets the value by sampling from the distribution
     *        and returns the new value
     *
     * If no distribution is set, the value is not changed.
     */
    ScalarType draw_sample();

private:
    ScalarType m_value;
    std::unique_ptr<ParameterDistribution> m_dist;
};

//gtest printer
//TODO: should be extended when UncertainValue gets operator== that compares distributions as well
inline void PrintTo(const UncertainValue& uv, std::ostream* os)
{
    (*os) << uv.value();
}

} // namespace epi

#endif // UNCERTAINVALUE_H
