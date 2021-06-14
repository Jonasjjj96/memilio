#ifndef MIGRATION_H
#define MIGRATION_H

#include "epidemiology/migration/graph_simulation.h"
#include "epidemiology/utils/time_series.h"
#include "epidemiology/utils/eigen.h"
#include "epidemiology/utils/eigen_util.h"
#include "epidemiology/utils/metaprogramming.h"
#include "epidemiology/model/simulation.h"
#include "epidemiology/utils/compiler_diagnostics.h"
#include "epidemiology/math/euler.h"
#include "epidemiology/secir/contact_matrix.h"
#include "epidemiology/secir/dynamic_npis.h"

#include <cassert>

namespace epi
{

/**
 * represents the simulation in one node of the graph.
 */
template <class Model>
class ModelNode
{
public:
    template <class... Args, typename = std::enable_if_t<std::is_constructible<Model, Args...>::value, void>>
    ModelNode(Args&&... args)
        : model(std::forward<Args>(args)...)
        , m_last_state(model.get_result().get_last_value())
        , m_t0(model.get_result().get_last_time())
    {
    }

    /**
     * get the result of the simulation in this node.
     */
    decltype(auto) get_result() const
    {
        return model.get_result();
    }

    /**
     * get the result of the simulation in this node.
     */
    decltype(auto) get_result()
    {
        return model.get_result();
    }

    /**
     * get the the simulation in this node.
     */
    Model& get_simulation()
    {
        return model;
    }

    Eigen::Ref<const Eigen::VectorXd> get_last_state() const
    {
        return m_last_state;
    }

    double get_t0() const
    {
        return m_t0;
    }

    void evolve(double t, double dt)
    {
        model.advance(t + dt);
        m_last_state = model.get_result().get_last_value();
    }

    Model model;

private:
    Eigen::VectorXd m_last_state;
    double m_t0;
};

/**
 * time dependent migration coefficients.
 */ 
using MigrationCoefficients = DampingMatrixExpression<VectorDampings>;

/**
 * sum of time dependent migration coefficients.
 * differentiate between sources of migration.
 */ 
using MigrationCoefficientGroup = DampingMatrixExpressionGroup<MigrationCoefficients>; 

/**
 * parameters that influence migration.
 */
class MigrationParameters
{
public:
    /**
     * constructor from migration coefficients.
     * @param coeffs migration coefficients
     */
    MigrationParameters(const MigrationCoefficientGroup& coeffs)
        : m_coefficients(coeffs)
    {
    }

    /**
     * constructor from migration coefficients.
     * @param coeffs migration coefficients
     */
    MigrationParameters(const Eigen::VectorXd& coeffs)
        : m_coefficients({MigrationCoefficients(coeffs)})
    {
    }

    /** 
     * equality comparison operators
     */
    //@{
    bool operator==(const MigrationParameters& other) const
    {
        return m_coefficients == other.m_coefficients;
    }
    bool operator!=(const MigrationParameters& other) const
    {
        return m_coefficients != other.m_coefficients;
    }
    //@}

    /**
     * Get/Setthe migration coefficients.
     * The coefficients represent the (time-dependent) percentage of people migrating 
     * from one node to another by age and infection compartment. 
     * @{
     */
    /**
     * @return the migration coefficients.
     */
    const MigrationCoefficientGroup& get_coefficients() const
    {
        return m_coefficients;
    }
    MigrationCoefficientGroup& get_coefficients()
    {
        return m_coefficients;
    }
    /**
     * @param coeffs the migration coefficients.
     */
    void set_coefficients(const MigrationCoefficientGroup& coeffs)
    {
        m_coefficients = coeffs;
    }
    /** @} */

    /**
     * Get/Set dynamic NPIs that are implemented when relative infections exceed thresholds.
     * This feature is optional. The simulation model needs to overload the get_infected_relative function.
     * @{
     */
    /**
     * @return dynamic NPIs for relative infections.
     */
    const DynamicNPIs& get_dynamic_npis_infected() const
    {
        return m_dynamic_npis;
    }
    DynamicNPIs& get_dynamic_npis_infected()
    {
        return m_dynamic_npis;
    }
    /**
     * @param v dynamic NPIs for relative infections.
     */
    void set_dynamic_npis_infected(const DynamicNPIs& v)
    {
        m_dynamic_npis = v;
    }
    /** @} */

private:
    MigrationCoefficientGroup m_coefficients; //one per group and compartment
    DynamicNPIs m_dynamic_npis;
};

/** 
 * represents the migration between two nodes.
 */
class MigrationEdge
{
public:
    /**
     * create edge with coefficients.
     * @param coeffs % of people in each group and compartment that migrate in each time step.
     */
    MigrationEdge(const MigrationParameters& params)
        : m_parameters(params)
        , m_migrated(params.get_coefficients().get_shape().rows())
        , m_return_times(0)
        , m_return_migrated(false)
    {
    }

