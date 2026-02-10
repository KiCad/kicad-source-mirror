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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_SCH_SCHEMATIC_BUILDER_H_
#define PADS_SCH_SCHEMATIC_BUILDER_H_

#include <sch_io/pads/pads_sch_parser.h>
#include <sch_label.h>
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
 * Builder class to create KiCad schematic elements from parsed PADS data.
 *
 * This class handles the conversion of parsed PADS signals and connectivity
 * into KiCad schematic wire segments, junctions, and labels.
 */
class PADS_SCH_SCHEMATIC_BUILDER
{
public:
    PADS_SCH_SCHEMATIC_BUILDER( const PARAMETERS& aParams, SCHEMATIC* aSchematic );
    ~PADS_SCH_SCHEMATIC_BUILDER();

    /**
     * Create wire segments from signal definitions and add to screen.
     *
     * @param aSignals Vector of parsed signal definitions.
     * @param aScreen Target screen to add wires to.
     * @return Number of wires created.
     */
    int CreateWires( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    /**
     * Create a single wire segment.
     *
     * @param aWire Wire segment data.
     * @return New SCH_LINE object. Caller takes ownership.
     */
    SCH_LINE* CreateWire( const WIRE_SEGMENT& aWire );

    /**
     * Create junctions at wire intersection points.
     *
     * @param aSignals Vector of parsed signal definitions.
     * @param aScreen Target screen to add junctions to.
     * @return Number of junctions created.
     */
    int CreateJunctions( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    /**
     * Create net labels for named signals.
     *
     * For signals with OPC endpoints, one label is created per OPC connection point with
     * orientation derived from the adjacent wire direction. Signals without OPC endpoints
     * fall back to a single label at a dangling wire end.
     *
     * @param aSignals Vector of parsed signal definitions.
     * @param aScreen Target screen to add labels to.
     * @param aSignalOpcIds Set of OPC reference strings (e.g. "@@@O48") for signal OPCs.
     * @param aSkipSignals Signal names to skip (e.g. power nets handled elsewhere).
     * @return Number of labels created.
     */
    int CreateNetLabels( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen,
                         const std::set<std::string>& aSignalOpcIds,
                         const std::set<std::string>& aSkipSignals = {} );

    /**
     * Create a global net label for a signal.
     *
     * PADS signals are global by nature, so all net labels are created as
     * SCH_GLOBALLABEL to avoid sheet-path prefix issues with local labels.
     *
     * @param aSignal Signal definition.
     * @param aPosition Label position.
     * @param aOrientation Label spin style (default RIGHT).
     * @return New SCH_GLOBALLABEL object. Caller takes ownership.
     */
    SCH_GLOBALLABEL* CreateNetLabel( const SCH_SIGNAL& aSignal, const VECTOR2I& aPosition,
                                     SPIN_STYLE aOrientation = SPIN_STYLE::RIGHT );

    /**
     * Create bus wires and entries for bus signals.
     *
     * @param aSignals Vector of parsed signal definitions.
     * @param aScreen Target screen to add bus wires to.
     * @return Number of bus wires created.
     */
    int CreateBusWires( const std::vector<SCH_SIGNAL>& aSignals, SCH_SCREEN* aScreen );

    /**
     * Create a single bus wire segment.
     *
     * @param aWire Wire segment data.
     * @return New SCH_LINE object with bus layer. Caller takes ownership.
     */
    SCH_LINE* CreateBusWire( const WIRE_SEGMENT& aWire );

    /**
     * Check if a signal name indicates a bus.
     *
     * @param aName Signal name to check.
     * @return True if the name matches bus naming patterns.
     */
    static bool IsBusSignal( const std::string& aName );

    /**
     * Apply part attributes to a symbol instance.
     *
     * Sets the reference designator, value, footprint, and other fields from the
     * parsed PADS part placement attributes.
     *
     * @param aSymbol Target symbol instance to update.
     * @param aPlacement Parsed part placement with attributes.
     */
    void ApplyPartAttributes( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    /**
     * Apply field visibility and position from PADS attribute settings.
     *
     * @param aSymbol Target symbol instance.
     * @param aPlacement Parsed part placement with attribute visibility settings.
     */
    void ApplyFieldSettings( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    /**
     * Create custom fields from non-standard PADS attributes.
     *
     * Creates KiCad custom fields for PADS attributes that don't map to
     * standard fields (Reference, Value, Footprint, Datasheet, Description).
     * Uses PADS_ATTRIBUTE_MAPPER for name normalization.
     *
     * @param aSymbol Target symbol instance.
     * @param aPlacement Parsed part placement with attributes.
     * @return Number of custom fields created.
     */
    int CreateCustomFields( SCH_SYMBOL* aSymbol, const PART_PLACEMENT& aPlacement );

    /**
     * Create title block from parsed PADS parameters.
     *
     * Maps PADS *FIELDS* section entries to KiCad TITLE_BLOCK, checking both standard
     * and custom field names since PADS designs often leave standard names empty and
     * use custom variants instead (e.g. "TITLE1" instead of "Title").
     *
     * @param aScreen Target screen to set title block on.
     */
    void CreateTitleBlock( SCH_SCREEN* aScreen );

    /**
     * Create hierarchical sheet for a sub-schematic page.
     *
     * Creates a SCH_SHEET object representing a sub-sheet in the hierarchy.
     * The sheet is positioned on the parent sheet and linked with a screen
     * for the sub-schematic content.
     *
     * @param aSheetNumber Sheet number (1-based).
     * @param aTotalSheets Total number of sheets in the design.
     * @param aParentSheet Parent sheet to contain this sub-sheet.
     * @param aBaseFilename Base filename for generating sheet filenames.
     * @return New SCH_SHEET object. Caller takes ownership.
     */
    SCH_SHEET* CreateHierarchicalSheet( int aSheetNumber, int aTotalSheets,
                                        SCH_SHEET* aParentSheet,
                                        const wxString& aBaseFilename );

    /**
     * Get standard sheet size for a given sheet number.
     *
     * Returns a default sheet symbol size suitable for display on the parent.
     */
    VECTOR2I GetDefaultSheetSize() const;

    /**
     * Calculate position for a sheet symbol on the parent sheet.
     *
     * Arranges sheets in a grid layout.
     *
     * @param aSheetIndex Zero-based sheet index.
     * @param aTotalSheets Total number of sub-sheets.
     * @return Position for the sheet symbol.
     */
    VECTOR2I CalculateSheetPosition( int aSheetIndex, int aTotalSheets ) const;

    /**
     * Create hierarchical sheet pin on a sheet symbol.
     *
     * Sheet pins connect to hierarchical labels inside the sub-schematic.
     *
     * @param aSheet Target sheet to add pin to.
     * @param aSignalName Name of the signal crossing the hierarchy.
     * @param aPinIndex Index for positioning the pin on the sheet edge.
     * @return New SCH_SHEET_PIN object. Ownership transferred to sheet.
     */
    SCH_SHEET_PIN* CreateSheetPin( SCH_SHEET* aSheet, const std::string& aSignalName,
                                   int aPinIndex );

    /**
     * Create hierarchical label in a sub-schematic.
     *
     * Hierarchical labels connect to sheet pins on the parent sheet.
     *
     * @param aSignalName Name of the signal.
     * @param aPosition Label position.
     * @param aScreen Target screen to add label to.
     * @return New SCH_HIERLABEL object. Caller takes ownership.
     */
    SCH_HIERLABEL* CreateHierLabel( const std::string& aSignalName, const VECTOR2I& aPosition,
                                    SCH_SCREEN* aScreen );

    /**
     * Check if a signal name represents a global signal.
     *
     * Global signals include power nets (VCC, GND) and signals
     * that appear on multiple sheets.
     *
     * @param aSignalName Signal name to check.
     * @param aSheetNumbers Set of sheet numbers where signal appears.
     * @return True if this should be a global label.
     */
    static bool IsGlobalSignal( const std::string& aSignalName,
                                const std::set<int>& aSheetNumbers );

private:
    /**
     * Convert PADS coordinate to KiCad internal units.
     */
    int toKiCadUnits( double aPadsValue ) const;

    /**
     * Convert PADS Y coordinate to KiCad Y, accounting for Y-axis inversion and page offset.
     */
    int toKiCadY( double aPadsY ) const;

    /**
     * Find junction points where 3+ wire segments meet.
     */
    std::vector<VECTOR2I> findJunctionPoints( const std::vector<SCH_SIGNAL>& aSignals );

    /**
     * Choose best position for net label on a signal's wires.
     */
    VECTOR2I chooseLabelPosition( const SCH_SIGNAL& aSignal );

    /**
     * Compute label orientation from the wire direction at the label position.
     * The label extends opposite to the wire direction.
     */
    SPIN_STYLE computeLabelOrientation( const VECTOR2I& aLabelPos,
                                        const VECTOR2I& aAdjacentPos );

    /**
     * Convert a PADS net name for use as a KiCad label.
     * Handles "/" prefix → "~{}" overbar conversion for inverted signals.
     */
    wxString convertNetName( const std::string& aName ) const;

    const PARAMETERS& m_params;
    SCHEMATIC*        m_schematic;
    int               m_pageHeightIU;
};

} // namespace PADS_SCH

#endif // PADS_SCH_SCHEMATIC_BUILDER_H_
