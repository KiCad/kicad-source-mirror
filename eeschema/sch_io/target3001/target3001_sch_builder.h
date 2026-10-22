/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TARGET3001_SCH_BUILDER_H
#define TARGET3001_SCH_BUILDER_H

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include <math/box2.h>
#include <math/vector2d.h>

#include <io/target3001/target3001_fonts.h>
#include <io/target3001/target3001_parser.h>

class LIB_SYMBOL;
class REPORTER;
class SCH_ITEM;
class SCH_LINE;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_TEXT;
class SCHEMATIC;


/**
 * Builds KiCad schematic sheets from a parsed TARGET 3001! project.
 *
 * Gate graphics are stored already placed on the sheet, so each component becomes a library
 * symbol whose units hold its gates exactly as drawn, and each gate is placed unrotated at its
 * stored origin.
 */
class SCH_BITMAP;

class TARGET3001_SCH_BUILDER
{
public:
    TARGET3001_SCH_BUILDER( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const TARGET3001::PROJECT& aProject,
                            const wxString& aFileName, REPORTER* aReporter );

    void Build();

private:
    /// Placed pin of a signal
    struct PIN_END
    {
        int      sheet;
        VECTOR2I pos;
        float    direction;   ///< file direction, pointing from the end to the body
        bool     power;       ///< pin of a supply symbol, which names its net globally
    };

    /// Sheet number of a schematic element, or 0 for board elements
    static int sheetOf( const TARGET3001::ELEMENT& aElem );

    /// Screen for a 1-based sheet number, created on first use
    SCH_SCREEN* screen( int aSheet );

    void addComponent( const TARGET3001::ELEMENT& aComponent );
    void addConnections( const TARGET3001::ELEMENT& aSignal );
    void addFreeItem( const TARGET3001::ELEMENT& aElem );
    void addFreeSymbol( const TARGET3001::ELEMENT& aSymbol );

    /**
     * Draw each gate's body as one unit of @a aLib; @a aFieldTexts leaves reference and value texts to fields.
     * Pins take the number of the package pad they connect to from @a aPadNumbers, keyed by pin number.
     */
    void addGateBodies( LIB_SYMBOL* aLib, const std::vector<const TARGET3001::ELEMENT*>& aGates, bool aFieldTexts,
                        const std::map<uint16_t, wxString>& aPadNumbers = {} ) const;

    /// Add graphics shared by free items and symbol bodies; returns nullptr for unsupported items
    SCH_ITEM* makeGraphic( const TARGET3001::ELEMENT& aElem, const VECTOR2I& aOrigin, bool aInSymbol ) const;

    /// Image a rectangle shows instead of its fill, or nullptr when it has none
    SCH_BITMAP* makePicture( const TARGET3001::ELEMENT& aElem ) const;

    void addToSheet( int aSheet, SCH_ITEM* aItem );

    /// Add a wire, junction or label of signal @a aSignal; separateSignals() keeps it apart from other signals
    void addNetItem( int aSheet, SCH_ITEM* aItem, int aSignal );

    /**
     * Move apart items of different signals that touch where KiCad would join them, since the file connects
     * items by signal alone. Wire ends also leave buses, taking a label so the bus still joins its members.
     */
    void separateSignals( int aSheet );

    /**
     * Bend the wires of @a aSignal that pass through @a aPos around it, one gap aside on a side @a aIsFree accepts,
     * adding the new pieces to @a aWires. Returns the offset of the bent run at @a aPos, or nothing when the wires
     * are too short or not parallel.
     */
    std::optional<VECTOR2I> bendAround( int aSheet, int aSignal, const VECTOR2I& aPos,
                                        std::vector<std::pair<SCH_LINE*, int>>&       aWires,
                                        const std::function<bool( const VECTOR2I& )>& aIsFree );

    /// True when the signal's labels must be global: it reaches several sheets or meets supply symbols
    bool joinsGlobally( const TARGET3001::ELEMENT& aSignal ) const;

    /// Point on the signal's own wires or pins nearest a name drawn at @a aPos, if it is close enough to name them
    std::optional<VECTOR2I> labelAnchor( const TARGET3001::ELEMENT& aSignal, int aSheet, const VECTOR2I& aPos ) const;

    /// Join the pieces of every signal as separateSignals() left them; see joinPieces()
    void joinSignalPieces();

    /**
     * Join the pieces of signal @a aSignal that KiCad would see apart, since the file joins a signal's items by
     * identity. @a aItems holds the signal's net items by sheet. Ends a hair apart get a stub wire, and each piece
     * holding a pin but no name gets a label.
     */
    void joinPieces( int aSignal, const std::map<int, std::vector<SCH_ITEM*>>& aItems );

    void finishSheets();

    /// Sheet position in KiCad units, before the per-sheet page offset is applied
    static VECTOR2I toSch( int32_t aX, int32_t aY );
    static int      toSch( int32_t aLength );

    SCHEMATIC*                 m_schematic;
    SCH_SHEET*                 m_rootSheet;
    const TARGET3001::PROJECT& m_project;
    wxString                   m_fileName;
    REPORTER*                  m_reporter;

    std::map<int, SCH_SHEET*> m_sheets;     ///< by 1-based sheet number
    std::map<int, BOX2I>      m_extents;    ///< drawn extent of each sheet
    std::map<int, std::vector<SCH_ITEM*>> m_pending;
    int                       m_powerCount = 0;
    int                       m_freeSymbolCount = 0;
    std::map<int, std::set<int>> m_powerSheets;   ///< sheets holding a power symbol, by signal ID
    std::map<int, std::vector<std::pair<SCH_ITEM*, int>>> m_netItems;  ///< items and their signal, by sheet
    std::map<int, std::vector<std::pair<VECTOR2I, int>>>  m_pinSignals; ///< pin points and signal, by sheet
    std::set<int>             m_globalSignals; ///< signals joined by global labels
    std::map<int, std::vector<PIN_END>> m_pinEnds;   ///< placed pins of each signal, by signal ID
    int                       m_unconnectedPins = 0;
    mutable TARGET3001::FONTS          m_fonts;    ///< lookup cache, filled while drawing
};

#endif // TARGET3001_SCH_BUILDER_H
