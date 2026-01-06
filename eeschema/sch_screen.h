/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <memory>
#include <stddef.h>
#include <unordered_set>
#include <vector>
#include <wx/arrstr.h>
#include <wx/chartype.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <base_screen.h>
#include <core/typeinfo.h>
#include <kiid.h>
#include <kiway_holder.h>
#include <layer_ids.h>
#include <marker_base.h>
#include <page_info.h>
#include <template_fieldnames.h>
#include <title_block.h>

#include <lib_id.h>
#include <sch_reference_list.h>
#include <sch_rtree.h>
#include <sch_sheet.h>
#include <sch_label.h>
#include <sch_sheet_path.h>     // SCH_SYMBOL_INSTANCE

class BUS_ALIAS;
class EDA_ITEM;
class LIB_SYMBOL;
class SCH_COMMIT;
class SCH_PIN;
class SCH_SYMBOL;
class SCH_LINE;
class SCH_LABEL_BASE;
class PLOTTER;
class REPORTER;
class SCH_IO_ALTIUM;
class SCH_EDIT_FRAME;
class SCH_SHEET_LIST;
class SCH_IO_KICAD_SEXPR_PARSER;
class SCH_IO_KICAD_SEXPR;
class TEST_SCH_SCREEN_FIXTURE;

enum SCH_LINE_TEST_T
{
    ENTIRE_LENGTH_T,
    END_POINTS_ONLY_T,
    EXCLUDE_END_POINTS_T
};


struct PICKED_SYMBOL
{
    LIB_ID LibId;
    int    Unit;
    int    Convert;

    bool   KeepSymbol;
    bool   PlaceAllUnits;

    std::vector<std::pair<FIELD_T, wxString>> Fields;

    PICKED_SYMBOL() :
            Unit( 1 ),
            Convert( 1 ),
            KeepSymbol( false ),
            PlaceAllUnits( false )
    {
    }
};

class SCH_SCREEN : public BASE_SCREEN
{
public:
    SCH_SCREEN( EDA_ITEM* aParent = nullptr );

    ~SCH_SCREEN();

    SCHEMATIC* Schematic() const;

    /**
     * Get the full RTree, usually for iterating.
     *
     * @note The iteration order of the RTree is not readily apparent and will change
     *       if/when you add or move items and the RTree is re-balanced.  Any exposure of the
     *       RTree contents to the user MUST be sorted before being presented.  See
     *       SCH_IO_KICAD_SEXPR::Format() or SCH_EDITOR_CONTROL::nextMatch() for examples.
     *
     * @return Complete RTree of the screen's items.
     */
    EE_RTREE& Items() { return m_rtree; }
    const EE_RTREE& Items() const { return m_rtree; }

    bool IsEmpty() const
    {
        return m_rtree.empty();
    }

    bool HasItems( KICAD_T aItemType ) const;

    bool HasSheets() const { return HasItems( SCH_SHEET_T ); }

