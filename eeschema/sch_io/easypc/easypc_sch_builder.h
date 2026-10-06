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

/**
 * @file easypc_sch_builder.h
 * @brief Builds KiCad sheets from Easy-PC / DesignSpark schematic designs.
 *
 * Connectivity comes from the design's node graph, never from geometry: every node becomes a wire end, pin,
 * junction or label at its stored position.  User-defined net names join across a project's sheets without regard
 * to case; automatic names stay on their sheet.
 */

#ifndef EASYPC_SCH_BUILDER_H
#define EASYPC_SCH_BUILDER_H

#include <map>
#include <set>
#include <vector>

#include <math/vector2d.h>
#include <wx/string.h>

class REPORTER;
class SCHEMATIC;
class SCH_LABEL_BASE;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SYMBOL;

namespace EASYPC
{

struct SOURCE_BUS;
struct SOURCE_COMPONENT;
struct SOURCE_COMPONENT_INSTANCE;
struct SOURCE_DESIGN;
struct SOURCE_NODE;
struct SOURCE_SHAPE;
struct SOURCE_SYMBOL;
struct SOURCE_SYMBOL_INSTANCE;
struct DESIGN_DOCUMENT;
struct POINT32;
struct SCH_FRAME;


/// True unless aName is an automatic net name (N or n and digits, an optional !sheet! suffix)
bool IsNamedNet( const wxString& aName );


/// Project-wide net naming: a user name takes the spelling of its first occurrence
class SCH_NET_NAMES
{
public:
    void AddSheet( const DESIGN_DOCUMENT& aDoc );

    wxString Name( const wxString& aStoredName ) const;

    /// A user name used on more than one sheet, which is one net across the project
    bool Shared( const wxString& aName ) const;

private:
    std::map<wxString, wxString> m_canonical; ///< upper-cased name to first spelling
    std::map<wxString, int>      m_sheetCount;
};


class SCH_BUILDER
{
public:
    SCH_BUILDER( SCHEMATIC* aSchematic, const SCH_NET_NAMES& aNames, REPORTER* aReporter );

    void BuildSheet( const DESIGN_DOCUMENT& aDoc, SCH_SHEET* aSheet, const SCH_SHEET_PATH& aSheetPath );

    /// Symbols of every sheet, for gates placed on a sheet other than the one being built
    void SetProjectSymbols( const std::vector<const DESIGN_DOCUMENT*>& aDocs );

private:
    VECTOR2I toSheet( int32_t aX, int32_t aY ) const;
    VECTOR2I toSheet( const POINT32& aPoint ) const;

    void computeOffset( const SOURCE_DESIGN& aDesign );
    void buildSymbols( const SOURCE_DESIGN& aDesign, const SCH_SHEET_PATH& aSheetPath );
    void buildWires( const SOURCE_DESIGN& aDesign );
    void buildLabels( const SOURCE_DESIGN& aDesign );
    void buildCommonPinCopies( const SOURCE_DESIGN& aDesign );
    void buildBuses( const SOURCE_DESIGN& aDesign );
    void addBusAlias( const SOURCE_BUS& aBus, const SOURCE_SHAPE& aShape );
    void placePinItems( const SOURCE_COMPONENT& aPart, const SOURCE_SYMBOL_INSTANCE& aInst, const SCH_FRAME& aFrame );
    void placeFields( SCH_SYMBOL& aSymbol, const SOURCE_COMPONENT_INSTANCE& aComp, const SOURCE_SYMBOL_INSTANCE& aInst,
                      bool aPower, const SCH_FRAME& aFrame );

    /// A local label, global when the name crosses sheets or a global power symbol names it
    SCH_LABEL_BASE* addLabel( const wxString& aName, const VECTOR2I& aAt );

    SCHEMATIC*           m_schematic;
    const SCH_NET_NAMES& m_names;
    REPORTER*            m_reporter;
    SCH_SCREEN*          m_screen = nullptr;
    const SOURCE_DESIGN* m_design = nullptr;

    int32_t m_originX = 0; ///< design point at the sheet's top left corner
    int32_t m_originY = 0;

    std::map<wxString, const SOURCE_SYMBOL*> m_projectSymbols; ///< by name, first sheet wins
    std::set<VECTOR2I>                       m_sharedPoints;   ///< points where connections of different nets meet
    std::set<wxString>                       m_powerNets;      ///< upper-cased nets a power symbol names
    std::set<const SOURCE_NODE*>             m_powerNodes;
    std::set<wxString>                       m_labelledNets; ///< upper-cased nets a label names
    bool                                     m_keepUpright = false;
    int                                      m_powerCount = 0;
};

} // namespace EASYPC

#endif // EASYPC_SCH_BUILDER_H
