#include "casm/monte/models/basic_ising_eigen.hh"

#include "casm/misc/CASM_Eigen_math.hh"
#include "casm/misc/CASM_math.hh"
#include "casm/monte/calculators/basic_semigrand_canonical.hh"
#include "gtest/gtest.h"
#include "testdir.hh"

using namespace CASM;
using namespace CASM::monte::models::basic_ising_eigen;
using namespace CASM::monte::calculators::basic_semigrand_canonical;

typedef SemiGrandCanonicalConditions conditions_type;
typedef IsingState<conditions_type> state_type;
typedef std::mt19937_64 engine_type;
typedef IsingSemiGrandCanonicalEventGenerator<conditions_type, engine_type>
    event_generator_type;
typedef event_generator_type::random_number_generator_type
    random_number_generator_type;
typedef IsingFormationEnergy<conditions_type> formation_energy_f_type;
typedef IsingComposition<conditions_type> composition_f_type;
typedef IsingSystem<conditions_type> system_type;
typedef SemiGrandCanonicalPotential<system_type> potential_type;
typedef SemiGrandCanonicalCalculator<system_type, event_generator_type>
    calculator_type;

namespace testing {

state_type make_basic_Ising_default_state(Index rows = 25, Index cols = 25,
                              int fill_value = 1, double temperature = 2000.0,
                              double mu = 0.0) {
  Eigen::VectorXi shape(2);
  shape << rows, cols;
  IsingConfiguration configuration(shape, fill_value);

  Eigen::VectorXd exchange_potential(1);
  exchange_potential << mu;
  conditions_type conditions(temperature, exchange_potential);

  return state_type(configuration, conditions);
}

}  // namespace testing

TEST(BasicIsingEigenTest, IsingConfiguration1) {
  Eigen::VectorXi shape(2);
  shape << 25, 25;
  int fill_value = 1;
  IsingConfiguration configuration(shape, fill_value);

  ASSERT_EQ(configuration.n_sites, shape[0] * shape[1]);

  jsonParser json;
  to_json(configuration, json);
  //  std::cout << json << std::endl;

  IsingConfiguration new_config;
  from_json(new_config, json);
  ASSERT_EQ(new_config.shape, configuration.shape);
  ASSERT_EQ(new_config.occupation(), configuration.occupation());
}

TEST(BasicIsingEigenTest, SemiGrandCanonicalConditions1) {
  typedef SemiGrandCanonicalConditions conditions_type;

  double temperature = 2000.0;
  Eigen::VectorXd exchange_potential(1);
  exchange_potential << 2.0;
  conditions_type conditions(temperature, exchange_potential);

  ASSERT_EQ(conditions.exchange_potential.size(), 1);

  jsonParser json;
  to_json(conditions, json);
  //  std::cout << json << std::endl;

  conditions_type new_conditions;
  from_json(new_conditions, json);
  ASSERT_TRUE(almost_equal(new_conditions.temperature, conditions.temperature));
  ASSERT_EQ(new_conditions.exchange_potential.size(),
            conditions.exchange_potential.size());
  ASSERT_TRUE(almost_equal(new_conditions.exchange_potential(0),
                           conditions.exchange_potential(0)));
}

TEST(BasicIsingEigenTest, SemiGrandCanonicalIsingState1) {
  state_type state = testing::make_basic_Ising_default_state(25, 25);

  ASSERT_EQ(state.configuration.n_sites, 25 * 25);
  ASSERT_EQ(state.conditions.exchange_potential.size(), 1);
}

TEST(BasicIsingEigenTest, IsingSemiGrandCanonicalEventGenerator1) {
  event_generator_type event_generator;

  ASSERT_EQ(event_generator.state, nullptr);

  state_type state = testing::make_basic_Ising_default_state(25, 25);
  event_generator.set_state(&state);

  ASSERT_EQ(event_generator.occ_event.linear_site_index.size(), 1);
  ASSERT_EQ(event_generator.occ_event.new_occ.size(), 1);

  random_number_generator_type random_number_generator;
  for (Index i = 0; i < 10000000; ++i) {
    event_generator.propose(random_number_generator);
    Index l = event_generator.occ_event.linear_site_index[0];
    int new_occ = event_generator.occ_event.new_occ[0];
    ASSERT_TRUE(l >= 0 && l < state.configuration.n_sites);
    ASSERT_TRUE(new_occ == -1 || new_occ == 1);
  }
}