    static bool ClassOf( const EDA_ITEM* aItem );

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_SCREEN" );
    }

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }

    const PAGE_INFO& GetPageSettings() const                { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    /**
     * Set the file name for this screen to \a aFileName.
     *
     * @note Screen file names must be absolute or empty.  Absolute file names do not have to
     *       exist yet in the case of a new schematic file but file names cannot be relative.
     *
     * @param aFileName is the absolute file name and path of the screen.
     */
    void SetFileName( const wxString& aFileName );

    const wxString& GetFileName() const                     { return m_fileName; }

    void SetFileReadOnly( bool aIsReadOnly )                { m_isReadOnly = aIsReadOnly; }
    bool IsReadOnly() const                                 { return m_isReadOnly; }

    void SetFileExists( bool aFileExists )                  { m_fileExists = aFileExists; }
    bool FileExists() const                                 { return m_fileExists; }

    const VECTOR2I& GetAuxOrigin() const                    { return m_aux_origin; }
    void SetAuxOrigin( const VECTOR2I& aPosition )          { m_aux_origin = aPosition; }

    const TITLE_BLOCK& GetTitleBlock() const                { return m_titles; }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )    { m_titles = aTitleBlock; }

    void DecRefCount();
    void IncRefCount();
    int GetRefCount() const                                 { return m_refCount; }

    void SetConnectivityDirty();

    /**
     * Return the number of times this screen is used.
     *
     * In the legacy file formats: if this screen is used only once (not a complex hierarchy)
     * the reference field can be used to store the symbol reference.  If this screen is used
     * more than once (a complex hierarchy), then symbols must have a full alternate reference
     * management via sheet paths.
     *
     * In the new schematic file format, all instance data is stored in the root sheet even
     * for simple hierarchies.
     *
     * @return the sheet paths sharing this screen.
     */
    std::vector<SCH_SHEET_PATH>& GetClientSheetPaths()
    {
        return m_clientSheetPathList;
    }

    void Append( SCH_ITEM* aItem, bool aUpdateLibSymbol = true );

    /**
     * Copy the contents of \a aScreen into this #SCH_SCREEN object.
     *
     * @warning The contents of \a Screen cannot contain any duplicate sheet names or any
     *          hierarchy recursion issues or bad things will happen.
     *
     * @param aScreen is the screen to append to this one.
     * @return false if there are any duplicate sheet names or any hierarchy recursion issues the
     *         calling this method or KiCad will crash.
     */
    void Append( SCH_SCREEN* aScreen );

    /**
     * Delete all draw items and clears the project settings.
     */
    void Clear( bool aFree = true );

    /**
     * Free all the items from the schematic associated with the screen.
     *
     * This does not delete any sub hierarchies.
     */
    void FreeDrawList();

    /**
     * Check \a aPosition within a distance of \a aAccuracy for items of type \a aFilter.
     *
     * @param[in] aPosition Position in drawing units.
     * @param aAccuracy The maximum distance within \a Position to check for an item.
     * @param aType The type of item to find.
     * @return The item found that meets the search criteria or NULL if none found.
     */
    SCH_ITEM* GetItem( const VECTOR2I& aPosition, int aAccuracy = 0,
                       KICAD_T aType = SCH_LOCATE_ANY_T ) const;

    /**
     * Initialize the #LIB_SYMBOL reference for each #SCH_SYMBOL found in this schematic
     * from the project #SYMBOL_LIB_TABLE.
     *
     * Symbol library links are set using the symbol library table and will fall back to
     * the cache only if the cache is loaded.  The cache should only be loaded when opening
     * legacy schematic files.
     *
     * @note This should only be called when the user specifically requests all library symbol
     *       links to be updated or when the legacy schematic is opened for the last time.  All
     *       subsequent schematic loads with the new s-expression will contain the library
     *       symbols and should call #UpdateLocalLibSymbolLinks.
     *
     * @param[in] aReporter Optional #REPORTER object to write status and error messages into.
     */
    void UpdateSymbolLinks( REPORTER* aReporter = nullptr );

    /**
     * Initialize the #LIB_SYMBOL reference for each #SCH_SYMBOL found in this schematic
     * with the local project library symbols.
     */
    void UpdateLocalLibSymbolLinks();

    /**
     * Plot all the schematic objects to \a aPlotter.
     *
     * @note This function is useful only for schematic. The library editor and library viewer
     *       do not use a draw list and therefore plots nothing.
     *
     * @param[in] aPlotter The plotter object to plot to.
     */
    void Plot( PLOTTER* aPlotter, const SCH_PLOT_OPTS& aPlotOpts ) const;

    /**
     * Plot selected schematic objects to \a aPlotter.
     *
     * @param[in] aPlotter The plotter object to plot to.
     * @param[in] aPlotOpts The plot options to use.
     * @param[in] aItems The items to plot.
     */
    void Plot( PLOTTER* aPlotter, const SCH_PLOT_OPTS& aPlotOpts,
               const std::vector<SCH_ITEM*>& aItems ) const;

    /**
     * Remove \a aItem from the schematic associated with this screen.
     *
     * @note The removed item is not deleted.  It is only unlinked from the item list.
     * @param[in] aItem Item to be removed from schematic.
     * @param aUpdateLibSymbol removes the library symbol as required when true.
     * @return True if we successfully removed the item
     */
    bool Remove( SCH_ITEM* aItem, bool aUpdateLibSymbol = true );

    /**
     * Update \a aItem's bounding box in the tree
     *
     * @param[in] aItem Item that needs to be updated.
     * @param aUpdateLibSymbol removes the library symbol as required when true.
     */
    void Update( SCH_ITEM* aItem, bool aUpdateLibSymbol = true );

    /**
     * Remove \a aItem from the linked list and deletes the object.
     *
     * If \a aItem is a schematic sheet label, it is removed from the screen associated with
     * the sheet that contains the label to be deleted.
     *
     * @param[in] aItem The schematic object to be deleted from the screen.
     */
    void DeleteItem( SCH_ITEM* aItem );

    bool CheckIfOnDrawList( const SCH_ITEM* aItem ) const;

    /**
     * Test all of the connectable objects in the schematic for unused connection points.
     *
     * @param aPath is a sheet path to pass to UpdateDanglingState if desired.
     * @param aChangedHandler is an optional callback to make on each changed item.
     */
    void TestDanglingEnds( const SCH_SHEET_PATH* aPath = nullptr,
                           std::function<void( SCH_ITEM* )>* aChangedHandler = nullptr ) const;

    /**
     * Return all wires and junctions connected to \a aItem which are not connected any
     * symbol pin or all graphical segments lines connected to \a aItem.
     *
     * @param aItem The item to test for connections.
     * @return a set of all #SCH_ITEM objects connected to \a aItem.
     */
    std::set<SCH_ITEM*> MarkConnections( SCH_ITEM* aItem, bool aSecondPass );

    /**
     * Clear the state flags of all the items in the screen.
     */
    void ClearDrawingState();

    size_t CountConnectedItems( const VECTOR2I& aPos, bool aTestJunctions ) const;

    /**
     * Test if a junction is required for the items at \a aPosition on the screen.  Note that
     * this could be either an implied junction (bus entry) or an explicit junction (dot).
     *
     * A junction is required at \a aPosition if one of the following criteria is satisfied:
     *  - One wire midpoint and one or more wire endpoints.
     *  - Three or more wire endpoints.
     *  - One wire midpoint and a symbol pin.
     *  - Two or more wire endpoints and a symbol pin.
     *  - One bus midpoint or endpoint and a bus entry.
     *
     * @param[in] aPosition The position to test.
     * @return True if a junction is required at \a aPosition.
     */
    bool IsJunction( const VECTOR2I& aPosition ) const;

    /**
     * Indicate that a junction dot is necessary at the given location.
     *
     * See IsJunctionNeeded() for more info.
     */
    bool IsExplicitJunction( const VECTOR2I& aPosition ) const;

    /**
     * Indicate that a junction dot is necessary at the given location, and does not yet exist.
     *
     * See IsJunctionNeeded() for more info.
     */
    bool IsExplicitJunctionNeeded( const VECTOR2I& aPosition ) const;

    SPIN_STYLE GetLabelOrientationForPoint( const VECTOR2I&       aPosition,
                                            SPIN_STYLE            aDefaultOrientation,
                                            const SCH_SHEET_PATH* aSheet ) const;
    /**
     * Indicate that a junction dot may be placed at the given location.
     *
     * See IsJunctionNeeded() for more info.
     */
    bool IsExplicitJunctionAllowed( const VECTOR2I& aPosition ) const;

    /**
     * Test if \a aPosition is a connection point on \a aLayer.
     *
     * @param[in] aPosition Position to test.
     * @param aLayer The layer type to test against.  Valid layer types are #LAYER_NOTES,
     *               #LAYER_BUS, and #LAYER_WIRE.
     * @return True if \a Position is a connection point on \a aLayer.
     */
    bool IsTerminalPoint( const VECTOR2I& aPosition, int aLayer ) const;

    /**
     * Test the screen for a symbol pin item at \a aPosition.
     *
     * @param[in] aPosition Position to test.
     * @param[out] aSymbol The symbol if a pin was found, otherwise NULL.
     * @param aEndPointOnly Set to true to test if \a aPosition is the connection
     *                      point of the pin.
     * @return The pin item if found, otherwise NULL.
     */
    SCH_PIN* GetPin( const VECTOR2I& aPosition, SCH_SYMBOL** aSymbol = nullptr,
                     bool aEndPointOnly = false ) const;

    /**
     * Test the screen if \a aPosition is a sheet label object.
     *
     * @param[in] aPosition The position to test.
     * @return The sheet label object if found otherwise NULL.
     */
    SCH_SHEET_PIN* GetSheetPin( const VECTOR2I& aPosition ) const;

    /**
     * Clear the annotation for the symbols in \a aSheetPath on the screen.
     *
     * @param[in] aSheetPath The sheet path of the symbol annotation to clear.  If NULL then
     *                       the entire hierarchy is cleared.
     * @param[in] aResetPrefix The annotation prefix ('R', 'U', etc.) should be reset to the
     *                         symbol library prefix.
     */
    void ClearAnnotation( SCH_SHEET_PATH* aSheetPath, bool aResetPrefix );

    /**
     * For screens shared by many sheetpaths (complex hierarchies):
     * to be able to clear or modify any reference related  sharing this screen
     * (i.e. the list of symbols), an entry for each screen path must exist.
     * This function creates missing entries, using as default reference the current
     * reference field and unit number
     * Note: m_clientSheetPathList must be up to date
     * ( built by SCH_SCREENS::BuildClientSheetPathList() )
     */
    void EnsureAlternateReferencesExist();

    /**
     * Add all schematic sheet and symbol objects in the screen to \a aItems.
     *
     * @param[out] aItems Hierarchical item list to fill.
     */
    void GetHierarchicalItems( std::vector<SCH_ITEM*>* aItems ) const;

    /**
     * Similar to Items().OfType( SCH_SHEET_T ), but return the sheets in a
     * deterministic order (L-R, T-B) for sheet numbering.
     */
    void GetSheets( std::vector<SCH_ITEM*>* aItems ) const;

    /**
     * Return a line item located at \a aPosition.
     *
     * @param[in] aPosition The VECTOR2I to test for a line item.
     * @param aAccuracy Amount to inflate the item hit test bounding box.
     * @param aLayer The layer the line is drawn upon.
     * @param aSearchType Additional line test criteria.
     * @return The SCH_LINE* of the wire item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_LINE* GetLine( const VECTOR2I& aPosition, int aAccuracy = 0, int aLayer = LAYER_NOTES,
                       SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T ) const;

    SCH_LINE* GetWire( const VECTOR2I& aPosition, int aAccuracy = 0,
                       SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T ) const
    {
        return GetLine( aPosition, aAccuracy, LAYER_WIRE, aSearchType );
    }

    SCH_LINE* GetBus( const VECTOR2I& aPosition, int aAccuracy = 0,
                      SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T ) const
    {
        return GetLine( aPosition, aAccuracy, LAYER_BUS, aSearchType );
    }

    /**
     * Return buses and wires passing through aPosition.
     *
     * @param aPosition Position to search for
     * @param aIgnoreEndpoints If true, ignore wires/buses with end points matching aPosition
     * @return Buses and wires
    */
    std::vector<SCH_LINE*> GetBusesAndWires( const VECTOR2I& aPosition,
                                             bool aIgnoreEndpoints = false ) const;

    /**
     * Collect a unique list of all possible connection points in the schematic.
     *
     * @return vector of connections
     */
    std::vector<VECTOR2I> GetConnections() const;

    /**
     * Return the unique set of points belonging to aItems where a junction is needed.
     *
     * @param aItems List of objects to check
     * @return Points where a junction is needed
    */
    std::vector<VECTOR2I> GetNeededJunctions( const std::deque<EDA_ITEM*>& aItems ) const;

    /**
     * Return a label item located at \a aPosition.
     *
     * @param[in] aPosition The VECTOR2I to test for label items.
     * @param aAccuracy Amount to inflate the item hit test bounding box.
     * @return The SCH_LABEL_BASE* of the label item found or nullptr.
     */
    SCH_LABEL_BASE* GetLabel( const VECTOR2I& aPosition, int aAccuracy = 0 ) const;

    /**
     * Fetch a list of unique #LIB_SYMBOL object pointers required to properly render each
     * #SCH_SYMBOL in this schematic.
     *
     * @return The list of unique #LIB_SYMBOL object pointers.
     */
    const std::map<wxString, LIB_SYMBOL*>& GetLibSymbols() const { return m_libSymbols; }

    /**
     * Add \a aLibSymbol to the library symbol map.
     *
     * The symbol is mapped to the result of #LIB_ID::Format().  If a symbol is already
     * mapped, the existing symbol is replaced with \a aLibSymbol.  The screen object takes
     * ownership of the pointer.
     *
     * @param aLibSymbol A pointer the #LIB_SYMBOL to be added to the symbol map.
     */
    void AddLibSymbol( LIB_SYMBOL* aLibSymbol );

    /**
     * After loading a file from disk, the library symbols do not yet contain the full
     * data for their embedded files, only a reference.  This iterates over all lib symbols
     * in the schematic and updates the library symbols with the full data.
    */
    void FixupEmbeddedData();

    /**
     * Add a bus alias definition.
     */
    void AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    const std::vector<SCH_SYMBOL_INSTANCE>& GetSymbolInstances() const
    {
        return m_symbolInstances;
    }

    const std::vector<SCH_SHEET_INSTANCE>& GetSheetInstances() const
    {
        return m_sheetInstances;
    }

    const KIID& GetUuid() const { return m_uuid; }

    void AssignNewUuid() { m_uuid = KIID(); }

    /**
     * Update the symbol value and footprint instance data for legacy designs.
     */
    void SetLegacySymbolInstanceData();

    /**
     * Fix legacy power symbols that have mismatched value text fields
     * and invisible power pin names.
     */
    void FixLegacyPowerSymbolMismatches();

    /**
     * Check all symbol default instance to see if they are not set yet.
     */
    bool AllSymbolDefaultInstancesNotSet();

    bool HasSymbolFieldNamesWithWhiteSpace() const;

    /**
     * Check if the schematic file is in the current project path.
     *
     * @retval true if the schematic file resides in the current project path or a sub-folder.
     * @retval false if the schematic file does not reside within the current project path.
     */
    bool InProjectPath() const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    /**
     * Migrate any symbols having V6 simulation models to their V7 equivalents.
     */
    void MigrateSimModels();

    /**
     * Remove all invalid symbol instance data in this screen object for the project defined
     * by \a aProjectName and the list of \a aValidSheetPaths.
     *
     * @warning This method will assert and exit on debug builds when \a aProjectName is empty.
     *
     * @note This method does not affect instance data for any other projects.
     *
     * @param aProjectName is the name of the current project.
     * @param aValidSheetPaths is the list of valid #SCH_SHEET_PATH objects for the current
     *                         project.
     */
    void PruneOrphanedSymbolInstances( const wxString& aProjectName,
                                       const SCH_SHEET_LIST& aValidSheetPaths );

    /**
     * Remove all invalid sheet instance data in this screen object for the project defined
     * by \a aProjectName and the list of \a aValidSheetPaths.
     *
     * @warning This method will assert and exit on debug builds when \a aProjectName is empty.
     *
     * @note This method does not affect instance data for any other projects.
     *
     * @param aProjectName is the name of the current project.
     * @param aValidSheetPaths is the list of valid #SCH_SHEET_PATH objects for the current
     *                         project.
     */
    void PruneOrphanedSheetInstances( const wxString& aProjectName,
                                      const SCH_SHEET_LIST& aValidSheetPaths );

    /**
     * @return a list of names of all of the sheets in this schematic file.
     */
    std::set<wxString> GetSheetNames() const;

    /**
     * Check symbols for instance data from other projects.
     *
     * @retval true if the schematic contains symbols and/or sheets with instances from a
     *         project other than the current project.
     * @retval false if the schematic does not  contain symbols and/or sheets with instances
     *         from a project other than the current project.
     */
    bool HasInstanceDataFromOtherProjects() const;

    /**
     * Consistency check of internal m_groups structure.
     *
     * @param repair if true, modify groups structure until it passes the sanity check.
     * @return empty string on success.  Or error description if there's a problem.
     */
    wxString GroupsSanityCheck( bool repair = false );

    /**
     * @param repair if true, make one modification to groups structure that brings it
     *        closer to passing the sanity check.
     * @return empty string on success.  Or error description if there's a problem.
     */
    wxString GroupsSanityCheckInternal( bool repair );

    std::set<wxString> GetVariantNames() const;

    void DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit = nullptr );

    void RenameVariant( const wxString& aOldName, const wxString& aNewName,
                        SCH_COMMIT* aCommit = nullptr );

    void CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                      SCH_COMMIT* aCommit = nullptr );

