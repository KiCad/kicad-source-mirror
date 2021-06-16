/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _ERC_SETTINGS_H
#define _ERC_SETTINGS_H

#include <erc_item.h>
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
    ERCE_PIN_NOT_CONNECTED,       ///< Pin not connected and not no connect symbol.
    ERCE_PIN_NOT_DRIVEN,          ///< Pin connected to some others pins but no pin to drive it.
                                  ///< pins to drive it can be output, passive, 3sttae, I/O
    ERCE_POWERPIN_NOT_DRIVEN,     ///< Power input pin connected to some others pins but no
                                  ///< power out pin to drive it.
    ERCE_HIERACHICAL_LABEL,       ///< Mismatch between hierarchical labels and pins sheets.
    ERCE_NOCONNECT_CONNECTED,     ///< A no connect symbol is connected to more than 1 pin.
    ERCE_NOCONNECT_NOT_CONNECTED, ///< A no connect symbol is not connected to anything.
    ERCE_LABEL_NOT_CONNECTED,     ///< Label not connected to anything.
    ERCE_SIMILAR_LABELS,          ///< 2 labels are equal for case insensitive comparisons.
    ERCE_DIFFERENT_UNIT_FP,       ///< Different units of the same symbol have different
                                  ///< footprints assigned.
    ERCE_DIFFERENT_UNIT_NET,      ///< Shared pin in a multi-unit symbol is connected to
                                  ///< more than one net.
    ERCE_BUS_ALIAS_CONFLICT,      ///< Conflicting bus alias definitions across sheets.
    ERCE_DRIVER_CONFLICT,         ///< Conflicting drivers (labels, etc) on a subgraph.
    ERCE_BUS_ENTRY_CONFLICT,      ///< A wire connected to a bus doesn't match the bus.
    ERCE_BUS_LABEL_ERROR,         ///< A label attached to a bus isn't in bus format.
    ERCE_BUS_TO_BUS_CONFLICT,     ///< A connection between bus objects doesn't share at least
                                  ///< one net.
    ERCE_BUS_TO_NET_CONFLICT,     ///< A bus wire is graphically connected to a net port/pin
                                  ///< (or vice versa).
    ERCE_GLOBLABEL,               ///< A global label is unique.
    ERCE_UNRESOLVED_VARIABLE,     ///< A text variable could not be resolved.
    ERCE_WIRE_DANGLING,           ///< Some wires are not connected to anything else.
    ERCE_LIB_SYMBOL_ISSUES,       ///< Library symbol changed from current symbol in schematic or
                                  ///< the library symbol link no longer valid.
    ERCE_UNANNOTATED,             ///< Symbol has not been annotated.
    ERCE_EXTRA_UNITS,             ///< Symbol has more units than are defined.
    ERCE_DIFFERENT_UNIT_VALUE,    ///< Units of same symbol have different values.
    ERCE_DUPLICATE_REFERENCE,     ///< More than one symbol with the same reference.
    ERCE_BUS_ENTRY_NEEDED,        ///< Importer failed to auto-place a bus entry.

    ERCE_LAST = ERCE_BUS_ENTRY_NEEDED,

    // Errors after this point will not automatically appear in the Severities Panel

    ERCE_PIN_TO_PIN_WARNING,    // pin connected to an other pin: warning level
    ERCE_PIN_TO_PIN_ERROR,      // pin connected to an other pin: error level
    ERCE_ANNOTATION_ACTION      // Not actually an error; just an action performed during
                                // annotation which is passed back through the error handler.
};

/// The values a pin-to-pin entry in the pin matrix can take on
enum class PIN_ERROR
{
    OK,
    WARNING,
    PP_ERROR,
    UNCONNECTED
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
        return ( other.m_Severities == m_Severities );
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

    std::map<int, SEVERITY> m_Severities;
    std::set<wxString>      m_ErcExclusions;

    PIN_ERROR m_PinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

    static int m_PinMinDrive[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

private:

    static PIN_ERROR m_defaultPinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];
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

    void SetSeverities( int aSeverities ) override;

    int GetCount( int aSeverity = -1 ) const override;

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override;

    std::shared_ptr<ERC_ITEM> GetERCItem( int aIndex ) const;

    void DeleteItem( int aIndex, bool aDeep ) override;

    void DeleteAllItems( bool aIncludeExclusions, bool aDeep ) override;

private:

    void visitMarkers( std::function<void( SCH_MARKER* )> aVisitor ) const;
};


#endif // _ERC_SETTINGS_H
