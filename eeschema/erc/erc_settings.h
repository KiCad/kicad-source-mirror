/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <erc/erc_item.h>
#include <pin_type.h>
#include <settings/nested_settings.h>
#include <widgets/ui_common.h>


class SCH_MARKER;
class SCHEMATIC;


/// ERC error codes
enum ERCE_T
{
    ERCE_UNSPECIFIED = 0,
    ERCE_FIRST,
    ERCE_DUPLICATE_SHEET_NAME = ERCE_FIRST,  ///< Duplicate sheet names within a given sheet.
    ERCE_ENDPOINT_OFF_GRID,       ///< Pin or wire-end off grid.
    ERCE_PIN_NOT_CONNECTED,       ///< Pin not connected and not no connect symbol.
    ERCE_PIN_NOT_DRIVEN,          ///< Pin connected to some others pins but no pin to drive it.
                                  ///<   pins to drive it can be output, passive, 3sttae, I/O
    ERCE_POWERPIN_NOT_DRIVEN,     ///< Power input pin connected to some others pins but no
                                  ///<   power out pin to drive it.
    ERCE_HIERACHICAL_LABEL,       ///< Mismatch between hierarchical labels and pins sheets.
    ERCE_NOCONNECT_CONNECTED,     ///< A no connect symbol is connected to more than 1 pin.
    ERCE_NOCONNECT_NOT_CONNECTED, ///< A no connect symbol is not connected to anything.
    ERCE_LABEL_NOT_CONNECTED,     ///< Label not connected to any pins.
    ERCE_SIMILAR_LABELS,          ///< 2 labels are equal for case insensitive comparisons.
    ERCE_SIMILAR_POWER,           ///< 2 power pins are equal for case insensitive comparisons.
    ERCE_SIMILAR_LABEL_AND_POWER, ///< label and pin are equal for case insensitive comparisons.
    ERCE_SINGLE_GLOBAL_LABEL,            ///< A label only exists once in the schematic.
    ERCE_SAME_LOCAL_GLOBAL_LABEL, ///< 2 labels are equal for case insensitive comparisons.
    ERCE_DIFFERENT_UNIT_FP,       ///< Different units of the same symbol have different
                                  ///<   footprints assigned.
    ERCE_MISSING_POWER_INPUT_PIN, ///< Symbol has power input pins that are not placed on the
                                  ///<   schematic
    ERCE_MISSING_INPUT_PIN,       ///< Symbol has input pins that are not placed
    ERCE_MISSING_BIDI_PIN,        ///< Symbol has bi-directional pins that are not placed
    ERCE_MISSING_UNIT,            ///< Symbol has units that are not placed on the schematic
    ERCE_DIFFERENT_UNIT_NET,      ///< Shared pin in a multi-unit symbol is connected to
                                  ///<   more than one net.
    ERCE_BUS_ALIAS_CONFLICT,      ///< Conflicting bus alias definitions across sheets.
    ERCE_DRIVER_CONFLICT,         ///< Conflicting drivers (labels, etc) on a subgraph.
    ERCE_BUS_ENTRY_CONFLICT,      ///< A wire connected to a bus doesn't match the bus.
    ERCE_BUS_TO_BUS_CONFLICT,     ///< A connection between bus objects doesn't share at least
                                  ///<   one net.
    ERCE_BUS_TO_NET_CONFLICT,     ///< A bus wire is graphically connected to a net port/pin
                                  ///<   (or vice versa).
    ERCE_GROUND_PIN_NOT_GROUND,   ///< A ground-labeled pin is not on a ground net while another pin is.
    ERCE_LABEL_SINGLE_PIN,        ///< A label is connected only to a single pin
    ERCE_UNRESOLVED_VARIABLE,     ///< A text variable could not be resolved.
    ERCE_UNDEFINED_NETCLASS,      ///< A netclass was referenced but not defined.
    ERCE_SIMULATION_MODEL,        ///< An error was found in the simulation model.
    ERCE_WIRE_DANGLING,           ///< Some wires are not connected to anything else.
    ERCE_LIB_SYMBOL_ISSUES,       ///< Symbol not found in active libraries.
    ERCE_LIB_SYMBOL_MISMATCH,     ///< Symbol doesn't match copy in library.
    ERCE_FOOTPRINT_LINK_ISSUES,   ///< The footprint link is invalid, or points to a missing
                                  ///<   (or inactive) footprint or library.
    ERCE_FOOTPRINT_FILTERS,       ///< The assigned footprint doesn't match the footprint filters
    ERCE_UNANNOTATED,             ///< Symbol has not been annotated.
    ERCE_EXTRA_UNITS,             ///< Symbol has more units than are defined.
    ERCE_DIFFERENT_UNIT_VALUE,    ///< Units of same symbol have different values.
    ERCE_DUPLICATE_REFERENCE,     ///< More than one symbol with the same reference.
    ERCE_BUS_ENTRY_NEEDED,        ///< Importer failed to auto-place a bus entry.
    ERCE_FOUR_WAY_JUNCTION,       ///< A four-way junction was found.
    ERCE_LABEL_MULTIPLE_WIRES,    ///< A label is connected to more than one wire.
    ERCE_UNCONNECTED_WIRE_ENDPOINT, ///< A label is connected to more than one wire.
    ERCE_STACKED_PIN_SYNTAX,      ///< Pin name resembles stacked pin notation.