private:
    friend SCH_EDIT_FRAME;     // Only to populate m_symbolInstances.
    friend SCH_IO_KICAD_SEXPR_PARSER;   // Only to load instance information from schematic file.
    friend SCH_IO_KICAD_SEXPR;   // Only to save the loaded instance information to schematic file.
    friend SCH_IO_ALTIUM;
    friend TEST_SCH_SCREEN_FIXTURE;

    bool doIsJunction( const VECTOR2I& aPosition, bool aBreakCrossings,
                       bool* aHasExplicitJunctionDot, bool* aHasBusEntry ) const;

    void clearLibSymbols();

    /**
     * Return a list of potential library symbol matches for \a aSymbol.
     *
     * When and existing library symbol named with the full #LIB_ID object is found, there may
     * be more potential matches if the #SCH_SCREEN::Append() method need to create an alternate
     * symbol due to differences from the original symbol.  This process creates a new library
     * symbol name by adding a "_#" suffix to the existing #LIB_ID item name.
     *
     * @param[in] aSymbol is the schematic symbol to search for potential library symbol matches.
     * @param[out] aMatches contains library cache names of all of the potential matches.
     *
     * @return the number of potential matches found for \a aSymbol.
     */
    size_t getLibSymbolNameMatches( const SCH_SYMBOL& aSymbol, std::vector<wxString>& aMatches );

