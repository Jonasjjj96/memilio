/*
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Elisabeth Kluth, David Kerkmann, Sascha Korf, Martin J. Kuehn, Khoa Nguyen
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
#ifndef MIO_ABM_TESTING_SCHEME_H
#define MIO_ABM_TESTING_SCHEME_H

#include "abm/config.h"
#include "abm/parameters.h"
#include "abm/person.h"
#include "abm/location.h"
#include "abm/time.h"
#include "memilio/config.h"
#include <bitset>

namespace mio
{
namespace abm
{

/**
 * @brief TestingCriteria for TestingScheme.
 */
class TestingCriteria
{
public:
    /**
     * @brief Create a TestingCriteria where everyone is tested.
     */
    TestingCriteria() = default;

    /**
     * @brief Create a TestingCriteria.
     * @param[in] ages Vector of AgeGroup%s that are either allowed or required to be tested.
     * @param[in] infection_states Vector of #InfectionState%s that are either allowed or required to be tested.
     * An empty vector of ages or none bitset of #InfectionStates% means that no condition on the corresponding property
     * is set!
     */
    TestingCriteria(const std::vector<AgeGroup>& ages, const std::vector<InfectionState>& infection_states);

    /**
     * @brief Compares two TestingCriteria for functional equality.
     */
    bool operator==(const TestingCriteria& other) const;

    /**
     * @brief Add an AgeGroup to the set of AgeGroup%s that are either allowed or required to be tested.
     * @param[in] age_group AgeGroup to be added.
     */
    void add_age_group(const AgeGroup age_group);

    /**
     * @brief Remove an AgeGroup from the set of AgeGroup%s that are either allowed or required to be tested.
     * @param[in] age_group AgeGroup to be removed.
     */
    void remove_age_group(const AgeGroup age_group);

    /**
     * @brief Add an #InfectionState to the set of #InfectionState%s that are either allowed or required to be tested.
     * @param[in] infection_state #InfectionState to be added.
     */
    void add_infection_state(const InfectionState infection_state);

    /**
     * @brief Remove an #InfectionState from the set of #InfectionState%s that are either allowed or required to be
     * tested.
     * @param[in] infection_state #InfectionState to be removed.
     */
    void remove_infection_state(const InfectionState infection_state);

    /**
     * @brief Check if a Person and a Location meet all the required properties to get tested.
     * @param[in] p Person to be checked.
     * @param[in] t TimePoint when to evaluate the TestingCriteria.
     */
    bool evaluate(const Person& p, TimePoint t) const;

    /**
     * serialize this. 
     * @see mio::serialize
     */
    template <class IOContext>
    void serialize(IOContext& io) const
    {
        auto obj = io.create_object("TestingCriteria");
        obj.add_element("ages", m_ages.to_ulong());
        obj.add_element("infection_states", m_infection_states.to_ulong());
    }

    /**
     * deserialize an object of this class.
     * @see mio::deserialize
     */
    template <class IOContext>
    static IOResult<TestingCriteria> deserialize(IOContext& io)
    {
        auto obj              = io.expect_object("TestingCriteria");
        auto ages             = obj.expect_element("ages", Tag<unsigned long>{});
        auto infection_states = obj.expect_element("infection_states", Tag<unsigned long>{});
        return apply(
            io,
            [](auto&& ages_, auto&& infection_states_) {
                return TestingCriteria{ages_, infection_states_};
            },
            ages, infection_states);
    }

private:
    std::bitset<MAX_NUM_AGE_GROUPS> m_ages; ///< Set of #AgeGroup%s that are either allowed or required to be tested.
    std::bitset<(size_t)InfectionState::Count>
        m_infection_states; /**< BitSet of #InfectionState%s that are either allowed or required to
    be tested.*/
};

/**
 * @brief TestingScheme to regular test Person%s.
 */
