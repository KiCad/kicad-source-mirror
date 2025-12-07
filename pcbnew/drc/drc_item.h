/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DRC_ITEM_H
#define DRC_ITEM_H

#include <rc_item.h>
#include <marker_base.h>

class PCB_BASE_FRAME;
class DRC_RULE;
class DRC_TEST_PROVIDER;
class PCB_MARKER;
class BOARD;

enum PCB_DRC_CODE
{
    DRCE_FIRST = 1,
    DRCE_UNCONNECTED_ITEMS = DRCE_FIRST, // items are unconnected
    DRCE_SHORTING_ITEMS,                 // items short two nets but are not a net-tie
    DRCE_ALLOWED_ITEMS,                  // a disallowed item has been used
    DRCE_TEXT_ON_EDGECUTS,               // text or dimension on Edge.Cuts layer
    DRCE_CLEARANCE,                      // items are too close together
    DRCE_CREEPAGE,                       // items are too close together ( creepage )
    DRCE_TRACKS_CROSSING,                // tracks are crossing
    DRCE_EDGE_CLEARANCE,                 // a copper item is too close to the board edge
    DRCE_ZONES_INTERSECT,                // copper zone outlines intersect
    DRCE_ISOLATED_COPPER,                // copper fill with no electrical connections
    DRCE_STARVED_THERMAL,                // insufficient number of thermal spokes connected to zone
    DRCE_DANGLING_VIA,                   // via which isn't connected to anything
    DRCE_DANGLING_TRACK,                 // track with at least one end not connected to anything
    DRCE_DRILLED_HOLES_TOO_CLOSE,        // overlapping drilled holes break drill bits
    DRCE_DRILLED_HOLES_COLOCATED,        // two holes at the same location
    DRCE_HOLE_CLEARANCE,                 //
    DRCE_TRACK_WIDTH,                    // Track width is too small or too large
    DRCE_TRACK_ANGLE,                    // Angle between two connected tracks is too small or too large
    DRCE_TRACK_SEGMENT_LENGTH,           // Track segment is too short or too long
    DRCE_ANNULAR_WIDTH,                  // Via size and drill leave annular ring too small
    DRCE_CONNECTION_WIDTH,               // Net connection too small
    DRCE_DRILL_OUT_OF_RANGE,             // Too small via or pad drill
    DRCE_VIA_DIAMETER,                   // Via diameter checks (min/max)
    DRCE_PADSTACK,                       // something is questionable with a pad or via stackup
    DRCE_PADSTACK_INVALID,               // something is invalid with a pad or via stackup
    DRCE_MICROVIA_DRILL_OUT_OF_RANGE,    // Too small micro via drill
    DRCE_OVERLAPPING_FOOTPRINTS,         // footprint courtyards overlap
    DRCE_MISSING_COURTYARD,              // footprint has no courtyard defined
    DRCE_MALFORMED_COURTYARD,            // footprint has a courtyard but malformed
                                         // (not convertible to a closed polygon with holes)
    DRCE_PTH_IN_COURTYARD,
    DRCE_NPTH_IN_COURTYARD,
    DRCE_DISABLED_LAYER_ITEM, // item on a disabled layer
    DRCE_INVALID_OUTLINE,     // invalid board outline

    DRCE_MISSING_FOOTPRINT,   // footprint not found for netlist item
    DRCE_DUPLICATE_FOOTPRINT, // more than one footprints found for netlist item
    DRCE_EXTRA_FOOTPRINT,     // netlist item not found for footprint
    DRCE_NET_CONFLICT,        // pad net doesn't match netlist
    DRCE_SCHEMATIC_PARITY,    // footprint attributes don't match symbol attributes
    DRCE_FOOTPRINT_FILTERS,   // footprint doesn't match symbol's footprint filters

    DRCE_FOOTPRINT_TYPE_MISMATCH, // footprint attribute does not match actual pads
    DRCE_LIB_FOOTPRINT_ISSUES,    // footprint not found in active libraries
    DRCE_LIB_FOOTPRINT_MISMATCH,  // footprint does not match the current library
    DRCE_PAD_TH_WITH_NO_HOLE,     // footprint has Plated Through-Hole with no hole
    DRCE_FOOTPRINT,               // error in footprint definition

