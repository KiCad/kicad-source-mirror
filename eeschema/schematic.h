/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <embedded_files.h>
#include <schematic_holder.h>
#include <sch_sheet_path.h>
#include <schematic_settings.h>
#include <project.h>


class BUS_ALIAS;
class CONNECTION_GRAPH;
class EDA_BASE_FRAME;
class ERC_SETTINGS;
class PROJECT;
class SCH_COMMIT;
class SCH_LINE;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_LIST;
class SCH_GLOBALLABEL;
class SCH_REFERENCE;
class PROGRESS_REPORTER;
class TOOL_MANAGER;
class PICKED_ITEMS_LIST;

namespace KIFONT
{
class OUTLINE_FONT;
}

namespace KIGFX
{
class SCH_VIEW;
}


class SCHEMATIC;

class SCHEMATIC_LISTENER
{
public:
    virtual ~SCHEMATIC_LISTENER() {}
    virtual void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}
    virtual void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}
    virtual void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) {}

    // This is called when the user changes to a new sheet, not when a sheet is altered.
    // Sheet alteration events will call OnSchItems*
    virtual void OnSchSheetChanged( SCHEMATIC& aSch ) {}
};

enum SCH_CLEANUP_FLAGS
{
    NO_CLEANUP,
    LOCAL_CLEANUP,
    GLOBAL_CLEANUP
};

/**
 * Holds all the data relating to one schematic.
 *
 * A schematic may consist of one or more sheets (and one root sheet)
 * Right now, Eeschema can have only one schematic open at a time, but this could change.
 * Please keep this possibility in mind when adding to this object.
 */
