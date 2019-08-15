/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <unordered_set>
#include <macros.h>
#include <dlist.h>
#include <sch_item.h>
#include <lib_item.h>
#include <base_screen.h>
#include <title_block.h>
#include <page_info.h>
#include <kiway_holder.h>
#include <sch_marker.h>
#include <bus_alias.h>


class LIB_PIN;
class SCH_COMPONENT;
class SCH_SHEET_LIST;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_LINE;
class SCH_TEXT;
class PLOTTER;
class SCH_SHEET_LIST;


enum SCH_LINE_TEST_T
{
    ENTIRE_LENGTH_T,
    END_POINTS_ONLY_T,
    EXCLUDE_END_POINTS_T
};


/// Max number of sheets in a hierarchy project
#define NB_MAX_SHEET    500


class SCH_SCREEN : public BASE_SCREEN, public KIWAY_HOLDER
{
private:

    wxString    m_fileName;     ///< File used to load the screen.

    int         m_refCount;     ///< Number of sheets referencing this screen.
                                ///< Delete when it goes to zero.

    /** the list of scheet paths sharing this screen
     * used in some annotation calculations to update alternate references
     * Note: a screen having a m_refCount = 1 (only one sheet path using it)
     * can have many scheet paths sharing this screen, if this sheet is inside
     * an other sheet having many instances (one sheet path by parent sheet instance).
     */
    wxArrayString m_clientSheetPathList;

    /// The size of the paper to print or plot on
    PAGE_INFO   m_paper;        // keep with the MVC 'model' if this class gets split

    TITLE_BLOCK m_titles;

    /// Origin of the auxiliary axis, which is used in exports mostly, but not yet in EESCHEMA
    wxPoint     m_aux_origin;

    DLIST< SCH_ITEM > m_drawList;       ///< Object list for the screen.

    int     m_modification_sync;        ///< inequality with PART_LIBS::GetModificationHash()
                                        ///< will trigger ResolveAll().

    /// List of bus aliases stored in this screen
    std::unordered_set< std::shared_ptr< BUS_ALIAS > > m_aliases;

public:

    /**
     * Constructor
     */
    SCH_SCREEN( KIWAY* aKiway );

    ~SCH_SCREEN();