    /**
     * create edge with coefficients.
     * @param coeffs % of people in each group and compartment that migrate in each time step.
     */
    MigrationEdge(const Eigen::VectorXd& coeffs)
        : m_parameters(coeffs)
        , m_migrated(coeffs.rows())
        , m_return_times(0)
        , m_return_migrated(false)
    {
    }

    /**
     * get the migration parameters.
     */
    const MigrationParameters& get_parameters() const
    {
        return m_parameters;
    }

    /**
     * compute migration from node_from to node_to.
     * migration is based on coefficients.
     * migrants are added to the current state of node_to, subtracted from node_from.
     * on return, migrants (adjusted for infections) are subtracted from node_to, added to node_from.
     * @param t current time
     * @param dt last time step (fixed to 0.5 for migration model)
     * @param node_from node that people migrated from, return to
     * @param node_to node that people migrated to, return from
     */
    template <class Model>
    void apply_migration(double t, double dt, ModelNode<Model>& node_from, ModelNode<Model>& node_to);

private:
    MigrationParameters m_parameters;
    TimeSeries<double> m_migrated;
    TimeSeries<double> m_return_times;
    bool m_return_migrated;
    double m_t_last_dynamic_npi_check = -std::numeric_limits<double>::infinity();
    std::pair<double, SimulationTime> m_dynamic_npi = {-std::numeric_limits<double>::max(), epi::SimulationTime(0)};
};

/**
 * adjust number of migrated people when they return according to the model.
 * E.g. during the time in the other node, some people who left as susceptible will return exposed.
 * Implemented for general compartmentmodel simulations, overload for your custom model if necessary
 * so that it can be found with argument-dependent lookup, i.e. in the same namespace as the model.
 * @param[inout] migrated number of people that migrated as input, number of people that return as output
 * @param params parameters of model in the node that the people migrated to.
 * @param total total population in the node that the people migrated to.
 * @param t time of migration
 * @param dt time between migration and return
 */
template <typename Model, class = std::enable_if_t<is_compartment_model_simulation<Model>::value>>
void calculate_migration_returns(Eigen::Ref<TimeSeries<double>::Vector> migrated, const Model& model,
                                 Eigen::Ref<const TimeSeries<double>::Vector> total, double t, double dt)
{    
    auto y0 = migrated.eval();
    auto y1 = migrated;
    EulerIntegratorCore().step(
        [&](auto&& y, auto&& t_, auto&& dydt) {
        model.get_model().get_derivatives(total, y, t_, dydt);
        },
        y0, t, dt, y1);
}

/**
 * detect a get_infections_relative function for the Model type.
 */
template<class Model>
using get_infections_relative_expr_t = decltype(get_infections_relative(std::declval<const Model&>(), std::declval<const Eigen::Ref<const Eigen::VectorXd>&>()));

/**
 * get the percantage of infected people of the total population in the node
 * If dynamic NPIs are enabled, there needs to be an overload of get_infections_relative(model, y)
 * for the Model type that can be found with argument-dependent lookup. Ideally define get_infections_relative 
 * in the same namespace as the Model type.
 * @param node a node of a migration graph.
 * @param y the current value of the simulation.
 */
template< class Model, std::enable_if_t<!is_expression_valid<get_infections_relative_expr_t, Model>::value, void*> = nullptr>
double get_infections_relative(const ModelNode<Model>& /*node*/, const Eigen::Ref<const Eigen::VectorXd>& /*y*/)
{
    assert(false && "Overload get_infections_relative for your own model/simulation if you want to use dynamic NPIs.");
    return 0;
}
template< class Model, std::enable_if_t<is_expression_valid<get_infections_relative_expr_t, Model>::value, void*> = nullptr>
double get_infections_relative(const ModelNode<Model>& node, const Eigen::Ref<const Eigen::VectorXd>& y)
{
    return get_infections_relative(node.model, y);
}

template <class Model>
void MigrationEdge::apply_migration(double t, double dt, ModelNode<Model>& node_from, ModelNode<Model>& node_to)
{
    //check dynamic npis
    if (m_t_last_dynamic_npi_check == -std::numeric_limits<double>::infinity()) {
        m_t_last_dynamic_npi_check = node_from.get_t0();
    }

    auto& dyn_npis = m_parameters.get_dynamic_npis_infected();
    if (dyn_npis.get_thresholds().size() > 0 && floating_point_greater_equal(t, m_t_last_dynamic_npi_check + dyn_npis.get_interval().get())) {
        auto inf_rel            = get_infections_relative(node_from, node_from.get_last_state()) * dyn_npis.get_base_value();
        auto exceeded_threshold = dyn_npis.get_max_exceeded_threshold(inf_rel);
        if (exceeded_threshold != dyn_npis.get_thresholds().end() &&
            (exceeded_threshold->first > m_dynamic_npi.first ||
             t > double(m_dynamic_npi.second))) { //old NPI was weaker or is expired
            auto t_end    = epi::SimulationTime(t + double(dyn_npis.get_duration()));
            m_dynamic_npi = std::make_pair(exceeded_threshold->first, t_end);
            epi::implement_dynamic_npis(
                m_parameters.get_coefficients(), exceeded_threshold->second, SimulationTime(t), t_end, [this](auto& g) {
                    return epi::make_migration_damping_vector(m_parameters.get_coefficients().get_shape(), g);
                });
        }
    }

    //returns
    for (Eigen::Index i = m_return_times.get_num_time_points() - 1; i >= 0; --i) {
        if (m_return_times.get_time(i) <= t) {
            auto v0 = find_value_reverse(node_to.get_result(), m_migrated.get_time(i), 1e-10, 1e-10);
            assert(v0 != node_to.get_result().rend() && "unexpected error.");
            calculate_migration_returns(m_migrated[i], node_to.model, *v0, m_migrated.get_time(i), dt);
            node_from.get_result().get_last_value() += m_migrated[i];
            node_to.get_result().get_last_value() -= m_migrated[i];
            m_migrated.remove_time_point(i);
            m_return_times.remove_time_point(i);
        }
    }

    if (!m_return_migrated && (m_parameters.get_coefficients().get_matrix_at(t).array() > 0.0).any()) {
        //normal daily migration
        m_migrated.add_time_point(t, (node_from.get_last_state().array() * m_parameters.get_coefficients().get_matrix_at(t).array()).matrix());
        m_return_times.add_time_point(t + dt);

        node_to.get_result().get_last_value() += m_migrated.get_last_value();
        node_from.get_result().get_last_value() -= m_migrated.get_last_value();
    }
    m_return_migrated = !m_return_migrated;
}

/**
 * edge functor for migration simulation.
 * @see ModelNode::evolve
 */
template <class Model>
void evolve_model(double t, double dt, ModelNode<Model>& node)
{
    node.evolve(t, dt);
}

/**
 * edge functor for migration simulation.
 * @see MigrationEdge::apply_migration
 */
template <class Model>
void apply_migration(double t, double dt, MigrationEdge& migrationEdge, ModelNode<Model>& node_from,
                     ModelNode<Model>& node_to)
{
    migrationEdge.apply_migration(t, dt, node_from, node_to);
}

/**
 * create a migration simulation.
 * After every second time step, for each edge a portion of the population corresponding to the coefficients of the edge
 * moves from one node to the other. In the next timestep, the migrated population return to their "home" node. 
 * Returns are adjusted based on the development in the target node. 
 * @param t0 start time of the simulation
 * @param dt time step between migrations
 * @param graph set up for migration simulation
 */
template <typename Model>
GraphSimulation<Graph<ModelNode<Model>, MigrationEdge>>
make_migration_sim(double t0, double dt, const Graph<ModelNode<Model>, MigrationEdge>& graph)
{
    return make_graph_sim(t0, dt, graph, &evolve_model<Model>, &apply_migration<Model>);
}

} // namespace epi

#endif //MIGRATION_H