public:
    bool IsZoomInitialized() const { return m_zoomInitialized; }

    /**
     * last value for the zoom level, useful in Eeschema when changing the current displayed
     * sheet to reuse the same zoom level when back to the sheet using this screen
     */
    double m_LastZoomLevel;

private:
    wxString    m_fileName;                 // File used to load the screen.
    int         m_fileFormatVersionAtLoad;
    int         m_refCount;                 // Number of sheets referencing this screen.
                                            // Delete when it goes to zero.
    /**
     * The list of sheet paths sharing this screen.  Used in some annotation calculations to
     * update alternate references.
     *
     * Note: a screen having a m_refCount = 1 (only one sheet path using it) can have many
     * sheet paths sharing this screen if it is inside another sheet having many instances.
     */
    std::vector<SCH_SHEET_PATH> m_clientSheetPathList;


    PAGE_INFO   m_paper;                    // The size of the paper to print or plot on.
    TITLE_BLOCK m_titles;
    VECTOR2I    m_aux_origin;               // Origin used for drill & place files by Pcbnew.
    EE_RTREE    m_rtree;

    int         m_modification_sync;        // Inequality with SYMBOL_LIBS::GetModificationHash()
                                            // will trigger ResolveAll().

    bool        m_zoomInitialized;          // Set to true once the zoom value is initialized with
                                            // `InitZoom()`.

    bool        m_isReadOnly;               ///< Read only status of the screen file.

    /// Flag to indicate the file associated with this screen has been created.
    bool        m_fileExists;

    /// Library symbols required for this schematic.
    std::map<wxString, LIB_SYMBOL*> m_libSymbols;

    /**
     * The list of symbol instances loaded from the schematic file.
     *
     * This list is only used to as temporary storage when the schematic file is loaded.
     * If the screen is the root sheet, then this information is used to update the
     *  #SCH_SYMBOL instance reference and unit information after the entire schematic
     * is loaded and is never used again.  If this screen is not the root sheet, then the
     * schematic file is the root sheet of another project and this information is saved
     * unchanged back to the schematic file.
     *
     * @warning Under no circumstances is this information to be modified or used after the
     *          schematic file is loaded.  It is read only and it is only written to non-root
     *          schematic files.
     */
    std::vector<SCH_SYMBOL_INSTANCE> m_symbolInstances;
    std::vector<SCH_SHEET_INSTANCE> m_sheetInstances;

    /**
     * A unique identifier for each schematic file.
     *
     * As of right now, this only has meaning for the root schematic.  In the future, it may
     * be useful to detect unexpected hierarchy changes.
     */
    KIID m_uuid;
};


