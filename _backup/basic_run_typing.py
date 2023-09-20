"""PEP 544 Protocols for typing basic Monte Carlo calculators

A basic Monte Carlo calculator supports:

- sampling_functions: which quantities to sample and how
- completion_check_params: how to check for completion
"""
from typing import Any, Optional, Protocol, TypeVar

import numpy as np
import numpy.typing as npt

from libcasm.monte import (
    CompletionCheckParams,
    CompletionCheckResults,
    RandomNumberGenerator,
    Sampler,
    SamplerMap,
    StateSamplingFunctionMap,
    ValueMap,
)
from libcasm.monte.events import (
    IntVector,
    LongVector,
    OccEvent,
    OccLocation,
)

C = TypeVar("C", bound="ConditionsType")


class ConditionsType(Protocol):
    """Monte Carlo thermodynamic conditions

    Attributes
    ----------
    temperature: float
        The temperature, :math:`T`, in K
    """

    temperature: float

    @staticmethod
    def from_values(values: ValueMap) -> C:
        """Construct from a conditions ValueMap"""
        ...

    def to_values(self) -> ValueMap:
        """Construct a conditions ValueMap"""
        ...

    @staticmethod
    def from_dict(data: dict) -> C:
        """Construct from a conditions dict"""
        ...

    @staticmethod
    def to_dict(self) -> dict:
        """Construct a conditions dict"""
        ...


class ConfigurationType(Protocol):
    """Monte Carlo configuration

    Attributes
    ----------
    n_sites: int
        The total number of sites in the Monte Carlo configuration
    n_variable_sites: int
        The number of variable sites in the Monte Carlo configuration
    n_unitcells: int
        The number of unit cells in the Monte Carlo configuration
    """

    n_sites: int
    n_variable_sites: int
    n_unitcells: int

    def occupation(self) -> npt.NDArray[np.int32]:
        """Get the current occupation (as a read-only view)"""
        ...

    def set_occupation(self, occupation: npt.NDArray[np.int32]) -> None:
        """Set the current occupation, without changing supercell shape/size"""
        ...

    def occ(self, linear_site_index: int) -> np.int32:
        """Get the current occupation of one site"""
        ...

    def set_occ(self, linear_site_index: int, value: int) -> None:
        """Set the current occupation of one site"""
        ...

    @staticmethod
    def from_dict(data: dict) -> C:
        """Construct from a configuration dict"""
        ...

    @staticmethod
    def to_dict(self) -> dict:
        """Construct a configuration dict"""
        ...


class StateType(Protocol):
    """Monte Carlo state, including configuration and thermodynamic conditions

    Attributes
    ----------
    configuration: ConfigurationType
        Current Monte Carlo configuration
    conditions: ConditionsType
        Current thermodynamic conditions
    properties: :class:`~libcasm.monte.ValueMap`
        Current calculated properties, if applicable

    """

    configuration: ConfigurationType
    conditions: ConditionsType
    properties: ValueMap


class OccEventGeneratorType(Protocol):
    """Propose and apply Monte Carlo occupation events

    Attributes
    ----------
    state: StateType
        The current state for which events are proposed and applied
    occ_event: OccEvent
        The current proposed event
    """

    state: Optional[StateType]
    occ_event: OccEvent

    def set_state(
        self,
        state: StateType,
    ) -> None:
        """Set the current Monte Carlo state and occupant locations"""
        ...

    def propose(
        self,
        random_number_generator: RandomNumberGenerator,
    ) -> OccEvent:
        """Propose a Monte Carlo occupation event, by setting self.occ_event"""
        ...

    def apply(self, occ_event: OccEvent) -> None:
        """Update the occupation of the current state, using self.occ_event"""
        ...


class OccEventGeneratorType(Protocol):
    """Propose and apply Monte Carlo occupation events

    Attributes
    ----------
    state: StateType
        The current state for which events are proposed and applied
    occ_event: OccEvent
        The current proposed event
    """

    state: Optional[StateType]
    occ_event: OccEvent

    def set_state(
        self,
        state: StateType,
    ) -> None:
        """Set the current Monte Carlo state and occupant locations"""
        ...

    def propose(
        self,
        random_number_generator: RandomNumberGenerator,
    ) -> OccEvent:
        """Propose a Monte Carlo occupation event, by setting self.occ_event"""
        ...

    def apply(self, occ_event: OccEvent) -> None:
        """Update the occupation of the current state, using self.occ_event"""
        ...


class PropertyCalculatorType:
    """Calculate a property of a state, and changes in the property for an event

    Return types could be float or np.ndarray of float

    Attributes
    ----------
    state: StateType
        The state the property is calculated for
    """

    state: StateType

    def set_state(self, state: StateType):
        """Set the current Monte Carlo state"""
        ...

    def per_supercell(self) -> Any:
        """Calculate and return the per_supercell value of the property"""
        ...

    def per_unitcell(self) -> Any:
        """Calculate and return the per_unitcell value of the property"""
        ...

    def occ_delta_per_supercell(
        self,
        linear_site_index: LongVector,
        new_occ: IntVector,
    ) -> Any:
        """Calculate and return the change in the per_supercell value of the property due \
        to an occupation change
        """
        ...


class MonteCarloCalculatorDataType(Protocol):
    """Monte Carlo calculator data structure

    Attributes
    ----------
    samplers: :class:`~libcasm.monte.SamplerMap`
        Holds sampled data
    sample_weight: :class:`~libcasm.monte.Sampler`
        Sample weights remain empty (unweighted)
    n_accept: int
        The number of acceptances during `run`
    n_reject: int
        The number of rejections during `run`
    completion_check_results: :class:`~libcasm.monte.CompletionCheckResults`
        Completion check results
    """
    sampling_functions: StateSamplingFunctionMap
    samplers: SamplerMap
    sample_weight: Sampler
    n_accept: int
    n_reject: int
    completion_check_results: CompletionCheckResults


class BasicMonteCarloOccEventCalculatorType(Protocol):
    """The system implementation

    Notes
    -----
    - Sampling functions can expect that attributes which are PropertyCalculator are
      set to calculate self.state by the `run` method.
    -
    """

    system: Any
    state: Optional[StateType]
    potential: PropertyCalculatorType
    formation_energy_calculator: PropertyCalculatorType
    composition_calculator: PropertyCalculatorType
    data: Optional[MonteCarloCalculatorDataType]

    def run(
        self,
        state: StateType,
        sampling_functions: StateSamplingFunctionMap,
        completion_check_params: CompletionCheckParams,
        event_generator: OccEventGeneratorType,
    ) -> None:
        """Runs the Monte Carlo calculator to completion

        Notes
        -----
        Not specified by the protocol but often included:
        - state: The initial configuration and thermodynamic conditions
        - occ_location: Occupant tracking
        - sampling parameters: What to sample and when
        - random number engine: Generate random numbers
        - logging: Log calculator progress

        Parameters
        ----------
        state: State
            The initial configuration and thermodynamic conditions
        sampling_functions: StateSamplingFunctionMap
            The sampling functions to use
        completion_check_params: CompletionCheckParams
            Completion criteria
        event_generator: OccEventGenerator
            Proposes and applies events

        Returns
        -------
        results: MonteCarloOccEventCalculatorResults
            Results of the Monte Carlo calculator, including at minimum sampled data
            and completion check results.
        """
        ...