class TestingScheme
{
public:
    /**
     * @brief Create a TestingScheme.
     * @param[in] testing_criteria Vector of TestingCriteria that are checked for testing.
     * @param[in] minimal_time_since_last_test TimeSpan of how often this scheme applies, i. e., when a new test is
     * performed after a Person's last test.
     * @param start_date Starting date of the scheme.
     * @param end_date Ending date of the scheme.
     * @param test_type The type of test to be performed.
     * @param probability Probability of the test to be performed if a testing rule applies.
     */
    TestingScheme(const TestingCriteria& testing_criteria, TimeSpan minimal_time_since_last_test, TimePoint start_date,
                  TimePoint end_date, const GenericTest& test_type, ScalarType probability);

    /**
     * @brief Compares two TestingScheme%s for functional equality.
     */
    bool operator==(const TestingScheme& other) const;

    /**
     * @brief Get the activity status of the scheme.
     * @return Whether the TestingScheme is currently active.
     */
    bool is_active() const;

    /**
     * @brief Checks if the scheme is active at a given time and updates activity status.
     * @param[in] t TimePoint to be updated at.
     */
    void update_activity_status(TimePoint t);

    /**
     * @brief Runs the TestingScheme and potentially tests a Person.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person being tested.
     * @param[in] person Person to check.
     * @param[in] t TimePoint when to run the scheme.
     * @return If the person is allowed to enter the Location by the scheme.
     */
    bool run_scheme(PersonalRandomNumberGenerator& rng, Person& person, TimePoint t) const;

    /**
     * serialize this. 
     * @see mio::serialize
     */
    template <class IOContext>
    void serialize(IOContext& io) const
    {
        auto obj = io.create_object("TestingScheme");
        obj.add_element("criteria", m_testing_criteria);
        obj.add_element("min_time_since_last_test", m_minimal_time_since_last_test);
        obj.add_element("start_date", m_start_date);
        obj.add_element("end_date", m_end_date);
        obj.add_element("test_type",
                        m_test_type.get_default()); // FIXME: m_test_type should contain TestParameters directly
        obj.add_element("probability", m_probability);
        obj.add_element("is_active", m_is_active);
    }

    /**
     * deserialize an object of this class.
     * @see mio::deserialize
     */
    template <class IOContext>
    static IOResult<TestingScheme> deserialize(IOContext& io)
    {
        auto obj                      = io.expect_object("TestingScheme");
        auto criteria                 = obj.expect_element("criteria", Tag<TestingCriteria>{});
        auto min_time_since_last_test = obj.expect_element("min_time_since_last_test", Tag<TimeSpan>{});
        auto start_date               = obj.expect_element("start_date", Tag<TimePoint>{});
        auto end_date                 = obj.expect_element("end_date", Tag<TimePoint>{});
        auto test_type                = obj.expect_element(
            "test_type", Tag<GenericTest::Type>{}); // FIXME: m_test_type should contain TestParameters directly
        auto probability = obj.expect_element("probability", Tag<ScalarType>{});
        auto is_active   = obj.expect_element("is_active", Tag<bool>{});
        return apply(
            io,
            [](auto&& criteria_, auto&& min_time_since_last_test_, auto&& start_date_, auto&& end_date_,
               auto&& test_type_, auto&& probability_, auto&& is_active_) {
                return TestingScheme{
                    criteria_, min_time_since_last_test_, start_date_, end_date_, test_type_, probability_, is_active_};
            },
            criteria, min_time_since_last_test, start_date, end_date, test_type, probability, is_active);
    }

private:
    TestingCriteria m_testing_criteria; ///< TestingCriteria of the scheme.
    TimeSpan m_minimal_time_since_last_test; ///< Shortest period of time between two tests.
    TimePoint m_start_date; ///< Starting date of the scheme.
    TimePoint m_end_date; ///< Ending date of the scheme.
    GenericTest m_test_type; ///< Type of the test.
    ScalarType m_probability; ///< Probability of performing the test.
    bool m_is_active = false; ///< Whether the scheme is currently active.
};

/**
 * @brief Set of TestingSchemes that are checked for testing.
 */
