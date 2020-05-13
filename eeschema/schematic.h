/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SCHEMATIC_H
#define KICAD_SCHEMATIC_H

#include <base_struct.h>
#include <sch_sheet_path.h>


class BUS_ALIAS;
class CONNECTION_GRAPH;
class EDA_BASE_FRAME;
class ERC_SETTINGS;
class SCH_SCREEN;
class SCH_SHEET;

/**
 * Holds all the data relating to one schematic
 * A schematic may consist of one or more sheets (and one root sheet)
 * Right now, eeschema can have only one schematic open at a time, but this could change.
 * Please keep this possibility in mind when adding to this object.
 */
class SCHEMATIC : public EDA_ITEM
{
    friend class SCH_EDIT_FRAME;

private:

    /// The top-level sheet in this schematic hierarchy (or potentially the only one)
    SCH_SHEET* m_rootSheet;

    /**
     * The sheet path of the sheet currently being edited or displayed.
     * Note that this was moved here from SCH_EDIT_FRAME because currently many places in the code
     * want to know the current sheet.  Potentially this can be moved back to the UI code once
     * the only places that want to know it are UI-related
     */
    SCH_SHEET_PATH* m_currentSheet;

    /// Holds and calculates connectivity information of this schematic
    CONNECTION_GRAPH* m_connectionGraph;

    /// Holds this schematic's ERC settings
    // TODO: This should be moved to project settings, not schematic
    ERC_SETTINGS* m_ercSettings;

public:
    SCHEMATIC();

    ~SCHEMATIC();

    virtual wxString GetClass() const override
    {
        return wxT( "SCHEMATIC" );
    }

    /// Initializes this schematic to a blank one, unloading anything existing
    void Reset();

    /**
     * Builds and returns an updated schematic hierarchy
     * TODO: can this be cached?
     * @return a SCH_SHEET_LIST containing the schematic hierarchy
     */
    SCH_SHEET_LIST GetSheets() const
    {
        return SCH_SHEET_LIST( m_rootSheet );
    }

    SCH_SHEET& Root() const
    {
        return *m_rootSheet;
    }

    void SetRoot( SCH_SHEET* aRootSheet )
    {
        m_rootSheet = aRootSheet;
    }

    /// A simple test if the schematic is loaded, not a complete one
    bool IsValid() const
    {
        return m_rootSheet != nullptr;
    }

    /// Helper to retreive the screen of the root sheet
    SCH_SCREEN* RootScreen() const;

    /// Helper to retrieve the filename from the root sheet screen
    wxString GetFileName() const;

    SCH_SHEET_PATH& CurrentSheet() const
    {
        return *m_currentSheet;
    }

    void SetCurrentSheet( const SCH_SHEET_PATH& aPath )
    {
        *m_currentSheet = aPath;
    }

    CONNECTION_GRAPH* ConnectionGraph() const
    {
        return m_connectionGraph;
    }

    /**
     * Returns a pointer to a bus alias object for the given label,
     * or null if one doesn't exist
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aLabel ) const;

    ERC_SETTINGS* ErcSettings() const
    {
        return m_ercSettings;
    }

    // TODO(JE) Move out of SCHEMATIC
    int GetErcSeverity( int aErrorCode ) const;

    // TODO(JE) Move out of SCHEMATIC
    void SetErcSeverity( int aErrorCode, int aSeverity );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

};


/**
 * SHEETLIST_ERC_ITEMS_PROVIDER
 * is an implementation of the RC_ITEM_LISTinterface which uses the global SHEETLIST
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

    int GetCount( int aSeverity = -1 ) override;

    ERC_ITEM* GetItem( int aIndex ) override;

    void DeleteItem( int aIndex, bool aDeep ) override;

    void DeleteAllItems() override;
};



#endif