class SCHEMATIC : public EDA_ITEM, public EMBEDDED_FILES, public PROJECT::_ELEM
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
    PROJECT& Project() const { return *m_project; }
    void SetProject( PROJECT* aPrj );

    const std::map<wxString, wxString>* GetProperties() { return &m_properties; }

    SCH_SHEET_LIST BuildSheetListSortedByPageNumbers() const;

    SCH_SHEET_LIST BuildUnorderedSheetList() const;

    /**
     * Return the full schematic flattened hierarchical sheet list.
     */
    SCH_SHEET_LIST Hierarchy() const;

    /**
     * Check if the hierarchy has been built.
     *
     * @return true if RefreshHierarchy() has been called and the hierarchy is populated.
     */
    bool HasHierarchy() const { return !m_hierarchy.empty(); }

    void RefreshHierarchy();

    SCH_ITEM* ResolveItem( const KIID& aID, SCH_SHEET_PATH* aPathOut = nullptr,
                           bool aAllowNullptrReturn = false ) const
    {
        return BuildUnorderedSheetList().ResolveItem( aID, aPathOut, aAllowNullptrReturn );
    }

    SCH_SHEET& Root() const
    {
        return *m_rootSheet;
    }

    /**
     * Get the list of top-level sheets.
     *
     * @return vector of pointers to top-level sheets (children of virtual root).
     */
    std::vector<SCH_SHEET*> GetTopLevelSheets() const;

    SCH_SHEET* GetTopLevelSheet( int aIndex = 0 ) const;

    void SetTopLevelSheets( const std::vector<SCH_SHEET*>& aSheets );

    /**
     * Add a new top-level sheet to the schematic.
     *
     * @param aSheet is the sheet to add as a top-level sheet.
     */
    void AddTopLevelSheet( SCH_SHEET* aSheet );

    /**
     * Remove a top-level sheet from the schematic.
     *
     * @param aSheet is the sheet to remove.
     * @return true if the sheet was successfully removed.
     */
    bool RemoveTopLevelSheet( SCH_SHEET* aSheet );

    /**
     * Check if a sheet is a top-level sheet (direct child of virtual root).
     *
     * @param aSheet is the sheet to check.
     * @return true if the sheet is a top-level sheet.
     */
    bool IsTopLevelSheet( const SCH_SHEET* aSheet ) const;

    /// A simple test if the schematic is loaded, not a complete one
    bool IsValid() const
    {
        return m_project && m_rootSheet != nullptr;
    }

    /// Helper to retrieve the screen of the root sheet
    SCH_SCREEN* RootScreen() const;

    void GetContextualTextVars( wxArrayString* aVars ) const;

    bool ResolveTextVar( const SCH_SHEET_PATH* aSheetPath, wxString* token, int aDepth ) const;

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

    SCH_SCREEN* GetCurrentScreen() const { return CurrentSheet().LastScreen(); }

    CONNECTION_GRAPH* ConnectionGraph() const
    {
        return m_connectionGraph;
    }

    SCHEMATIC_SETTINGS& Settings() const;

    ERC_SETTINGS& ErcSettings() const;

    std::vector<SCH_MARKER*> ResolveERCExclusions();

    EMBEDDED_FILES* GetEmbeddedFiles() override;
    const EMBEDDED_FILES* GetEmbeddedFiles() const;

    /**
     * Return a pointer to a bus alias object for the given label, or null if one
     * doesn't exist.
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aLabel ) const;

    void AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    void SetBusAliases( const std::vector<std::shared_ptr<BUS_ALIAS>>& aAliases );

    const std::vector<std::shared_ptr<BUS_ALIAS>>& GetAllBusAliases() const
    {
        return m_busAliases;
    }

    /**
     * Return the set of netname candidates for netclass assignment.
     *
     * The list will include both composite names (buses) and atomic net names.  Names are
     * fetched from available labels, power pins, etc.
     */
    std::set<wxString> GetNetClassAssignmentCandidates();

    /**
     * Resolves text vars that refer to other items.
     *
     * @note The actual resolve is delegated to the symbol/sheet in question.  This routine
     *       just does the look-up and delegation.
     */
    bool ResolveCrossReference( wxString* token, int aDepth ) const;

    std::map<wxString, std::set<int>>& GetPageRefsMap() { return m_labelToPageRefsMap; }

    std::map<int, wxString> GetVirtualPageToSheetNamesMap() const;
    std::map<int, wxString> GetVirtualPageToSheetPagesMap() const;

    wxString ConvertRefsToKIIDs( const wxString& aSource ) const;
    wxString ConvertKIIDsToRefs( const wxString& aSource ) const;

    /**
     * Update the symbol value and footprint instance data for legacy designs.
     *
     * Prior to schematic file format version 20200828 and legacy file format version, only
     * symbol reference field and unit were saved in the instance data.  The value and footprint
     * fields must be carried forward from the original symbol to prevent data loss.
     */
    void SetLegacySymbolInstanceData();

    /**
     * Get the unique file name for the current sheet.
     *
     * This file name is unique and must be used instead of the screen file name when one must
     * create files for each sheet in the hierarchy.  The name is
     * &ltroot sheet filename&gt-&ltsheet path&gt and has no extension.
     * If filename is too long name is &ltsheet filename&gt-&ltsheet number&gt
     *
     * @return a filename that can be used in plot and print functions for the current screen
     * and sheet path.
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
    void RecomputeIntersheetRefs();

    /**
     * Clear operating points from a .op simulation.
     */
    void ClearOperatingPoints()
    {
        m_operatingPoints.clear();
    }

    /**
     * Set operating points from a .op simulation.
     *
     * Called after the simulation completes.
     */
    void SetOperatingPoint( const wxString& aSignal, double aValue )
    {
        m_operatingPoints[ aSignal ] = aValue;
    }

    wxString GetOperatingPoint( const wxString& aNetName, int aPrecision, const wxString& aRange );

    /**
     * Add junctions to this schematic where required.
     *
     * This function is needed for some plugins (e.g. Legacy and Cadstar) in order to retain
     * connectivity after loading.
     */
    void FixupJunctionsAfterImport();

    /**
     * Scan existing markers and record data from any that are Excluded.
     */
    void RecordERCExclusions();

    /**
     * Update markers to match recorded exclusions.
     */
    void ResolveERCExclusionsPostUpdate();

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
      * Notify the schematic and its listeners that an item on the schematic has
      * been modified in some way.
      */
    void OnItemsChanged( std::vector<SCH_ITEM*>& aItems );

    /**
      * Notify the schematic and its listeners that the current sheet has been changed.
      *
      * This is called when the user navigates to a different sheet, not when the sheet is
      * altered.
      */
    void OnSchSheetChanged();

    /**
     * Add a listener to the schematic to receive calls whenever something on the
     * schematic has been modified.
     *
     * The schematic does not take ownership of the listener object.  Make sure to call
     * RemoveListener before deleting the listener object.  The order of listener invocations
     * is not guaranteed.  If the specified listener object has been added before, it will
     * not be added again.
     */
    void AddListener( SCHEMATIC_LISTENER* aListener );

    /**
     * Remove the specified listener.
     *
     * If it has not been added before, it will do nothing.
     */
    void RemoveListener( SCHEMATIC_LISTENER* aListener );

    /**
     * Remove all listeners.
     */
    void RemoveAllListeners();

    void RunOnNestedEmbeddedFiles( const std::function<void( EMBEDDED_FILES* )>& aFunction ) override;

    /**
     * Embed fonts in the schematic.
     */
    void EmbedFonts() override;

    /**
     * Get a set of fonts used in the schematic.
     */
    std::set<KIFONT::OUTLINE_FONT*> GetFonts() const override;

    /**
     * Return a list of schematic files in the current project that contain instance data for
     * multiple projects.
     *
     * @return all of the #SCH_SCREEN objects that contain instance data for multiple projects.
     */
    std::set<const SCH_SCREEN*> GetSchematicsSharedByMultipleProjects() const;

    /**
     * Test if the schematic is a complex hierarchy.
     *
     * @return true if the schematic is a complex hierarchy or false if it's a simple hierarchy.
     */
    bool IsComplexHierarchy() const;

    void SetSchematicHolder( SCHEMATIC_HOLDER* aHolder ) { m_schematicHolder = aHolder; }

    /**
     * Perform routine schematic cleaning including breaking wire and buses and deleting
     * identical objects superimposed on top of each other.
     *
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     */
    void CleanUp( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen = nullptr );

    /**
     * Generate the connection data for the entire schematic hierarchy.
     */
    void RecalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanupFlags,
                                 TOOL_MANAGER* aToolManager,
                                 PROGRESS_REPORTER* aProgressReporter = nullptr,
                                 KIGFX::SCH_VIEW* aSchView = nullptr,
                                 std::function<void( SCH_ITEM* )>* aChangedItemHandler = nullptr,
                                 PICKED_ITEMS_LIST*                aLastChangeList = nullptr );

    /**
     * Store all existing annotations in the REFDES_TRACKER.
     * This is useful when adding existing sheets or enabling tracking for the first time.
     * It will not change the existing annotations, but will allow the REFDES_TRACKER
     * to track the existing annotations and prevent duplicates.
     */
    void CacheExistingAnnotation();

    /**
     * Check if the schematic contains the specified reference.
     *
     * @param aRef is the reference to check for.
     * @return true if the schematic contains the reference, false otherwise.
     */
    bool Contains( const SCH_REFERENCE& aRef ) const;

    void CreateDefaultScreens();

    /**
     * Return an array of variant names for using in wxWidgets UI controls.
     *
     * Most wxWidgets UI controls that support multiple entries take a wxArrayString as an argument.  This
     * is a convenience method so it's not necessary to convert a std::set<wxString> to a wxArrayString at
     * the point of use.
     *
     * @note This also adds the pseudo "<default>" entry  to the beginning of the array followed by all other
     *       variant names sorted using the #StrNumCmp function.
     */
    wxArrayString GetVariantNamesForUI() const;

    /**
     * Return the current variant being edited.
     *
     * @return the name of the current variant being edited.  An empty string is the default variant.
     */
    wxString GetCurrentVariant() const;

    void SetCurrentVariant( const wxString& aVariantName );

    /**
     * Delete all information for @a aVariantName.
     *
     * @param aVariantName is the name of the variant to remove.
     */
    void DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit = nullptr );

    void AddVariant( const wxString& aVariantName );

    /**
     * Rename a variant from @a aOldName to @a aNewName.
     *
     * This updates the variant name in all symbols and sheets throughout the schematic.
     *
     * @param aOldName is the current name of the variant to rename.
     * @param aNewName is the new name for the variant.
     * @param aCommit is an optional SCH_COMMIT for undo/redo support.
     */
    void RenameVariant( const wxString& aOldName, const wxString& aNewName,
                        SCH_COMMIT* aCommit = nullptr );

    /**
     * Copy a variant from @a aSourceVariant to @a aNewVariant.
     *
     * This creates a new variant with data copied from the source variant for all
     * symbols and sheets throughout the schematic.
     *
     * @param aSourceVariant is the name of the variant to copy from.
     * @param aNewVariant is the name of the new variant to create.
     * @param aCommit is an optional SCH_COMMIT for undo/redo support.
     */
    void CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                      SCH_COMMIT* aCommit = nullptr );

    /**
     * Return the set of variant names (without the default placeholder).
     */
    const std::set<wxString>& GetVariantNames() const { return m_variantNames; }

    /**
     * Return the description for a variant.
     *
     * @param aVariantName is the name of the variant.
     * @return the description or an empty string if the variant has no description.
     */
    wxString GetVariantDescription( const wxString& aVariantName ) const;

    /**
     * Set the description for a variant.
     *
     * @param aVariantName is the name of the variant.
     * @param aDescription is the description to set.
     */
    void SetVariantDescription( const wxString& aVariantName, const wxString& aDescription );

    /**
     * This is a throw away method for variant testing.
     *
     * Once the schematic loading is properly fixed due to SetRoot() method breakage, this method should
     * be removed.
     */
    void LoadVariants();

    /**
     * True if a SCHEMATIC exists, false if not
     */
    static bool m_IsSchematicExists;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::SCHEMATIC; }

    /**
     * Save schematic files to the .history directory.
     *
     * This method is used as a saver callback for LOCAL_HISTORY during autosave operations.
     *
     * @param aProjectPath The path to check against this schematic's project path
     * @param aFiles Output vector to append absolute file paths for history inclusion
     */
    void SaveToHistory( const wxString& aProjectPath, std::vector<wxString>& aFiles );