TEST(BasicIsingEigenTest, IsingFormationEnergy1) {
  /// Construct formation energy calculator
  double J = 0.1;
  int lattice_type = 1;  // square lattice ising model
  bool use_nlist = false;
  formation_energy_f_type formation_energy_calculator(J, lattice_type,
                                                      use_nlist);

  /// Construct a state to calculate the formation energy of
  state_type state = testing::make_basic_Ising_default_state(25, 25);
  formation_energy_calculator.set_state(&state);

  /// Check extensive formation energy
  double expected;
  double Ef_extensive = formation_energy_calculator.extensive_value();
  expected = 25 * 25 * 2.0 * -J;
  ASSERT_TRUE(almost_equal(Ef_extensive, expected))
      << Ef_extensive << " != " << expected;

  /// Check intensive formation energy
  double Ef_intensive = formation_energy_calculator.intensive_value();
  expected = 2.0 * -J;
  ASSERT_TRUE(almost_equal(Ef_intensive, expected))
      << Ef_intensive << " != " << expected;

  std::vector<Index> linear_site_index;
  linear_site_index.resize(1);
  std::vector<int> new_occ;
  new_occ.resize(1);
  double dEf;

  /// Check change in extensive formation energy
  linear_site_index[0] = 0;
  new_occ[0] = -1;
  dEf = formation_energy_calculator.occ_delta_extensive_value(linear_site_index,
                                                              new_occ);
  expected = 8.0 * J;
  ASSERT_TRUE(almost_equal(dEf, expected)) << dEf << " != " << expected;

  /// Check no change in extensive formation energy
  linear_site_index[0] = 0;
  new_occ[0] = 1;
  dEf = formation_energy_calculator.occ_delta_extensive_value(linear_site_index,
                                                              new_occ);
  expected = 0.0;
  ASSERT_TRUE(almost_equal(dEf, expected)) << dEf << " != " << expected;
}

TEST(BasicIsingEigenTest, IsingComposition1) {
  /// Construct composition calculator
  composition_f_type composition_calculator;

  /// Construct a state to calculate the composition of
  int fill_value = 1;
  state_type state = testing::make_basic_Ising_default_state(25, 25, fill_value);
  composition_calculator.set_state(&state);

  /// Check extensive composition (n_unitcells*x)
  Eigen::VectorXd expected(1);
  Eigen::VectorXd Nx = composition_calculator.extensive_value();
  expected(0) = state.configuration.n_sites * 1.0;
  ASSERT_TRUE(almost_equal(Nx, expected)) << Nx << " != " << expected;

  /// Check intensive composition (x)
  Eigen::VectorXd x = composition_calculator.intensive_value();
  expected(0) = 1.0;
  ASSERT_TRUE(almost_equal(x, expected)) << x << " != " << expected;

  std::vector<Index> linear_site_index;
  linear_site_index.resize(1);
  std::vector<int> new_occ;
  new_occ.resize(1);
  Eigen::VectorXd dNx;

  /// Check change in extensive composition (n_unitcells*dx)
  linear_site_index[0] = 0;
  new_occ[0] = -1;
  dNx = composition_calculator.occ_delta_extensive_value(linear_site_index,
                                                         new_occ);
  expected(0) = -1;
  ASSERT_TRUE(almost_equal(dNx, expected)) << dNx << " != " << expected;

  /// Check no change in extensive composition (n_unitcells*dx)
  linear_site_index[0] = 0;
  new_occ[0] = 1;
  dNx = composition_calculator.occ_delta_extensive_value(linear_site_index,
                                                         new_occ);
  expected(0) = 0.0;
  ASSERT_TRUE(almost_equal(dNx, expected)) << dNx << " != " << expected;
}

