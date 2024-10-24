#ifndef CASM_monte_methods_kinetic_monte_carlo
#define CASM_monte_methods_kinetic_monte_carlo

// logging
#include "casm/casm_io/Log.hh"
#include "casm/casm_io/container/stream_io.hh"
#include "casm/casm_io/json/jsonParser.hh"
#include "casm/monte/MethodLog.hh"
#include "casm/monte/checks/io/json/CompletionCheck_json_io.hh"
#include "casm/monte/events/OccLocation.hh"

namespace CASM {
namespace monte {

/// \brief Data that can be used by sampling functions
template <typename ConfigType, typename EngineType>
struct KMCData {
  /// \brief This will be set to the current sampling
  ///     fixture label before sampling data.
  std::string sampling_fixture_label;

  /// \brief This will be set to point to the current state
  ///     sampler sampling data.
  monte::StateSampler<ConfigType, EngineType> const *state_sampler;

  /// \brief This will be set to the total event rate at sampling time
  double total_rate;

  /// \brief Current simulation time
  ///
  /// For time-based sampling this will be equal to the time the
  /// For count-based sampling, this will be equal to the time the
  /// event occurred.
  double time;

  /// \brief Simulation time at last sample, by sampling fixture label
  ///
  /// This will be set to store the time when the last sample
  /// was taken, with key equal to sampling fixture label.
  std::map<std::string, double> prev_time;

  /// \brief Set this to hold atom names for each column of the
  ///     atom_positions_cart matrix
  ///
  /// When sampling, this will hold the atom name index for each column of the
  /// atom position matrices. Currently atom names only; does not distinguish
  /// atoms with different properties. Not set by monte::kinetic_monte_carlo,
  /// this must be set beforehand. TODO: KMC with atoms that move to/from
  /// resevoir will need to update this
  std::vector<Index> atom_name_index_list;

  /// \brief Current atom positions
  ///
  /// This will be set to store positions since occ_location was
  /// initialized. Before a sample is taken, this will be updated to
  /// contain the current atom positions in Cartesian coordinates,
  /// with shape=(3, n_atoms). Sampling functions can use this
  /// to calculate displacements since the begininning of the
  /// calculation or since the last sample time.
  Eigen::MatrixXd atom_positions_cart;

  /// \brief Atom positions at last sample, by sampling fixture label
  ///
  /// This will be set to store positions since occ_location was
  /// initialized. The keys are sampling fixture label, and the values
  /// will be set to contain the atom positions, in Cartesian
  /// coordinates, with shape=(3, n_atoms), at the previous sample time.
  /// Sampling functions can use this to calculate displacements since
  /// the begininning of the calculation or since the last sample time.
  std::map<std::string, Eigen::MatrixXd> prev_atom_positions_cart;
};

template <typename EventIDType, typename ConfigType, typename EventSelectorType,
          typename GetEventType, typename StatisticsType, typename EngineType>
void kinetic_monte_carlo(
    State<ConfigType> &state, OccLocation &occ_location,
    KMCData<ConfigType, EngineType> &kmc_data,
    EventSelectorType &event_selector, GetEventType get_event_f,
    RunManager<ConfigType, StatisticsType, EngineType> &run_manager);

// --- Implementation ---

/// \brief Run a kinetic Monte Carlo calculation
///
/// TODO: clean up the way data is made available to samplers, especiallly
/// for storing and sharing data taken at the previous sample time.
///
/// \param state The state. Consists of both the initial
///     configuration and conditions. Conditions must include `temperature`
///     and any others required by `potential`.
/// \param occ_location An occupant location tracker, which enables efficient
///     event proposal. It must already be initialized with the input state.
/// \param kmc_data Stores data to be made available to the sampling functions
///     along with the current state.
/// \param event_selector A method that selects events and returns an
///     std::pair<EventIDType, TimeIncrementType>.
/// \param get_event_f A method that gives an `OccEvent const &` corresponding
///     to the selected EventID.
/// \param run_manager Contains sampling fixtures and after completion holds
///     final results
///
/// \returns A Results<ConfigType> instance with run results.
///
/// Required interface for `State<ConfigType>`:
/// - `Eigen::VectorXi &get_occupation(State<ConfigType> const &state)`
/// - `Eigen::Matrix3l const &get_transformation_matrix_to_super(
///        State<ConfigType> const &state)`
///
/// State properties that are set:
/// - None
///
template <typename EventIDType, typename ConfigType, typename EventSelectorType,
          typename GetEventType, typename StatisticsType, typename EngineType>
void kinetic_monte_carlo(
    State<ConfigType> &state, OccLocation &occ_location,
    KMCData<ConfigType, EngineType> &kmc_data,
    EventSelectorType &event_selector, GetEventType get_event_f,
    RunManager<ConfigType, StatisticsType, EngineType> &run_manager) {
  // Used within the main loop:
  double total_rate;
  double event_time;
  double time_increment;
  clexmonte::EventID selected_event_id;

  // Initialize atom positions & time
  kmc_data.time = 0.0;
  kmc_data.atom_positions_cart = occ_location.atom_positions_cart();
  kmc_data.prev_atom_positions_cart.clear();
  for (auto &fixture : run_manager.sampling_fixtures) {
    kmc_data.prev_time.emplace(fixture.label(), kmc_data.time);
    kmc_data.prev_atom_positions_cart.emplace(fixture.label(),
                                              kmc_data.atom_positions_cart);
  }

  // Pre- and post- sampling actions

  // notes: it is important this uses
  // - the total_rate obtained before event selection
  auto pre_sample_action =
      [&](SamplingFixture<ConfigType, StatisticsType, EngineType> &fixture,
          State<ConfigType> const &state) {
        // set data that can be used in sampling functions
        kmc_data.sampling_fixture_label = fixture.label();
        kmc_data.state_sampler = &fixture.state_sampler();
        kmc_data.atom_positions_cart = occ_location.atom_positions_cart();
        kmc_data.total_rate = total_rate;
        if (kmc_data.state_sampler->sample_mode == SAMPLE_MODE::BY_TIME) {
          kmc_data.time = kmc_data.state_sampler->next_sample_time;
        }
      };

  auto post_sample_action =
      [&](SamplingFixture<ConfigType, StatisticsType, EngineType> &fixture,
          State<ConfigType> const &state) {
        // set data that can be used in sampling functions
        kmc_data.prev_time[fixture.label()] = kmc_data.time;
        kmc_data.prev_atom_positions_cart[fixture.label()] =
            kmc_data.atom_positions_cart;
      };

  // Main loop
  run_manager.initialize(state, occ_location.mol_size());
  run_manager.update_next_sampling_fixture();
  while (!run_manager.is_complete()) {
    run_manager.write_status_if_due();

    // Select an event
    total_rate = event_selector.total_rate();
    std::tie(selected_event_id, time_increment) = event_selector.select_event();
    event_time = kmc_data.time + time_increment;

    // Sample data, if a sample is due by count
    run_manager.sample_data_by_count_if_due(state, pre_sample_action,
                                            post_sample_action);

    // Sample data, if a sample is due by time
    run_manager.sample_data_by_time_if_due(event_time, state, pre_sample_action,
                                           post_sample_action);

    // Apply event
    run_manager.increment_n_accept();
    occ_location.apply(get_event_f(selected_event_id), get_occupation(state));
    kmc_data.time = event_time;

    // Set time -- for all fixtures
    run_manager.set_time(event_time);

    // Increment count -- for all fixtures
    run_manager.increment_step();
  }

  run_manager.finalize(state);
}

}  // namespace monte
}  // namespace CASM

#endif
