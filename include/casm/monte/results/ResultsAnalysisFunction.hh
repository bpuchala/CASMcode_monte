#ifndef CASM_monte_ResultsAnalysisFunction
#define CASM_monte_ResultsAnalysisFunction

#include <vector>

#include "casm/monte/definitions.hh"
#include "casm/monte/misc/math.hh"
#include "casm/monte/results/Results.hh"
#include "casm/monte/state/RunData.hh"

namespace CASM {
namespace monte {

template <typename ConfigType, typename StatisticsType>
struct Results;

// TODO: remove template parameters

/// \brief Use to calculate functions of the sampled data at the
///     end of a run (ex. covariance)
template <typename ConfigType, typename StatisticsType>
struct ResultsAnalysisFunction {
  /// \brief Constructor - default component names
  ResultsAnalysisFunction(std::string _name, std::string _description,
                          std::vector<Index> _shape,
                          std::function<Eigen::VectorXd(
                              RunData<ConfigType> const &,
                              Results<ConfigType, StatisticsType> const &)>
                              _function);

  /// \brief Constructor - custom component names
  ResultsAnalysisFunction(std::string _name, std::string _description,
                          std::vector<std::string> const &_component_names,
                          std::vector<Index> _shape,
                          std::function<Eigen::VectorXd(
                              RunData<ConfigType> const &,
                              Results<ConfigType, StatisticsType> const &)>
                              _function);

  /// \brief Function name
  std::string name;

  /// \brief Description of the function
  std::string description;

  /// \brief Shape of resulting value, with column-major unrolling
  ///
  /// Scalar: [], Vector: [n], Matrix: [m, n], etc.
  std::vector<Index> shape;

  /// \brief A name for each component of the resulting Eigen::VectorXd
  ///
  /// Can be string representing an index (i.e "0", "1", "2", etc.) or can
  /// be a descriptive string (i.e. "susc(Ni,Ni)", "susc(Ni,Al)", etc.)
  std::vector<std::string> component_names;

  /// \brief The function to be evaluated
  std::function<Eigen::VectorXd(RunData<ConfigType> const &,
                                Results<ConfigType, StatisticsType> const &)>
      function;

  /// \brief Evaluates `function`
  Eigen::VectorXd operator()(
      RunData<ConfigType> const &run_data,
      Results<ConfigType, StatisticsType> const &results) const;
};

/// \brief Evaluate all analysis functions
template <typename ConfigType, typename StatisticsType>
std::map<std::string, Eigen::VectorXd> make_analysis(
    RunData<ConfigType> const &run_data,
    Results<ConfigType, StatisticsType> const &results,
    ResultsAnalysisFunctionMap<ConfigType, StatisticsType> const
        &analysis_functions);

// --- Implementation ---

/// \brief Constructor - default component names
template <typename ConfigType, typename StatisticsType>
ResultsAnalysisFunction<ConfigType, StatisticsType>::ResultsAnalysisFunction(
    std::string _name, std::string _description, std::vector<Index> _shape,
    std::function<Eigen::VectorXd(RunData<ConfigType> const &,
                                  Results<ConfigType, StatisticsType> const &)>
        _function)
    : name(_name),
      description(_description),
      shape(_shape),
      component_names(default_component_names(shape)),
      function(_function) {}

/// \brief Constructor - custom component names
template <typename ConfigType, typename StatisticsType>
ResultsAnalysisFunction<ConfigType, StatisticsType>::ResultsAnalysisFunction(
    std::string _name, std::string _description,
    std::vector<std::string> const &_component_names, std::vector<Index> _shape,
    std::function<Eigen::VectorXd(RunData<ConfigType> const &,
                                  Results<ConfigType, StatisticsType> const &)>
        _function)
    : name(_name),
      description(_description),
      shape(_shape),
      component_names(_component_names),
      function(_function) {}

/// \brief Evaluates `function`
template <typename ConfigType, typename StatisticsType>
Eigen::VectorXd ResultsAnalysisFunction<ConfigType, StatisticsType>::operator()(
    RunData<ConfigType> const &run_data,
    Results<ConfigType, StatisticsType> const &results) const {
  return function(run_data, results);
}

/// \brief Evaluate all analysis functions
template <typename ConfigType, typename StatisticsType>
std::map<std::string, Eigen::VectorXd> make_analysis(
    RunData<ConfigType> const &run_data,
    Results<ConfigType, StatisticsType> const &results,
    ResultsAnalysisFunctionMap<ConfigType, StatisticsType> const
        &analysis_functions) {
  std::map<std::string, Eigen::VectorXd> analysis;
  for (auto const &pair : analysis_functions) {
    auto const &f = pair.second;
    try {
      analysis.emplace(f.name, f(run_data, results));
    } catch (std::exception &e) {
      CASM::err_log() << "Results analysis '" << pair.first
                      << "' failed: " << e.what() << std::endl;
      Eigen::VectorXd nan_vector = Eigen::VectorXd::Constant(
          f.component_names.size(),
          std::numeric_limits<double>::signaling_NaN());
      analysis.emplace(f.name, nan_vector);
    }
  }
  return analysis;
}

}  // namespace monte
}  // namespace CASM

#endif
