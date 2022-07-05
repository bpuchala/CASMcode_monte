#ifndef CASM_monte_OccLocation
#define CASM_monte_OccLocation

#include <vector>

#include "casm/crystallography/UnitCellCoord.hh"
#include "casm/global/definitions.hh"

class MTRand;

namespace CASM {
namespace monte {

class Conversions;
struct OccCandidate;
class OccSwap;
class OccCandidateList;

/// \brief Represents an atom in a molecule
struct Atom {
  Atom(xtal::UnitCellCoord _bijk_begin)
      : bijk_begin(_bijk_begin), delta_ijk(0, 0, 0) {}

  Index species_index;  ///< Species type index
  Index atom_index;     ///< Index into xtal::Molecule for this species_index
  Index id;             ///< Location in OccLocation.m_atoms
  xtal::UnitCellCoord bijk_begin;  ///< Saves initial position
  xtal::UnitCell delta_ijk;        ///< Saves change in position
  Index mol_comp_begin;            ///< Saves initial Mol.component index
};

/// \brief Represents the occupant on a site
///
/// - May be divisible into components or indivisible
struct Mol {
  Index id;             ///< Location in OccLocation.m_mol
  Index l;              ///< Location in config
  Index asym;           ///< Asym unit index (must be consistent with l)
  Index species_index;  ///< Species type index (must be consistent with
                        ///< config.occ(l))
  std::vector<Index>
      component;  ///< Location of component atom in OccLocation.m_atoms
  Index loc;      ///< Location in OccLocation.m_loc
};

struct OccTransform {
  Index l;             ///< Config occupant that is being transformed
  Index mol_id;        ///< Location in OccLocation.m_mol
  Index asym;          ///< Asym index
  Index from_species;  ///< Species index before transformation
  Index to_species;    ///< Species index after transformation
};

struct AtomLocation {
  Index l;         ///< Config occupant that is being transformed
  Index mol_id;    ///< Location in OccLocation.m_mol
  Index mol_comp;  ///< Location in mol.components
};

struct AtomTraj {
  AtomLocation from;
  AtomLocation to;
  xtal::UnitCell delta_ijk;
};

/// \brief Describes a Monte Carlo event that modifies occupation
struct OccEvent {
  /// \brief Linear site indices, indicating on which sites the occupation will
  ///     be modified
  std::vector<Index> linear_site_index;

  /// \brief Occupant indices, indicating the new occupation index on the sites
  ///     being modified
  std::vector<int> new_occ;

  /// \brief Information used to update occupant tracking information stored in
  ///     OccLocation
  std::vector<OccTransform> occ_transform;

  /// \brief Information used to update occupant tracking information stored in
  ///     OccLocation - use if tracking species trajectories for KMC
  std::vector<AtomTraj> atom_traj;
};

/// \brief Stores data to enable efficient proposal and update of occupation
/// mutation
///
/// What data it has:
/// - Input Conversions provides information about conversions between site
///   indices and asymmetric unit indices, species indices and site occupant
///   indices
/// - Input OccCandidateList specifies all unique (asymmetric unit, species
///   index) pairs
/// - `mol` list (type=monte::Mol, shape=(number of mutating sites,) ), stores
///   information about each of the occupants currently in the supercell
///    including site_index (l), asymmetric unit index (asym), species_index.
/// - `loc` list (type=Index, shape=(number of OccCandidate, number of current
///   occupants of that OccCandidate type)), stores the indices in the `mol`
///   list (mol_id) for all occupants of each OccCandidate type
///
/// Choosing events:
/// - `loc` list can be used to choose amongst particular types of occupants
///   (asymmeric unit and specie_index)
///
/// Updating after events occur, use `apply`:
/// - Both `loc`, and `mol` are updated.
///
/// For molecule support:
/// - `species` list (type=Monte::Atom, shape=(number of atom components,)),
///    stores information about individual atom components of molecules,
///    including species_index, initial site, initial molecule component index
/// - `Mol` also store indices of their atom components in the `species` list
class OccLocation {
 public:
  typedef Index size_type;

  OccLocation(const Conversions &_convert,
              const OccCandidateList &_candidate_list,
              bool _update_species = false);

  /// Fill tables with occupation info
  void initialize(Eigen::VectorXi const &occupation);

  /// Stochastically choose an occupant of a particular OccCandidate type
  Mol const &choose_mol(Index cand_index, MTRand &mtrand) const;

  /// Stochastically choose an occupant of a particular OccCandidate type
  Mol const &choose_mol(OccCandidate const &cand, MTRand &mtrand) const;

  /// Update occupation vector and this to reflect that event 'e' occurred
  void apply(OccEvent const &e, Eigen::VectorXi &occupation);

  /// Total number of mutating sites
  size_type mol_size() const;

  /// Access Mol by id
  Mol &mol(Index mol_id);

  /// Access Mol by id
  Mol const &mol(Index mol_id) const;

  /// Access the OccCandidateList
  OccCandidateList const &candidate_list() const;

  /// Total number of mutating sites, of OccCandidate type, specified by index
  size_type cand_size(Index cand_index) const;

  /// Total number of mutating sites, of OccCandidate type
  size_type cand_size(OccCandidate const &cand) const;

  /// Mol.id of a particular OccCandidate type
  Index mol_id(Index cand_index, Index loc) const;

  /// Mol.id of a particular OccCandidate type
  Index mol_id(OccCandidate const &cand, Index loc) const;

  /// Convert from config index to variable site index
  Index l_to_mol_id(Index l) const;

  /// Get Conversions objects
  Conversions const &convert() const;

 private:
  Conversions const &m_convert;

  OccCandidateList const &m_candidate_list;

  /// Gives a list of all Mol of the same {asym, species}-type allowed to mutate
  ///   m_loc[cand_index][i] -> m_mol index
  std::vector<std::vector<Index> > m_loc;

  /// Holds Monte::Atom objects
  std::vector<Atom> m_atoms;

  /// Holds Mol objects, one for each mutating site in the configuration
  std::vector<Mol> m_mol;

  /// l_to_mol[l] -> Mol.id, m_mol.size() otherwise
  std::vector<Index> m_l_to_mol;

  /// If true, update Atom location during apply
  bool m_update_atoms;

  /// Data structure used store temporaries during apply
  std::vector<Mol> m_tmol;
};

}  // namespace monte
}  // namespace CASM

#endif