private:
    friend class SCH_EDIT_FRAME;

    template <typename Func, typename... Args>
    void InvokeListeners( Func&& aFunc, Args&&... args )
    {
        for( auto&& l : m_listeners )
            ( l->*aFunc )( std::forward<Args>( args )... );
    }

    void ensureVirtualRoot();
    void ensureDefaultTopLevelSheet();
    void ensureCurrentSheetIsTopLevel();
    void rebuildHierarchyState( bool aResetConnectionGraph );

    PROJECT* m_project;

    /// The virtual root sheet (has no screen, contains all top-level sheets)
    SCH_SHEET* m_rootSheet;

    /// List of top-level sheets (direct children of virtual root)
    std::vector<SCH_SHEET*> m_topLevelSheets;

    /**
     * The sheet path of the sheet currently being edited or displayed.
     *
     * @note This was moved here from #SCH_EDIT_FRAME because currently many places in the code
     * want to know the current sheet.  Potentially this can be moved back to the UI code once
     * the only places that want to know it are UI-related.
     */
    SCH_SHEET_PATH* m_currentSheet;

    /// Hold and calculate connectivity information of this schematic.
    CONNECTION_GRAPH* m_connectionGraph;

    /**
     * Holds a map of labels to the page sequence (virtual page number) that they appear on.
     *
     * It is used for updating global label intersheet references.
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
     * Cache of the entire schematic hierarchy sorted by sheet page number.
     */
    SCH_SHEET_LIST m_hierarchy;

    /**
     * Currently installed listeners.
     */
    std::vector<SCHEMATIC_LISTENER*> m_listeners;

    /**
     * What currently "Holds" the schematic, i.e. a edit frame if available
     */
    SCHEMATIC_HOLDER* m_schematicHolder;

    void loadBusAliasesFromProject();

    void updateProjectBusAliases();

    std::vector<std::shared_ptr<BUS_ALIAS>> m_busAliases;

    wxString m_currentVariant;

    std::set<wxString> m_variantNames;

    /// Re-entry guard to prevent infinite recursion between ensureDefaultTopLevelSheet and
    /// RefreshHierarchy when setting up new schematics
    bool m_settingTopLevelSheets = false;
};

#endif
