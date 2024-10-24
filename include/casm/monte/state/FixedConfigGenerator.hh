#ifndef CASM_monte_FixedConfigGenerator
#define CASM_monte_FixedConfigGenerator

#include <vector>

#include "casm/monte/definitions.hh"
#include "casm/monte/state/ConfigGenerator.hh"
#include "casm/monte/state/RunData.hh"
#include "casm/monte/state/State.hh"

namespace CASM {
namespace monte {

/// \brief A `ConfigGenerator` for state generation -- always returns the same
/// configuration
///
/// - Returns the same configuration no matter what the current
///   conditions and previous runs are.
template <typename _ConfigType>
class FixedConfigGenerator
    : public ConfigGenerator<_ConfigType, RunData<_ConfigType>> {
 public:
  typedef _ConfigType ConfigType;
  typedef RunData<ConfigType> RunInfoType;
  FixedConfigGenerator(ConfigType const &configuration)
      : m_configuration(configuration) {}

  ConfigType operator()(
      ValueMap const &conditions,
      std::vector<RunData<ConfigType>> const &completed_runs) override {
    return m_configuration;
  }

 private:
  ConfigType m_configuration;
};

}  // namespace monte
}  // namespace CASM

#endif
