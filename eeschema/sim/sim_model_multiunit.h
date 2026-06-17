/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_MODEL_MULTIUNIT_H
#define SIM_MODEL_MULTIUNIT_H

#include <utility>
#include <vector>
#include <wx/string.h>

#include <sim/sim_model_spice.h>
#include <sim/spice_generator.h>


/**
 * One functional unit's pin map, gathered from its Sim.Pins field.
 *
 * The repetition of a model pin across units is the signal that drives
 * repeat-per-unit decomposition, so the maps are kept per unit and never
 * deduplicated against each other.
 */
struct UNIT_PIN_MAP
{
    int                                        unit = 0;
    std::vector<std::pair<wxString, wxString>> pins;   // symbolPinNumber -> modelPinName
};


/**
 * Per-component decomposition descriptor stored in the Sim.Decomposition field.
 *
 * It selects how a multi-unit symbol is turned into SPICE.  WHOLE_DEVICE is the
 * default and historic behavior (the per-unit Sim.Pins are merged into a single
 * instance of a hand-built whole-device model).  REPEAT_PER_UNIT instantiates a
 * single-unit vendor model once per functional unit, sharing the listed model
 * pins (typically the supply rails) across all instances.
 *
 * Unknown or empty fields resolve to WHOLE_DEVICE so older and newer files alike
 * keep today's safe behavior.
 */
struct SIM_DECOMPOSITION
{
    enum class MODE
    {
        WHOLE_DEVICE,
        REPEAT_PER_UNIT
    };

    MODE                  mode = MODE::WHOLE_DEVICE;
    std::vector<wxString> sharedModelPins;   // model-pin names common to all instances

    static SIM_DECOMPOSITION Parse( const wxString& aField );
    wxString                 Format() const;
};


/**
 * Parse one unit's Sim.Pins text into (symbolPinNumber -> modelPinName) pairs, preserving the
 * written order.
 *
 * @param aPins The raw Sim.Pins field value (whitespace-separated `num=name` tokens).
 * @param aRef  The owning symbol reference, used only for error messages.
 * @throws IO_ERROR on a malformed token (missing/edge `=`) or a within-unit conflict (the same
 *         symbol pin mapped to two different model pins).  Mapping different symbol pins to the
 *         same model pin within a unit is allowed.
 */
std::vector<std::pair<wxString, wxString>> ParseSimPinsTokens( const wxString& aPins,
                                                               const wxString& aRef );


class SIM_MODEL_MULTIUNIT;


/**
 * SPICE generator for the synthesized repeat-per-unit wrapper.
 *
 * ModelLine() emits the generated `.subckt` (interface = the component's mapped symbol pins, body
 * = one inner instance of the base single-unit model per functional unit, shared nodes common).
 * ModelName() returns the content-derived signature so identical components share one definition.
 */
class SPICE_GENERATOR_MULTIUNIT : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelName( const SPICE_ITEM& aItem ) const override;
    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::vector<std::string> CurrentNames( const SPICE_ITEM& aItem ) const override;

private:
    const SIM_MODEL_MULTIUNIT& multiunit() const;
};


/**
 * Wraps a resolved single-unit base model and presents it as one component-level SPICE device.
 *
 * The synthetic outer pin list (built into the model's own pin array) is the union of the
 * component's mapped symbol pins, so the existing ItemPins() machinery emits the outer
 * `X<refdes>` line.  The generator emits a matching `.subckt` whose body instantiates the base
 * model once per functional unit, with shared model pins (e.g. supply rails) wired to a common
 * node across instances.
 *
 * The constructor copies everything it needs from the base model (pin names/order and the base
 * SPICE name), so the wrapper holds no reference to it; the base only has to outlive construction.
 */
class SIM_MODEL_MULTIUNIT : public SIM_MODEL_SPICE
{
public:
    friend class SPICE_GENERATOR_MULTIUNIT;

    /**
     * @param aBaseModel       The resolved single-unit model (referenced, not owned).
     * @param aBaseModelName   The SPICE name to reference from each inner instance.
     * @param aUnitMaps        Per-unit symbol-pin -> model-pin maps (see collectUnitPinMaps()).
     * @param aSharedModelPins Model-pin names common to every instance (e.g. VCC, VEE).
     * @throws IO_ERROR on an inconsistent decomposition (unknown/unconnected shared pin, an
     *         unassigned base pin, a shared pin resolving to multiple nets, or a base model with
     *         unsupported subcircuit instance parameters).
     */
    SIM_MODEL_MULTIUNIT( const SIM_MODEL& aBaseModel, const wxString& aBaseModelName,
                         const std::vector<UNIT_PIN_MAP>& aUnitMaps,
                         const std::vector<wxString>& aSharedModelPins );

    const wxString& GetSignature() const { return m_signature; }
    int             GetInstanceCount() const { return static_cast<int>( m_instances.size() ); }

private:
    struct INSTANCE
    {
        int                   unit = 0;
        std::vector<wxString> nodes;   ///< one node per base-model pin, in base order
    };

    wxString computeSignature() const;

    wxString              m_baseModelName;   ///< name referenced by each inner instance line
    wxString              m_signature;       ///< content-derived wrapper subckt name
    std::vector<INSTANCE> m_instances;
};

#endif // SIM_MODEL_MULTIUNIT_H