    DLIST< SCH_ITEM > & GetDrawList() { return m_drawList; }

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_SCREEN" );
    }

    const PAGE_INFO& GetPageSettings() const                { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    void SetFileName( const wxString& aFileName )           { m_fileName = aFileName; }

    const wxString& GetFileName() const                     { return m_fileName; }

    const wxPoint& GetAuxOrigin() const                     { return m_aux_origin; }
    void SetAuxOrigin( const wxPoint& aPosition )           { m_aux_origin = aPosition; }

    const TITLE_BLOCK& GetTitleBlock() const                { return m_titles; }
    //TITLE_BLOCK& GetTitleBlock() const                      { return (TITLE_BLOCK&) m_titles; }
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )    { m_titles = aTitleBlock; }

    void DecRefCount();

    void IncRefCount();

    int GetRefCount() const                                 { return m_refCount; }

    /**
     * @return the sheet paths count sharing this screen
     * if 1 this screen is not in a complex hierarchy: the reference field can be
     * used to store the component reference
     * if > 1 this screen is in a complex hierarchy, and components must have
     * a full alternate reference management
     */
    int GetClientSheetPathsCount() { return (int) m_clientSheetPathList.GetCount(); }

    wxArrayString& GetClientSheetPaths() { return m_clientSheetPathList; }

    /**
     * @return A pointer to the first item in the linked list of draw items.
     */
    SCH_ITEM* GetDrawItems() const                          { return m_drawList.begin(); }

    void Append( SCH_ITEM* aItem )
    {
        m_drawList.Append( aItem );
        --m_modification_sync;
    }

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
     * Add \a aList of SCH_ITEM objects to the list for draw items for the sheet.
     *
     * @param aList A reference to a #DLIST containing the #SCH_ITEM to add to the sheet.
     */
    void Append( DLIST< SCH_ITEM >& aList )
    {
        m_drawList.Append( aList );
        --m_modification_sync;
    }

    /**
     * Delete all draw items and clears the project settings.
     */
    void Clear();

    /**
     * Free all the items from the schematic associated with the screen.
     *
     * This does not delete any sub hierarchies.
     */
    void FreeDrawList();

    /**
     * Check \a aPosition within a distance of \a aAccuracy for items of type \a aFilter.
     *
     * @param aPosition Position in drawing units.
     * @param aAccuracy The maximum distance within \a Position to check for an item.
     * @param aType The type of item to find.
     * @return The item found that meets the search criteria or NULL if none found.
     */
    SCH_ITEM* GetItem( const wxPoint& aPosition, int aAccuracy = 0,
                       KICAD_T aType = SCH_LOCATE_ANY_T ) const;

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC ) { };

    /**
     * Initialize or reinitialize the weak reference to the #LIB_PART for each #SCH_COMPONENT
     * found in m_drawList.
     *
     * It must be called from:
     * - Draw function
     * - when loading a schematic file
     * - before creating a netlist (in case a library is modified)
     * - whenever a symbol library is modified
     * - whenever the symbol library table is modified.
     *
     * @param aForce true forces a refresh even if the library modification has hasn't changed.
     */
    void UpdateSymbolLinks( bool aForce = false );

    /**
     * Print all the items in the screen to \a aDC.
     *
     * @note This function is useful only for schematic.  The library editor and library viewer
     *       do not use a draw list and therefore draws nothing.
     *
     * @param aDC The device context to draw on.
     */
    void Print( wxDC* aDC );

    /**
     * Plot all the schematic objects to \a aPlotter.
     *
     * @note This function is useful only for schematic. The library editor and library viewer
     *       do not use a draw list and therefore plots nothing.
     *
     * @param aPlotter The plotter object to plot to.
     */
    void Plot( PLOTTER* aPlotter );

    /**
     * Remove \a aItem from the schematic associated with this screen.
     *
     * @note The removed item is not deleted.  It is only unlinked from the item list.
     * @param aItem Item to be removed from schematic.
     */
    void Remove( SCH_ITEM* aItem );

    /**
     * Removes \a aItem from the linked list and deletes the object.
     *
     * If \a aItem is a schematic sheet label, it is removed from the screen associated with
     * the sheet that contains the label to be deleted.
     *
     * @param aItem The schematic object to be deleted from the screen.
     */
    void DeleteItem( SCH_ITEM* aItem );

    bool CheckIfOnDrawList( SCH_ITEM* st );

    /**
     * Test all of the connectable objects in the schematic for unused connection points.
     * @return True if any connection state changes were made.
     */
    bool TestDanglingEnds();

    /**
     * Replace all of the wires, buses, and junctions in the screen with \a aWireList.
     *
     * @param aWireList List of wires to replace the existing wires with.
     */
    void ReplaceWires( DLIST< SCH_ITEM >& aWireList );

    /**
     * Add all wires and junctions connected to \a aSegment which are not connected any
     * component pin to \a aItemList.
     *
     * @param aSegment The segment to test for connections.
     */
    void MarkConnections( SCH_LINE* aSegment );

    /* full undo redo management : */
    // use BASE_SCREEN::PushCommandToUndoList( PICKED_ITEMS_LIST* aItem )
    // use BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aItem )

    /**
     * Free the undo or redo list from \a aList element.
     *
     * - Wrappers are deleted.
     * - data pointed by wrappers are deleted if not in use in schematic
     *   i.e. when they are copy of a schematic item or they are no more in use (DELETED)
     *
     * @param aList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    virtual void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 ) override;

    /**
     * Clear the state flags of all the items in the screen.
     */
    void ClearDrawingState();

    int CountConnectedItems( const wxPoint& aPos, bool aTestJunctions ) const;

    /**
     * Test if a junction is required for the items at \a aPosition on the screen.
     * <p>
     * A junction is required at \a aPosition if one of the following criteria is satisfied:
     * <ul>
     * <li>one wire midpoint and one or more wire endpoints;</li>
     * <li>three or more wire endpoints;</li>
     * <li>one wire midpoint and a component pin;</li>
     * <li>two or more wire endpoints and a component pin.</li>
     * </ul>
     * </p>
     * @param aPosition The position to test.
     * @param aNew Checks if a _new_ junction is needed, i.e. there isn't one already
     * @return True if a junction is required at \a aPosition.
     */
    bool IsJunctionNeeded( const wxPoint& aPosition, bool aNew = false );

    /**
     * Test if \a aPosition is a connection point on \a aLayer.
     *
     * @param aPosition Position to test.
     * @param aLayer The layer type to test against.  Valid layer types are #LAYER_NOTES,
     *               #LAYER_BUS, and #LAYER_WIRE.
     * @return True if \a Position is a connection point on \a aLayer.
     */
    bool IsTerminalPoint( const wxPoint& aPosition, int aLayer );

    /**
     * Test the screen for a component pin item at \a aPosition.
     *
     * @param aPosition Position to test.
     * @param aComponent The component if a pin was found, otherwise NULL.
     * @param aEndPointOnly Set to true to test if \a aPosition is the connection
     *                      point of the pin.
     * @return The pin item if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxPoint& aPosition, SCH_COMPONENT** aComponent = NULL,
                     bool aEndPointOnly = false ) const;

    /**
     * Returns a sheet object pointer that is named \a aName.
     *
     * @note The screen hierarchy is not descend.
     * @param aName is the case insensitive name of the sheet.
     * @return A pointer to the SCH_SHEET object found or NULL.
     */
    SCH_SHEET* GetSheet( const wxString& aName );

    /**
     * Test the screen if \a aPosition is a sheet label object.
     *
     * @param aPosition The position to test.
     * @return The sheet label object if found otherwise NULL.
     */
    SCH_SHEET_PIN* GetSheetLabel( const wxPoint& aPosition );

    /**
     * Clear the annotation for the components in \a aSheetPath on the screen.
     *
     * @param aSheetPath The sheet path of the component annotation to clear.  If NULL then
     *                   the entire hierarchy is cleared.
     */
    void ClearAnnotation( SCH_SHEET_PATH* aSheetPath );

    /**
     * For screens shared by many sheetpaths (complex hierarchies):
     * to be able to clear or modify any reference related  sharing this screen
     * (i.e. thie list of components), an entry for each screen path must exist.
     * This function creates missing entries, using as default reference the current
     * reference field and unit number
     * Note: m_clientSheetPathList must be up to date
     * ( built by SCH_SCREENS::BuildClientSheetPathList() )
     */
    void EnsureAlternateReferencesExist();

    /**
     * Add all schematic sheet and component objects in the screen to \a aItems.
     *
     * @param aItems Hierarchical item list to fill.
     */
    void GetHierarchicalItems( EDA_ITEMS& aItems );

    /**
     * Return a wire or bus item located at \a aPosition.
     *
     * @param aPosition The wxPoint to test for node items.
     * @return The SCH_LINE* of the wire or bus item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_LINE* GetWireOrBus( const wxPoint& aPosition );

    /**
     * Return a line item located at \a aPosition.
     *
     * @param aPosition The wxPoint to test for a line item.
     * @param aAccuracy Amount to inflate the item hit test bounding box.
     * @param aLayer The layer the line is drawn upon.
     * @param aSearchType Additional line test criteria.
     * @return The SCH_LINE* of the wire item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_LINE* GetLine( const wxPoint& aPosition, int aAccuracy = 0, int aLayer = LAYER_NOTES,
                       SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T );

    SCH_LINE* GetWire( const wxPoint& aPosition, int aAccuracy = 0,
                       SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T )
    {
        return GetLine( aPosition, aAccuracy, LAYER_WIRE, aSearchType );
    }

    SCH_LINE* GetBus( const wxPoint& aPosition, int aAccuracy = 0,
                      SCH_LINE_TEST_T aSearchType = ENTIRE_LENGTH_T )
    {
        return GetLine( aPosition, aAccuracy, LAYER_BUS, aSearchType );
    }

    /**
     * Return a label item located at \a aPosition.
     *
     * @param aPosition The wxPoint to test for label items.
     * @param aAccuracy Amount to inflate the item hit test bounding box.
     * @return The SCH_TEXT* of the label item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_TEXT* GetLabel( const wxPoint& aPosition, int aAccuracy = 0 );

    /**
     * Search this screen for a symbol with \a aReference and set the footprint field to
     * \a aFootPrint if found.
     *
     * @param aSheetPath The sheet path used to look up the reference designator.
     * @param aReference The reference designator of the component.
     * @param aFootPrint The value to set the footprint field.
     * @param aSetVisible The value to set the field visibility flag.
     * @return True if \a aReference was found otherwise false.
     */
    bool SetComponentFootprint( SCH_SHEET_PATH* aSheetPath, const wxString& aReference,
                                const wxString& aFootPrint, bool aSetVisible );

    /**
     * Adds a bus alias definition (and transfers ownership of the pointer)
     */
    void AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    /**
     * Removes all bus alias definitions
     */
    void ClearBusAliases()
    {
        m_aliases.clear();
    }

    /**
     * Returns a list of bus aliases defined in this screen
     */
    std::unordered_set< std::shared_ptr<BUS_ALIAS> > GetBusAliases()
    {
        return m_aliases;
    }

    /**
     * Returns true if the given string is a valid bus alias in a loaded screen
     */
    static bool IsBusAlias( const wxString& aLabel );

    /**
     * Returns a pointer to a bus alias object for the given label,
     * or null if one doesn't exist
     */
    static std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aLabel );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif
};


