#ifndef CASM_monte_State
#define CASM_monte_State

#include <map>
#include <string>

#include "casm/global/eigen.hh"
#include "casm/monte/definitions.hh"

namespace CASM {
namespace monte {

/// A state of a Monte Carlo calculation
template <typename _ConfigType>
struct State {
  typedef _ConfigType ConfigType;

  State(ConfigType const &_configuration,
        VectorValueMap _conditions = VectorValueMap(),
        VectorValueMap _properties = VectorValueMap())
      : configuration(_configuration),
        conditions(_conditions),
        properties(_properties) {}

  /// Current configuration
  ConfigType configuration;

  /// Conditions of the state
  ///
  /// Thermodynamic conditions or calculation constraints, such as temperature,
  /// chemical potential (for grand canonical Monte Carlo), composition (for
  /// canonical Monte Carlo), etc., depending on the type of Monte Carlo
  /// calculation
  VectorValueMap conditions;

  /// Properties of the state
  ///
  /// Properties of the state could be formation_energy, potential_energy,
  /// comp_n, etc., depending on the type of Monte Carlo calculation.
  VectorValueMap properties;
};

}  // namespace monte
}  // namespace CASM

#endif