    DRCE_UNRESOLVED_VARIABLE,
    DRCE_ASSERTION_FAILURE,       // user-defined (custom rule) assertion
    DRCE_GENERIC_WARNING,         // generic warning
    DRCE_GENERIC_ERROR,           // generic error

    DRCE_COPPER_SLIVER,
    DRCE_SOLDERMASK_BRIDGE,       // failure to maintain min soldermask web thickness
                                  //   between copper items with different nets

    DRCE_SILK_MASK_CLEARANCE,     // silkscreen clipped by mask (potentially leaving it
                                  //   over pads, exposed copper, etc.)
    DRCE_SILK_EDGE_CLEARANCE,
    DRCE_SILK_CLEARANCE,          // silk-to-silk or silk-to-other clearance error
    DRCE_TEXT_HEIGHT,
    DRCE_TEXT_THICKNESS,

    DRCE_LENGTH_OUT_OF_RANGE,
    DRCE_SKEW_OUT_OF_RANGE,
    DRCE_VIA_COUNT_OUT_OF_RANGE,
    DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE,
    DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG,

    DRCE_MIRRORED_TEXT_ON_FRONT_LAYER,
    DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER,

    DRCE_MISSING_TUNING_PROFILE,        // Tuning profile used in net class is not defined
    DRCE_TUNING_PROFILE_IMPLICIT_RULES, // Pseudo-code for setting severities

    DRCE_TRACK_ON_POST_MACHINED_LAYER,  // Track connected to pad/via on post-machined/backdrilled layer

    DRCE_SCHEMATIC_FIELDS_PARITY, // Mismatch with schematic fields

    DRCE_LAST = DRCE_SCHEMATIC_FIELDS_PARITY
};


class DRC_ITEM : public RC_ITEM
{
public:
    /**
     * Constructs a DRC_ITEM for the given error code
     * @see DRCE_T
     */
    static std::shared_ptr<DRC_ITEM> Create( int aErrorCode );

    /**
     * Constructs a DRC item from a given error settings key
     * @param aErrorKey is a settings key for an error code (the untranslated string that is used
     * to represent a given error code in settings files and for storing ignored DRC items)
     * @return the created item
     */
    static std::shared_ptr<DRC_ITEM> Create( const wxString& aErrorKey );

    static std::vector<std::reference_wrapper<RC_ITEM>> GetItemsWithSeverities()
    {
        static std::vector<std::reference_wrapper<RC_ITEM>> itemsWithSeveritiesAll;

        if( itemsWithSeveritiesAll.empty() )
        {
            for( RC_ITEM& item : allItemTypes )
            {
                if( &item == &heading_internal )
                    break;

                itemsWithSeveritiesAll.push_back( item );
            }
        }

        return itemsWithSeveritiesAll;
    }

    void SetViolatingRule ( DRC_RULE *aRule ) { m_violatingRule = aRule; }
    DRC_RULE* GetViolatingRule() const { return m_violatingRule; }

    wxString GetViolatingRuleDesc( bool aTranslate ) const override;

    void SetViolatingTest( DRC_TEST_PROVIDER *aProvider ) { m_violatingTest = aProvider; }
    DRC_TEST_PROVIDER* GetViolatingTest() const { return m_violatingTest; }

    KIID GetAuxItem2ID() const override;
    KIID GetAuxItem3ID() const override;

private:
    DRC_ITEM( int aErrorCode = 0, const wxString& aTitle = "", const wxString& aSettingsKey = "" )
    {
        m_errorCode   = aErrorCode;
        m_errorTitle  = aTitle;
        m_settingsKey = aSettingsKey;
        m_parent      = nullptr;
    }

    /// A list of all DRC_ITEM types which are valid error codes
    static std::vector<std::reference_wrapper<RC_ITEM>> allItemTypes;

    static DRC_ITEM heading_electrical;
    static DRC_ITEM heading_DFM;
    static DRC_ITEM heading_schematic_parity;
    static DRC_ITEM heading_signal_integrity;
    static DRC_ITEM heading_readability;
    static DRC_ITEM heading_misc;
    static DRC_ITEM heading_internal;
    static DRC_ITEM heading_deprecated;