    ERCE_LAST = ERCE_STACKED_PIN_SYNTAX,

    ERCE_DUPLICATE_PIN_ERROR,
    ERCE_PIN_TO_PIN_WARNING,    // pin connected to an other pin: warning level
    ERCE_PIN_TO_PIN_ERROR,      // pin connected to an other pin: error level
    ERCE_ANNOTATION_ACTION,     // Not actually an error; just an action performed during
                                // annotation which is passed back through the error handler.
    ERCE_GENERIC_WARNING,
    ERCE_GENERIC_ERROR
};

/// The values a pin-to-pin entry in the pin matrix can take on
enum class PIN_ERROR
{
    OK,
    WARNING,
    PP_ERROR,
    UNCONNECTED
};

/// The sorting metric used for erc resolution of multi-pin errors.
enum class ERC_PIN_SORTING_METRIC
{
    SM_HEURISTICS,
    SM_VIOLATION_COUNT
};

/// Types of drive on a net (used for legacy ERC)
#define NPI    4  // Net with Pin isolated, this pin has type Not Connected and must be left N.C.
#define DRV    3  // Net driven by a signal (a pin output for instance)
#define NET_NC 2  // Net "connected" to a "NoConnect symbol"
#define NOD    1  // Net not driven ( Such as 2 or more connected inputs )
#define NOC    0  // initial state of a net: no connection

/**
 * Container for ERC settings
 *
 * Currently only stores flags about checks to run, but could later be expanded
 * to contain the matrix of electrical pin types.
 */
class ERC_SETTINGS : public NESTED_SETTINGS
{
public:
    ERC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~ERC_SETTINGS();

    bool operator==( const ERC_SETTINGS& other ) const
    {
        return ( other.m_ERCSeverities == m_ERCSeverities );
    }

    bool operator!=( const ERC_SETTINGS& other ) const
    {
        return !( other == *this );
    }

    bool IsTestEnabled( int aErrorCode ) const
    {
        return GetSeverity( aErrorCode ) != RPT_SEVERITY_IGNORE;
    }

    SEVERITY GetSeverity( int aErrorCode ) const;

    void SetSeverity( int aErrorCode, SEVERITY aSeverity );

    void ResetPinMap();

    /**
     * Get the weight for an electrical pin type.
     * Used for sorting of pins in pin-to-pin erc resolution.
     * @param aPinType the pin type to check
     * @returns the weight for sorting
     */
    int GetPinTypeWeight( ELECTRICAL_PINTYPE aPinType ) const
    {
        wxASSERT( static_cast<int>( aPinType ) < ELECTRICAL_PINTYPES_TOTAL );
        return m_PinTypeWeights.at( aPinType );
    }

