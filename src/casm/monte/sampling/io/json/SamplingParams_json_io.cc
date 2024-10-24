#include "casm/monte/sampling/io/json/SamplingParams_json_io.hh"

#include "casm/casm_io/container/json_io.hh"
#include "casm/casm_io/json/InputParser_impl.hh"
#include "casm/monte/sampling/SamplingParams.hh"

namespace CASM {
namespace monte {

/// \brief Construct SamplingParams from JSON
///
/// Expected:
///   sample_by: string (optional, default=(depends on calculation type))
///     What to count when determining when to sample the Monte Carlo state.
///     One of "pass", "step", "time" (not valid for all Monte Carlo methods).
///     A "pass" is a number of steps, equal to one step per site with degrees
///     of freedom (DoF).
///
///   spacing: string (optional, default="linear")
///     The spacing of samples in the specified `"period"`. One of "linear"
///     or "log".
///
///     For "linear" spacing, the n-th sample will be taken when:
///
///         sample/pass = round( begin + (period / samples_per_period) * n )
///                time = begin + (period / samples_per_period) * n
///
///     For "log" spacing, the n-th sample will be taken when:
///
///         sample/pass = round( begin + period ^ ( (n + shift) /
///                           samples_per_period ) )
///                time = begin + period ^ ( (n + shift) / samples_per_period )
///
///   begin: number (optional, default=0.0)
///     The number of pass/step or amount of time at which to begin
///     sampling.
///
///   period: number (required)
///     A number of pass/step or amount of time.
///
///   samples_per_period: number (optional, default=1.0)
///     The number of samples to be taken in the specified `"period"`.
///
///   shift: number (optional, default=0.0)
///     Used with `"spacing": "log"`.
///
///   stochastic_sample_period: bool (optional, default=false)
///     If true, then instead of setting the sample time / count
///     deterministally, use the sampling period to determine the
///     sampling rate and determine the next sample time / count
///     stochastically with equivalent mean rate.
///
///   quantities: array of string (optional)
///     Specifies which quantities will be sampled. Options depend on the
///     type of Monte Carlo calculation and should be keys in the
///     sampling_functions map.
///
///   save_initial_state: bool (optional, default=false)
///     If true, request that the initial configuration is saved.
///
///   save_final_state: bool (optional, default=false)
///     If true, request that the final configuration is saved.
///
///   sample_trajectory: bool (optional, default=false)
///     If true, request that the entire configuration is saved each time
///     samples are taken.
///
void parse(InputParser<SamplingParams> &parser,
           std::set<std::string> const &sampling_function_names,
           bool time_sampling_allowed) {
  SamplingParams sampling_params;

  // "sample_by"
  std::unique_ptr<std::string> sample_mode =
      parser.require<std::string>("sample_by");
  if (sample_mode == nullptr) {
    return;
  }
  if (*sample_mode == "pass") {
    sampling_params.sample_mode = SAMPLE_MODE::BY_PASS;
  } else if (*sample_mode == "step") {
    sampling_params.sample_mode = SAMPLE_MODE::BY_STEP;
  } else if (time_sampling_allowed && *sample_mode == "time") {
    sampling_params.sample_mode = SAMPLE_MODE::BY_TIME;
  } else {
    if (time_sampling_allowed) {
      parser.insert_error("sample_by",
                          "Error: \"sample_mode\" must be one of \"pass\", "
                          "\"step\", or \"time\".");
    } else {
      parser.insert_error(
          "sample_by",
          "Error: \"sample_mode\" must be one of \"pass\" or \"step\".");
    }
  }

  // "spacing"
  std::string sample_method = "linear";
  parser.optional(sample_method, "spacing");
  if (sample_method == "linear") {
    sampling_params.sample_method = SAMPLE_METHOD::LINEAR;
  } else if (sample_method == "log") {
    sampling_params.sample_method = SAMPLE_METHOD::LOG;
  } else {
    parser.insert_error(
        "spacing", "Error: \"spacing\" must be one of \"linear\", \"log\".");
  }

  // "begin"
  sampling_params.begin = 0.0;
  parser.optional(sampling_params.begin, "begin");

  // "period"
  parser.require(sampling_params.period, "period");
  if (sampling_params.sample_method == SAMPLE_METHOD::LOG &&
      sampling_params.period <= 1.0) {
    parser.insert_error(
        "period", "Error: For \"spacing\"==\"log\", \"period\" must > 1.0.");
  }
  if (sampling_params.sample_method == SAMPLE_METHOD::LINEAR &&
      sampling_params.period <= 0.0) {
    parser.insert_error(
        "period", "Error: For \"spacing\"==\"log\", \"period\" must > 0.0.");
  }

  // "samples_per_period"
  sampling_params.samples_per_period = 1.0;
  parser.optional(sampling_params.samples_per_period, "samples_per_period");

  // "shift"
  sampling_params.shift = 0.0;
  parser.optional(sampling_params.shift, "shift");

  // "stochastic_sample_period"
  sampling_params.stochastic_sample_period = false;
  parser.optional(sampling_params.stochastic_sample_period,
                  "stochastic_sample_period");

  // "quantities"
  parser.optional(sampling_params.sampler_names, "quantities");
  for (std::string name : sampling_params.sampler_names) {
    if (!sampling_function_names.count(name)) {
      std::stringstream msg;
      msg << "Error: \"" << name << "\" is not a sampling option.";
      parser.insert_error("quantities", msg.str());
    }
  }

  // "sample_trajectory"
  parser.optional(sampling_params.do_sample_trajectory, "sample_trajectory");

  sampling_params.do_sample_time = time_sampling_allowed;

  if (parser.valid()) {
    parser.value = std::make_unique<SamplingParams>(sampling_params);
  }
}

}  // namespace monte
}  // namespace CASM