class TestingStrategy
{
public:
    /**
     * @brief Create a TestingStrategy.
     * @param[in] testing_schemes Vector of TestingSchemes that are checked for testing.
     */
    TestingStrategy() = default;
    explicit TestingStrategy(const std::unordered_map<LocationId, std::vector<TestingScheme>>& location_to_schemes_map);

    /**
     * @brief Add a TestingScheme to the set of schemes that are checked for testing at a certain Location.
     * @param[in] loc_id LocationId key for TestingScheme to be added.
     * @param[in] scheme TestingScheme to be added.
     */
    void add_testing_scheme(const LocationId& loc_id, const TestingScheme& scheme);

    /**
     * @brief Add a TestingScheme to the set of schemes that are checked for testing at a certain LocationType.
     * A TestingScheme applies to all Location of the same type is store in 
     * LocationId{INVALID_LOCATION_INDEX, location_type} of m_location_to_schemes_map.
     * @param[in] loc_type LocationId key for TestingScheme to be added.
     * @param[in] scheme TestingScheme to be added.
     */
    void add_testing_scheme(const LocationType& loc_type, const TestingScheme& scheme)
    {
        add_testing_scheme(LocationId{INVALID_LOCATION_INDEX, loc_type}, scheme);
    }

    /**
     * @brief Remove a TestingScheme from the set of schemes that are checked for testing at a certain Location.
     * @param[in] loc_id LocationId key for TestingScheme to be remove.
     * @param[in] scheme TestingScheme to be removed.
     */
    void remove_testing_scheme(const LocationId& loc_id, const TestingScheme& scheme);

    /**
     * @brief Remove a TestingScheme from the set of schemes that are checked for testing at a certain Location.
     * A TestingScheme applies to all Location of the same type is store in 
     * LocationId{INVALID_LOCATION_INDEX, location_type} of m_location_to_schemes_map.
     * @param[in] loc_type LocationType key for TestingScheme to be remove.
     * @param[in] scheme TestingScheme to be removed.
     */
    void remove_testing_scheme(const LocationType& loc_type, const TestingScheme& scheme)
    {
        remove_testing_scheme(LocationId{INVALID_LOCATION_INDEX, loc_type}, scheme);
    }

    /**
     * @brief Checks if the given TimePoint is within the interval of start and end date of each TestingScheme and then
     * changes the activity status for each TestingScheme accordingly.
     * @param t TimePoint to check the activity status of each TestingScheme.
     */
    void update_activity_status(const TimePoint t);

    /**
     * @brief Runs the TestingStrategy and potentially tests a Person.
     * @param[inout] rng PersonalRandomNumberGenerator of the Person being tested.
     * @param[in] person Person to check.
     * @param[in] location Location to check.
     * @param[in] t TimePoint when to run the strategy.
     * @return If the Person is allowed to enter the Location.
     */
    bool run_strategy(PersonalRandomNumberGenerator& rng, Person& person, const Location& location, TimePoint t);

    /**
     * serialize this. 
     * @see mio::serialize
     */
    template <class IOContext>
    void serialize(IOContext& io) const
    {
        auto obj = io.create_object("TestingStrategy");
        obj.add_list("schemes", m_location_to_schemes_map.cbegin(), m_location_to_schemes_map.cend());
    }

    /**
     * deserialize an object of this class.
     * @see mio::deserialize
     */
    template <class IOContext>
    static IOResult<TestingStrategy> deserialize(IOContext& io)
    {
        auto obj     = io.expect_object("TestingStrategy");
        auto schemes = obj.expect_list("schemes", Tag<std::pair<LocationId, std::vector<TestingScheme>>>{});
        return apply(
            io,
            [](auto&& schemes_) {
                return TestingStrategy{schemes_};
            },
            schemes);
    }

private:
    std::vector<std::pair<LocationId, std::vector<TestingScheme>>>
        m_location_to_schemes_map; ///< Set of schemes that are checked for testing.
};

} // namespace abm
} // namespace mio

#endif