    /**
     * Get the type of sorting metric the ERC checker should
     * use to resolve multi-pin errors.
     */
    ERC_PIN_SORTING_METRIC GetERCSortingMetric() const { return m_ERCSortingMetric; }

    PIN_ERROR GetPinMapValue( int aFirstType, int aSecondType ) const
    {
        wxASSERT( aFirstType < ELECTRICAL_PINTYPES_TOTAL
                  && aSecondType < ELECTRICAL_PINTYPES_TOTAL );
        return m_PinMap[aFirstType][aSecondType];
    }

    PIN_ERROR GetPinMapValue( ELECTRICAL_PINTYPE aFirstType, ELECTRICAL_PINTYPE aSecondType ) const
    {
        return m_PinMap[static_cast<int>( aFirstType )][static_cast<int>( aSecondType )];
    }

    void SetPinMapValue( int aFirstType, int aSecondType, PIN_ERROR aValue )
    {
        wxASSERT( aFirstType < ELECTRICAL_PINTYPES_TOTAL
                  && aSecondType < ELECTRICAL_PINTYPES_TOTAL );
        m_PinMap[aFirstType][aSecondType] = aValue;
    }

    void SetPinMapValue( ELECTRICAL_PINTYPE aFirstType, ELECTRICAL_PINTYPE aSecondType,
                         PIN_ERROR aValue )
    {
        m_PinMap[static_cast<int>( aFirstType )][static_cast<int>( aSecondType )] = aValue;
    }

    int GetPinMinDrive( ELECTRICAL_PINTYPE aFirstType, ELECTRICAL_PINTYPE aSecondType ) const
    {
        return m_PinMinDrive[static_cast<int>( aFirstType )][static_cast<int>( aSecondType )];
    }

public:

    std::map<int, SEVERITY>      m_ERCSeverities;
    std::set<wxString>           m_ErcExclusions;           // Serialized excluded ERC markers
    std::map<wxString, wxString> m_ErcExclusionComments;    // Map from serialization to comment

    PIN_ERROR m_PinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

    static int m_PinMinDrive[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

private:

    static PIN_ERROR m_defaultPinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

    /**
     * Weights for electrical pins used in ERC
     * to decide which pin gets the marker
     * in case of a multi-pin erc pin-to-pin error.
     */
    std::map<ELECTRICAL_PINTYPE, int> m_PinTypeWeights;

    /**
     * The type of sorting used by the ERC checker
     * to resolve multi-pin errors.
     */
    ERC_PIN_SORTING_METRIC m_ERCSortingMetric;
};


/**
 * An implementation of the RC_ITEM_LIST interface which uses the global SHEETLIST
 * to fulfill the contract.
 */
class SHEETLIST_ERC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
private:
    SCHEMATIC*               m_schematic;
    int                      m_severities;
    std::vector<SCH_MARKER*> m_filteredMarkers;

public:
    SHEETLIST_ERC_ITEMS_PROVIDER( SCHEMATIC* aSchematic ) :
            m_schematic( aSchematic ),
            m_severities( 0 )
    { }

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    SHEETLIST_ERC_ITEMS_PROVIDER( const SHEETLIST_ERC_ITEMS_PROVIDER& ) = delete;
    SHEETLIST_ERC_ITEMS_PROVIDER& operator=( const SHEETLIST_ERC_ITEMS_PROVIDER& ) = delete;

    void SetSeverities( int aSeverities ) override;

    int GetSeverities() const override;

    int GetCount( int aSeverity = -1 ) const override;

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override;

    std::shared_ptr<ERC_ITEM> GetERCItem( int aIndex ) const;

    void DeleteItem( int aIndex, bool aDeep ) override;

private:

    void visitMarkers( std::function<void( SCH_MARKER* )> aVisitor ) const;
};