TEST(BasicIsingEigenTest, SemiGrandCanonicalPotential1) {
  /// Construct a state to calculate the semi-grand canonical energy of
  int fill_value = 1;
  double temperature = 2000.0;
  double mu = 2.0;
  state_type state =
      testing::make_basic_Ising_default_state(25, 25, fill_value, temperature, mu);

  /// Construct formation energy calculator
  double J = 0.1;
  int lattice_type = 1;  // square lattice ising model
  bool use_nlist = false;
  formation_energy_f_type formation_energy_calculator(J, lattice_type,
                                                      use_nlist);

  /// Construct composition calculator
  composition_f_type composition_calculator;

  /// Construct system
  std::shared_ptr<system_type> system = std::make_shared<system_type>(
      formation_energy_calculator, composition_calculator);

  /// Construct potential calculator
  potential_type potential(system);
  ASSERT_EQ(potential.state, nullptr);

  potential.set_state(&state);

  /// Check extensive semi-grand canonical energy (Ef - n_unitcells*(mu @ x))
  double expected;
  double E_sgc = potential.extensive_value();
  expected = state.configuration.n_sites * (2.0 * -J - mu * 1.0);
  ASSERT_TRUE(almost_equal(E_sgc, expected)) << E_sgc << " != " << expected;

  /// Check intensive semi-grand canonical energy (ef - mu @ x)
  double e_sgc = potential.intensive_value();
  expected = 2.0 * -J - mu * 1.0;
  ASSERT_TRUE(almost_equal(e_sgc, expected)) << e_sgc << " != " << expected;

  std::vector<Index> linear_site_index;
  linear_site_index.resize(1);
  std::vector<int> new_occ;
  new_occ.resize(1);
  Eigen::VectorXd dNx;

  /// Check change in extensive semi-grand canonical energy
  linear_site_index[0] = 0;
  new_occ[0] = -1;
  double dE_sgc =
      potential.occ_delta_extensive_value(linear_site_index, new_occ);
  expected = 8.0 * J - mu * (-1);
  ASSERT_TRUE(almost_equal(dE_sgc, expected)) << dE_sgc << " != " << expected;

  /// Check no change in extensive semi-grand canonical energy
  linear_site_index[0] = 0;
  new_occ[0] = 1;
  dE_sgc = potential.occ_delta_extensive_value(linear_site_index, new_occ);
  expected = 0.0;
  ASSERT_TRUE(almost_equal(dE_sgc, expected)) << dE_sgc << " != " << expected;
}

TEST(BasicIsingEigenTest, SemiGrandCanonicalRun1) {
  /// Construct a state to calculate the semi-grand canonical energy of
  int fill_value = 1;
  double temperature = 2000.0;
  double mu = 0.0;
  state_type state =
      testing::make_basic_Ising_default_state(25, 25, fill_value, temperature, mu);

  /// Construct formation energy calculator
  double J = 0.1;
  int lattice_type = 1;  // square lattice ising model
  bool use_nlist = false;
  formation_energy_f_type formation_energy_calculator(J, lattice_type,
                                                      use_nlist);

  /// Construct composition calculator
  composition_f_type composition_calculator;

  /// Construct system
  auto system = std::make_shared<system_type>(formation_energy_calculator,
                                              composition_calculator);

  /// Construct Monte Carlo calculator
  auto mc_calculator = std::make_shared<calculator_type>(system);

  /// Construct sampling functions
  monte::StateSamplingFunctionMap sampling_functions;
  std::vector<monte::StateSamplingFunction> fv = {
      make_parametric_composition_f(mc_calculator),
      make_formation_energy_f(mc_calculator),
      make_potential_energy_f(mc_calculator)};
  for (auto const &f : fv) {
    sampling_functions.emplace(f.name, f);
  }

  /// Construct an Ising model semi-grand canonical event proposer / applier
  event_generator_type event_generator;

  /// Completion check params
  monte::CompletionCheckParams<monte::BasicStatistics> completion_check_params;
  completion_check_params.equilibration_check_f =
      monte::default_equilibration_check;
  completion_check_params.calc_statistics_f =
      monte::BasicStatisticsCalculator();

  completion_check_params.cutoff_params.min_sample = 100;

  completion_check_params.log_spacing = false;
  completion_check_params.check_begin = 100;
  completion_check_params.check_period = 10;

  auto &req = completion_check_params.requested_precision;
  req[monte::SamplerComponent("param_composition", 0, "0")] =
      monte::RequestedPrecision::abs(0.001);
  req[monte::SamplerComponent("potential_energy", 0, "0")] =
      monte::RequestedPrecision::abs(0.001);

  /// Create a logger
  test::TmpDir tmpdir;
  std::optional<monte::MethodLog> method_log = monte::MethodLog();
  method_log->logfile_path = tmpdir.path();
  method_log->log_frequency = 10.0;

  /// Number of passes per sample
  int sample_period = 1;

  /// Default constructed random number engine
  std::shared_ptr<engine_type> random_engine;

  /// Write status function
  std::function<void(calculator_type const &, monte::MethodLog &)> write_status_f =
      default_write_status<calculator_type>;

  /// Run
  mc_calculator->run(
      state, sampling_functions, completion_check_params, event_generator,
      sample_period, method_log, random_engine, write_status_f);

  jsonParser json;
  to_json(mc_calculator->data->completion_check.results(), json);
  std::cout << json << std::endl;

  ASSERT_EQ(true, true);
}