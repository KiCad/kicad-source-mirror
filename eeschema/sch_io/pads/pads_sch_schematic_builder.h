/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PADS_SCH_SCHEMATIC_BUILDER_H_
#define PADS_SCH_SCHEMATIC_BUILDER_H_

#include <sch_io/pads/pads_sch_parser.h>
#include <sch_label.h>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <wx/string.h>

class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_HIERLABEL;
class SCH_GLOBALLABEL;
class SCH_LINE;
class SCH_JUNCTION;
class SCH_LABEL;
class SCH_BUS_WIRE_ENTRY;
class SCH_SYMBOL;
class SCHEMATIC;

namespace PADS_SCH
{

/**
 * Builds KiCad schematic elements (wires, junctions, labels, fields, sheets) from parsed
 * PADS data.
 */
class PADS_SCH_SCHEMATIC_BUILDER
{
public:
    PADS_SCH_SCHEMATIC_BUILDER( const PARAMETERS& aParams, SCHEMATIC* aSchematic );
    ~PADS_SCH_SCHEMATIC_BUILDER();

    int CreateWires( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    SCH_LINE* CreateWire( const WIRE_SEGMENT& aWire );

    int CreateJunctions( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    /**
     * Create net labels for named signals. Signals with OPC endpoints get one label per OPC
     * point oriented from the adjacent wire; others get one label at a dangling wire end.
     * aSkipSignals names are ignored (e.g. power nets handled elsewhere).
     */
    int CreateNetLabels( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen,
                         const std::set<std::string>& aSignalOpcIds,
                         const std::set<std::string>& aSkipSignals = {},
                         const std::map<std::string, NETNAME_LABEL>& aNetNameLabels = {} );

    /**
     * Map a PADS *NETNAMES* label entry to a KiCad global-label spin style.
     *
     * PADS encodes the label side via the offset of the text from the anchor
     * point. The dominant offset axis and its sign tell which way the text
     * reads away from the connection, which is exactly what the spin style
     * controls.
     */
    static SPIN_STYLE SpinFromNetNameLabel( const NETNAME_LABEL& aLabel );

    /**
     * Create a net label. PADS signals are global, so labels are SCH_GLOBALLABEL to avoid
     * sheet-path prefixing. Caller takes ownership.
     */
    SCH_GLOBALLABEL* CreateNetLabel( const SCH_SIGNAL& aSignal, const VECTOR2I& aPosition,
                                     SPIN_STYLE aOrientation = SPIN_STYLE::RIGHT );

    int CreateBusWires( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    SCH_LINE* CreateBusWire( const WIRE_SEGMENT& aWire );

    static bool IsBusSignal( const std::string& aName );

    /**
     * Set reference, value, footprint and other fields on a symbol from a part placement.
     */
    void ApplyPartAttributes( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    void ApplyFieldSettings( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    /**
     * Create KiCad user fields for PADS attributes that have no standard-field mapping.
     */
    int CreateCustomFields( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    /**
     * Set the title block from PADS fields, checking custom names too because PADS designs
     * often leave the standard names empty (e.g. "TITLE1" instead of "Title").
     */
    void CreateTitleBlock( SCH_SCREEN* aScreen );

    /**
     * Create a sub-sheet positioned on its parent and backed by its own screen. Caller takes
     * ownership.
     */
    SCH_SHEET* CreateHierarchicalSheet( int aSheetNumber, int aTotalSheets,
                                        SCH_SHEET* aParentSheet,
                                        const wxString& aBaseFilename );

    VECTOR2I GetDefaultSheetSize() const;

    /**
     * Position a sheet symbol in a roughly square grid on the parent.
     */
    VECTOR2I CalculateSheetPosition( int aSheetIndex, int aTotalSheets ) const;

    /**
     * Create a sheet pin that connects to a hierarchical label in the sub-schematic.
     * Ownership transfers to the sheet.
     */
    SCH_SHEET_PIN* CreateSheetPin( SCH_SHEET* aSheet, const std::string& aSignalName,
                                   int aPinIndex );

    /**
     * Create a hierarchical label that connects to a sheet pin on the parent. Caller takes
     * ownership.
     */
    SCH_HIERLABEL* CreateHierLabel( const std::string& aSignalName, const VECTOR2I& aPosition,
                                    SCH_SCREEN* aScreen );

    /**
     * A signal is global when it spans multiple sheets or is a common power net.
     */
    static bool IsGlobalSignal( const std::string& aSignalName,
                                const std::set<int>& aSheetNumbers );

    /**
     * Orient a label opposite to the wire direction at its position so it clears the wire.
     */
    static SPIN_STYLE computeLabelOrientation( const VECTOR2I& aLabelPos,
                                               const VECTOR2I& aAdjacentPos );

private:
    int toKiCadUnits( double aPadsValue ) const;

    /**
     * Convert PADS Y coordinate to KiCad Y, accounting for Y-axis inversion and page offset.
     */
    int toKiCadY( double aPadsY ) const;

    /**
     * Find points where 3+ wire segments meet.
     */
    std::vector<VECTOR2I> findJunctionPoints( const std::vector<SCH_SIGNAL>& aSignals );

    VECTOR2I chooseLabelPosition( const SCH_SIGNAL& aSignal );

    /**
     * Convert a PADS net name to a KiCad label, mapping a "/" prefix to a "~{}" overbar.
     */
    wxString convertNetName( const std::string& aName ) const;

    const PARAMETERS& m_params;
    SCHEMATIC*        m_schematic;
    int               m_pageHeightIU;
};

} // namespace PADS_SCH

#endif // PADS_SCH_SCHEMATIC_BUILDER_H_