/**
 * Container class that holds multiple #SCH_SCREEN objects in a hierarchy.
 *
 * Individual #SCH_SCREEN objects are unique and correspond to .sch files.
 *
 * @note It may be desirable to fold the functionality of #SCH_SCREENS into the new #SCHEMATIC
 *       class at some point, since SCHEMATIC can also be thought of as owning the collection
 *       of all the #SCH_SCREEN objects.
 */
class SCH_SCREENS
{
public:
    SCH_SCREENS( SCH_SHEET* aSheet );
    SCH_SCREENS( SCH_SHEET& aSheet ) : SCH_SCREENS( &aSheet ) {}
    ~SCH_SCREENS();
    size_t GetCount() const { return m_screens.size(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int aIndex ) const;
    SCH_SHEET* GetSheet( unsigned int aIndex ) const;

    /**
     * Clear the annotation for the symbols inside new sheetpaths
     * when a complex hierarchy is modified and new sheetpaths added
     * when a screen shares more than one sheet path, missing alternate references are added
     * and alternate references of new sheet paths are cleared
     *
     * @param aInitialSheetPathList is the initial sheet paths list of hierarchy before changes.
     */
    void ClearAnnotationOfNewSheetPaths( SCH_SHEET_LIST& aInitialSheetPathList );

    /**
     * Test all sheet and symbol objects in the schematic for duplicate time stamps
     * and replaces them as necessary.
     *
     * Time stamps must be unique in order for complex hierarchies know which symbols go
     * to which sheets.
     *
     * @return The number of duplicate time stamps replaced.
     */
    int ReplaceDuplicateTimeStamps();

    /**
     * Delete all electronic rules check markers of \a aMarkerType from all the screens in
     * the list.
     *
     * @param aMarkerType Type of markers to be deleted.
     */
    void DeleteAllMarkers( enum MARKER_BASE::MARKER_T aMarkerType, bool aIncludeExclusions );

    /**
     * Delete all markers of a particular type and error code.
     */
    void DeleteMarkers( enum MARKER_BASE::MARKER_T aMarkerTyp, int aErrorCode,
                        bool aIncludeExclusions = true );

    /**
     * Delete a specific marker.
     */
    void DeleteMarker( SCH_MARKER* aMarker );

    /**
     * Initialize the #LIB_SYMBOL reference for each #SCH_SYMBOL found in the full schematic.
     *
     * @note This should only be called when the user specifically requests all library symbol
     *       links to be update or when the legacy schematic is opened for the last time.  All
     *       subsequent schematic loads with the new s-expression will contain the library
     *       symbols.
     *
     * @param[in] aReporter An optional #REPORTER object pointer to write warning and error
     *                      messages into.
     */
    void UpdateSymbolLinks( REPORTER* aReporter = nullptr );

    void ClearEditFlags();

    /**
     * Test all of the schematic symbols to see if all #LIB_ID objects library nickname is not
     * set.
     *
     * If none of the #LIB_ID object library nicknames are not set, this indicates that the
     * project was created before the symbol library implementation.
     *
     * @return true in there are any symbols and if all of the #LIB_ID object library nicknames
     *         are empty, otherwise false.
     */
    bool HasNoFullyDefinedLibIds();

    /**
     * Fetch all of the symbol library nicknames into \a aLibNicknames.
     *
     * @param[out] aLibNicknames is the array to populate with all of the unique library nicknames.
     * @return the number of symbol library nicknames found.
     */
    size_t GetLibNicknames( wxArrayString& aLibNicknames );

    /**
     * Change all of the symbol library nicknames.
     *
     * @param[in] aFrom the current symbol library name to change.
     * @param[in] aTo the new symbol library name.
     * @return the number of symbol library nicknames that were changed.
     */
    int ChangeSymbolLibNickname( const wxString& aFrom, const wxString& aTo );

    /**
     * Check if one of the schematics in the list of screens is \a aSchematicFileName.
     *
     * Schematic file names in SCH_SCREEN object are stored with the absolute path to
     * the schematic file.
     *
     * @param[in] aSchematicFileName is the schematic file name to search.
     * @return true if the a schematic matching the file name has been found.
     */
    bool HasSchematic( const wxString& aSchematicFileName );

    /**
     * Build the list of sheet paths sharing a screen for each screen in use.
     */
    void BuildClientSheetPathList();

    /**
     * Update the symbol value and footprint instance data for legacy designs.
     */
    void SetLegacySymbolInstanceData();

    /**
     * Fix legacy power symbols that have mismatched value text fields
     * and invisible power pin names.
     */
    void FixLegacyPowerSymbolMismatches();

    void PruneOrphanedSymbolInstances( const wxString& aProjectName,
                                       const SCH_SHEET_LIST& aValidSheetPaths );

    void PruneOrphanedSheetInstances( const wxString& aProjectName,
                                      const SCH_SHEET_LIST& aValidSheetPaths );

    bool HasSymbolFieldNamesWithWhiteSpace() const;

    std::set<wxString> GetVariantNames() const;

    void DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit = nullptr );

    void RenameVariant( const wxString& aOldName, const wxString& aNewName,
                        SCH_COMMIT* aCommit = nullptr );

    void CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                      SCH_COMMIT* aCommit = nullptr );

private:
    void addScreenToList( SCH_SCREEN* aScreen, SCH_SHEET* aSheet );
    void buildScreenList( SCH_SHEET* aSheet);

    std::vector< SCH_SCREEN* > m_screens;
    std::vector< SCH_SHEET* >  m_sheets;
    unsigned int               m_index;
};

#endif /* SCREEN_H */