/**
 * Container class that holds multiple #SCH_SCREEN objects in a hierarchy.
 *
 * Individual #SCH_SCREEN objects are unique and correspond to .sch files.
 */
class SCH_SCREENS
{
private:
    std::vector< SCH_SCREEN* > m_screens;
    unsigned int               m_index;

public:
    SCH_SCREENS( SCH_SHEET* aSheet = NULL );
    ~SCH_SCREENS();
    int GetCount() const { return m_screens.size(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int aIndex ) const;

    /**
     * Clear the annotation for all components in the hierarchy.
     */
    void ClearAnnotation();

    /**
     * Clear the annotation for the components inside new sheetpaths
     * when a complex hierarchy is modified and new sheetpaths added
     * when a screen shares more than one sheet path, missing alternate references are added
     * and alternate references of new sheet paths are cleared
     *
     * @param aInitialSheetPathList is the initial sheet paths list of hierarchy before changes.
     */
    void ClearAnnotationOfNewSheetPaths( SCH_SHEET_LIST& aInitialSheetPathList );

    /**
     * Test all sheet and component objects in the schematic for duplicate time stamps
     * and replaces them as necessary.
     * Time stamps must be unique in order for complex hierarchies know which components go
     * to which sheets.
     * @return The number of duplicate time stamps replaced.
     */
    int ReplaceDuplicateTimeStamps();

    /**
     * Delete all electronic rules check markers of \a aMarkerType from all the screens in
     * the list.
     * @param aMarkerType Type of markers to be deleted.
     */
    void DeleteAllMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType );

    /**
     * Return the number of ERC markers of \a aMarkerType from all of the screens in the list.
     *
     * @param aMarkerType Indicates the type of marker to count. if MARKER_UNSPEC
     *                    all markers are counted.
     * @param aSeverity   Indicates the error level of marker to count.
     *                  useMARKER_SEVERITY_UNSPEC to count all markersof the specified type
     * @return int count of the markers found.
     */
    int GetMarkerCount( enum MARKER_BASE::TYPEMARKER aMarkerType,
                        enum MARKER_BASE::MARKER_SEVERITY aSeverity );

    /**
     * Initialize or reinitialize the weak reference to the #LIB_PART for each #SCH_COMPONENT
     * found in the full schematic.
     *
     * It must be called from:
     * - draw functions
     * - when loading a schematic file
     * - before creating a netlist (in case a library is modified)
     * - whenever any of the libraries are modified.
     * - whenever the symbol library table is modified.
     */
    void UpdateSymbolLinks( bool aForce = false );

    void TestDanglingEnds();

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
     * Fetch all of the symbol library nickames into \a aLibNicknames.
     *
     * @param aLibNicknames is the array to populate with all of the unique library nicknames.
     * @return the number of symbol library nicknames found.
     */
    size_t GetLibNicknames( wxArrayString& aLibNicknames );

    /**
     * Change all of the symbol library nicknames.
     *
     * @param aFrom the current symbol library name to change.
     * @param aTo the new symbol library name.
     * @return the number of symbol library nicknames that were changed.
     */
    int ChangeSymbolLibNickname( const wxString& aFrom, const wxString& aTo );

    /**
     * Check if one of the schematics in the list of screens is \a aSchematicFileName.
     *
     * Schematic file names in SCH_SCREEN object are stored with the absolute path to
     * the schematic file.
     *
     * @param aSchematicFileName is the schematic file name to search.
     * @return true if the a schematic matching the file name has been found.
     */
    bool HasSchematic( const wxString& aSchematicFileName );

    /**
     * built the list of sheet paths sharing a screen for each screen in use
     */
    void BuildClientSheetPathList();


private:
    void addScreenToList( SCH_SCREEN* aScreen );
    void buildScreenList( SCH_SHEET* aSheet);
};

#endif /* SCREEN_H */
