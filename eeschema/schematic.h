/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <sch_sheet_path.h>
#include <schematic_settings.h>


class BUS_ALIAS;
class CONNECTION_GRAPH;
class EDA_BASE_FRAME;
class ERC_SETTINGS;
class PROJECT;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_LIST;


class SCHEMATIC_IFACE
{
public:
    SCHEMATIC_IFACE() {};
    virtual ~SCHEMATIC_IFACE() {};

    virtual CONNECTION_GRAPH* ConnectionGraph() const = 0;
    virtual SCH_SHEET_LIST GetSheets() const = 0;
    virtual void SetCurrentSheet( const SCH_SHEET_PATH& aPath ) = 0;
    virtual SCH_SHEET_PATH& CurrentSheet() const = 0;
    virtual wxString GetFileName() const = 0;
    virtual PROJECT& Prj() const = 0;
};

/**
 * Holds all the data relating to one schematic.
 *
 * A schematic may consist of one or more sheets (and one root sheet)
 * Right now, Eeschema can have only one schematic open at a time, but this could change.
 * Please keep this possibility in mind when adding to this object.
 */
class SCHEMATIC : public SCHEMATIC_IFACE, public EDA_ITEM
{
public:
    SCHEMATIC( PROJECT* aPrj );

    virtual ~SCHEMATIC();

    virtual wxString GetClass() const override
    {
        return wxT( "SCHEMATIC" );
    }

    /// Initialize this schematic to a blank one, unloading anything existing.
    void Reset();

    /// Return a reference to the project this schematic is part of
    PROJECT& Prj() const override
    {
        return *m_project;
    }

    void SetProject( PROJECT* aPrj );

    /**
     * Builds and returns an updated schematic hierarchy
     * TODO: can this be cached?
     * @return a SCH_SHEET_LIST containing the schematic hierarchy
     */
    SCH_SHEET_LIST GetSheets() const override
    {
        return SCH_SHEET_LIST( m_rootSheet );
    }

    SCH_SHEET& Root() const
    {
        return *m_rootSheet;
    }

    /**
     * Initialize the schematic with a new root sheet.
     *
     * This is typically done by calling a file loader that returns the new root sheet
     * As a side-effect, takes care of some post-load initialization.
     *
     * @param aRootSheet is the new root sheet for this schematic.
     */
    void SetRoot( SCH_SHEET* aRootSheet );

    /// A simple test if the schematic is loaded, not a complete one
    bool IsValid() const
    {
        return m_rootSheet != nullptr;
    }

    /// Helper to retrieve the screen of the root sheet
    SCH_SCREEN* RootScreen() const;

    bool ResolveTextVar( wxString* token, int aDepth ) const;

    /// Helper to retrieve the filename from the root sheet screen
    wxString GetFileName() const override;

    SCH_SHEET_PATH& CurrentSheet() const override
    {
        return *m_currentSheet;
    }

    void SetCurrentSheet( const SCH_SHEET_PATH& aPath ) override
    {
        *m_currentSheet = aPath;
    }

    CONNECTION_GRAPH* ConnectionGraph() const override
    {
        return m_connectionGraph;
    }

    SCHEMATIC_SETTINGS& Settings() const;

    ERC_SETTINGS& ErcSettings() const;

    std::vector<SCH_MARKER*> ResolveERCExclusions();

    /**
     * Return a pointer to a bus alias object for the given label, or null if one
     * doesn't exist.
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aLabel ) const;

    /**
     * Return a list of name candidates for netclass assignment.  The list will include both
     * composite names (buses) and atomic net names.  Names are fetched from available labels,
     * power pins, etc.
     */
    std::vector<wxString> GetNetClassAssignmentCandidates();

    /**
     * Resolves text vars that refer to other items.
     * Note that the actual resolve is delegated to the symbol/sheet in question.  This routine
     * just does the look-up and delegation.
     */
    bool ResolveCrossReference( wxString* token, int aDepth ) const;

    std::map<wxString, std::set<wxString>>& GetPageRefsMap() { return m_labelToPageRefsMap; }

    wxString ConvertRefsToKIIDs( const wxString& aSource ) const;
    wxString ConvertKIIDsToRefs( const wxString& aSource ) const;

    /**
     * Return the full schematic flattened hierarchical sheet list.
     */
    SCH_SHEET_LIST& GetFullHierarchy() const;


#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

private:
    friend class SCH_EDIT_FRAME;

    PROJECT* m_project;

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

    /**
     * Holds a map of labels to the page numbers that they appear on.  Used to update global
     * label intersheet references.
     */
    std::map<wxString, std::set<wxString>> m_labelToPageRefsMap;
};

#endif