    static DRC_ITEM unconnectedItems;
    static DRC_ITEM shortingItems;
    static DRC_ITEM itemsNotAllowed;
    static DRC_ITEM textOnEdgeCuts;
    static DRC_ITEM clearance;
    static DRC_ITEM creepage;
    static DRC_ITEM tracksCrossing;
    static DRC_ITEM edgeClearance;
    static DRC_ITEM zonesIntersect;
    static DRC_ITEM isolatedCopper;
    static DRC_ITEM starvedThermal;
    static DRC_ITEM viaDangling;
    static DRC_ITEM trackDangling;
    static DRC_ITEM holeNearHole;
    static DRC_ITEM holesCoLocated;
    static DRC_ITEM holeClearance;
    static DRC_ITEM connectionWidth;
    static DRC_ITEM trackWidth;
    static DRC_ITEM trackAngle;
    static DRC_ITEM trackSegmentLength;
    static DRC_ITEM annularWidth;
    static DRC_ITEM drillTooSmall;
    static DRC_ITEM viaDiameter;
    static DRC_ITEM padstack;
    static DRC_ITEM padstackInvalid;
    static DRC_ITEM microviaDrillTooSmall;
    static DRC_ITEM courtyardsOverlap;
    static DRC_ITEM missingCourtyard;
    static DRC_ITEM malformedCourtyard;
    static DRC_ITEM pthInsideCourtyard;
    static DRC_ITEM npthInsideCourtyard;
    static DRC_ITEM itemOnDisabledLayer;
    static DRC_ITEM invalidOutline;
    static DRC_ITEM duplicateFootprints;
    static DRC_ITEM missingFootprint;
    static DRC_ITEM extraFootprint;
    static DRC_ITEM netConflict;
    static DRC_ITEM schematicParity;
    static DRC_ITEM footprintFilters;
    static DRC_ITEM libFootprintIssues;
    static DRC_ITEM libFootprintMismatch;
    static DRC_ITEM unresolvedVariable;
    static DRC_ITEM assertionFailure;
    static DRC_ITEM genericWarning;
    static DRC_ITEM genericError;
    static DRC_ITEM copperSliver;
    static DRC_ITEM silkMaskClearance;
    static DRC_ITEM silkEdgeClearance;
    static DRC_ITEM solderMaskBridge;
    static DRC_ITEM silkClearance;
    static DRC_ITEM textHeightOutOfRange;
    static DRC_ITEM textThicknessOutOfRange;
    static DRC_ITEM lengthOutOfRange;
    static DRC_ITEM skewOutOfRange;
    static DRC_ITEM viaCountOutOfRange;
    static DRC_ITEM diffPairGapOutOfRange;
    static DRC_ITEM diffPairUncoupledLengthTooLong;
    static DRC_ITEM footprint;
    static DRC_ITEM footprintTypeMismatch;
    static DRC_ITEM footprintTHPadhasNoHole;
    static DRC_ITEM mirroredTextOnFrontLayer;
    static DRC_ITEM nonMirroredTextOnBackLayer;
    static DRC_ITEM missingTuningProfile;
    static DRC_ITEM tuningProfileImplicitRules;
    static DRC_ITEM trackOnPostMachinedLayer;
    static DRC_ITEM schematicFieldsParity;

private:
    DRC_RULE*          m_violatingRule = nullptr;
    DRC_TEST_PROVIDER* m_violatingTest = nullptr;
};


class DRC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
public:
    DRC_ITEMS_PROVIDER( BOARD* aBoard, MARKER_BASE::MARKER_T aMarkerType,
                        MARKER_BASE::MARKER_T otherMarkerType = MARKER_BASE::MARKER_UNSPEC ) :
            m_board( aBoard ),
            m_severities( 0 )
    {
        m_markerTypes.push_back( aMarkerType );

        if( otherMarkerType != MARKER_BASE::MARKER_UNSPEC )
            m_markerTypes.push_back( otherMarkerType );
    }

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    DRC_ITEMS_PROVIDER( const DRC_ITEMS_PROVIDER& ) = delete;
    DRC_ITEMS_PROVIDER& operator=( const DRC_ITEMS_PROVIDER& ) = delete;

    void SetSeverities( int aSeverities ) override;

    int GetSeverities() const override;

    int GetCount( int aSeverity = -1 ) const override;

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override;

    void DeleteItem( int aIndex, bool aDeep ) override;

private:
    BOARD*                             m_board;
    std::vector<MARKER_BASE::MARKER_T> m_markerTypes;

    int                                m_severities;
    std::vector<PCB_MARKER*>           m_filteredMarkers;
};


#endif      // DRC_ITEM_H
