/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
class SCH_GLOBALLABEL;


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

class SCHEMATIC;

class SCHEMATIC_LISTENER
{
public:
    virtual ~SCHEMATIC_LISTENER() {}
    virtual void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}
    virtual void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}
    virtual void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}
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
    PROJECT& Prj() const override { return *m_project; }
    void SetProject( PROJECT* aPrj );

    const std::map<wxString, wxString>* GetProperties() { return &m_properties; }

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

    void GetContextualTextVars( wxArrayString* aVars ) const;

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
     * Return the set of netname candidates for netclass assignment.  The list will include both
     * composite names (buses) and atomic net names.  Names are fetched from available labels,
     * power pins, etc.
     */
    std::set<wxString> GetNetClassAssignmentCandidates();

    /**
     * Resolves text vars that refer to other items.
     * Note that the actual resolve is delegated to the symbol/sheet in question.  This routine
     * just does the look-up and delegation.
     */
    bool ResolveCrossReference( wxString* token, int aDepth ) const;

    std::map<wxString, std::set<int>>& GetPageRefsMap() { return m_labelToPageRefsMap; }

    std::map<int, wxString> GetVirtualPageToSheetNamesMap() const;
    std::map<int, wxString> GetVirtualPageToSheetPagesMap() const;

    wxString ConvertRefsToKIIDs( const wxString& aSource ) const;
    wxString ConvertKIIDsToRefs( const wxString& aSource ) const;

    /**
     * Return the full schematic flattened hierarchical sheet list.
     */
    SCH_SHEET_LIST& GetFullHierarchy() const;

    /**
     * Update the symbol value and footprint instance data for legacy designs.
     *
     * Prior to schematic file format version 20200828 and legacy file format version, only
     * symbol reference field and unit were saved in the instance data.  The value and footprint
     * fields must be carried forward from the original symbol to prevent data loss.
     */
    void SetLegacySymbolInstanceData();

    /**
     * @return a filename that can be used in plot and print functions for the current screen
     * and sheet path.  This filename is unique and must be used instead of the screen filename
     * when one must create files for each sheet in the hierarchy.
     * Name is &ltroot sheet filename&gt-&ltsheet path&gt and has no extension.
     * However if filename is too long name is &ltsheet filename&gt-&ltsheet number&gt
     */
    wxString GetUniqueFilenameForCurrentSheet();

    /**
     * Set the m_ScreenNumber and m_NumberOfScreens members for screens.
     *
     * @note This must be called after deleting or adding a sheet and when entering a sheet.
     */
    void SetSheetNumberAndCount();

    /**
     * Update the schematic's page reference map for all global labels, and refresh the labels
     * so that they are redrawn with up-to-date references.
     */
    void RecomputeIntersheetRefs( const std::function<void( SCH_GLOBALLABEL* )>& aItemCallback );

    /**
     * Clear operating points from a .op simulation.
     */
    void ClearOperatingPoints()
    {
        m_operatingPoints.clear();
    }

    /**
     * Set operating points from a .op simulation.  Called after the simulation completes.
     */
    void SetOperatingPoint( const wxString& aSignal, double aValue )
    {
        m_operatingPoints[ aSignal ] = aValue;
    }

    wxString GetOperatingPoint( const wxString& aNetName, int aPrecision, const wxString& aRange );

    /**
     * Add junctions to this schematic where required. This function is needed for some plugins
     * (e.g. Legacy and Cadstar) in order to retain connectivity after loading.
     */
    void FixupJunctions();

    /**
     * Must be used if Add() is used using a BULK_x ADD_MODE to generate a change event for
     * listeners.
     */
    void OnItemsAdded( std::vector<SCH_ITEM*>& aNewItems );

    /**
     * Must be used if Remove() is used using a BULK_x REMOVE_MODE to generate a change event
     * for listeners.
     */
    void OnItemsRemoved( std::vector<SCH_ITEM*>& aRemovedItems );

    /**
     * Add a listener to the schematic to receive calls whenever something on the
     * schematic has been modified.  The schematic does not take ownership of the
     * listener object.  Make sure to call RemoveListener before deleting the
     * listener object.  The order of listener invocations is not guaranteed.
     * If the specified listener object has been added before, it will not be
     * added again.
     */
    void AddListener( SCHEMATIC_LISTENER* aListener );

    /**
     * Remove the specified listener.  If it has not been added before, it
     * will do nothing.
     */
    void RemoveListener( SCHEMATIC_LISTENER* aListener );

    /**
     * Remove all listeners
     */
    void RemoveAllListeners();

    /**
      * Notify the schematic and its listeners that an item on the schematic has
      * been modified in some way.
      */
    void OnItemsChanged( std::vector<SCH_ITEM*>& aItems );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

private:
    friend class SCH_EDIT_FRAME;

    template <typename Func, typename... Args>
    void InvokeListeners( Func&& aFunc, Args&&... args )
    {
        for( auto&& l : m_listeners )
            ( l->*aFunc )( std::forward<Args>( args )... );
    }

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
     * Holds a map of labels to the page sequence (virtual page number) that they appear on.  It is
     * used for updating global label intersheet references.
     */
    std::map<wxString, std::set<int>> m_labelToPageRefsMap;

    /**
     * Properties for text variable substitution (and perhaps other uses in future).
     */
    std::map<wxString, wxString>      m_properties;

    /**
     * Simulation operating points for text variable substitution.
     */
    std::map<wxString, double>        m_operatingPoints;

    /**
     * Currently installed listeners
     */
    std::vector<SCHEMATIC_LISTENER*> m_listeners;
};

#endif
