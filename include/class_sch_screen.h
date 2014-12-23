/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_sch_screen.h
 * @brief Definitions for the Eeschema program SCH_SCREEN class.
 */

#ifndef CLASS_SCREEN_H
#define CLASS_SCREEN_H

#include <macros.h>
#include <dlist.h>
#include <sch_item_struct.h>
#include <class_base_screen.h>
#include <class_title_block.h>
#include <class_page_info.h>
#include <kiway_player.h>

#include <../eeschema/general.h>


class LIB_PIN;
class SCH_COMPONENT;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_LINE;
class SCH_TEXT;
class PLOTTER;


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

    /// The size of the paper to print or plot on
    PAGE_INFO   m_paper;        // keep with the MVC 'model' if this class gets split

    TITLE_BLOCK m_titles;

    /// Origin of the auxilliary axis, which is used in exports mostly, but not yet in EESCHEMA
    wxPoint     m_aux_origin;

    DLIST< SCH_ITEM > m_drawList;       ///< Object list for the screen.

    int     m_modification_sync;        ///< inequality with PART_LIBS::GetModificationHash()
                                        ///< will trigger ResolveAll().

    /**
     * Function addConnectedItemsToBlock
     * add items connected at \a aPosition to the block pick list.
     * <p>
     * This method tests all connectible unselected items in the screen that are connected to
     * \a aPosition and adds them to the block selection pick list.  This is used when a block
     * drag is being performed to ensure connections to items in the block are not lost.
     *</p>
     * @param aPosition = The connection point to test.
     */
    void addConnectedItemsToBlock( const wxPoint& aPosition );

public:

    /**
     * Constructor
     */
    SCH_SCREEN( KIWAY* aKiway );

    ~SCH_SCREEN();

    virtual wxString GetClass() const
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
     * Function GetDrawItems().
     * @return - A pointer to the first item in the linked list of draw items.
     */
    SCH_ITEM* GetDrawItems() const                          { return m_drawList.begin(); }

    void Append( SCH_ITEM* aItem )
    {
        m_drawList.Append( aItem );
        --m_modification_sync;
    }

    /**
     * Function Append
     * adds \a aList of SCH_ITEM objects to the list for draw items for the sheet.
     *
     * @param aList A reference to a #DLIST containing the #SCH_ITEM to add to the sheet.
     */
    void Append( DLIST< SCH_ITEM >& aList )
    {
        m_drawList.Append( aList );
        --m_modification_sync;
    }

    /**
     * Function GetCurItem
     * returns the currently selected SCH_ITEM, overriding BASE_SCREEN::GetCurItem().
     * @return SCH_ITEM* - the one selected, or NULL.
     */
    SCH_ITEM* GetCurItem() const { return (SCH_ITEM*) BASE_SCREEN::GetCurItem(); }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from SCH_ITEM
     */
    void SetCurItem( SCH_ITEM* aItem ) { BASE_SCREEN::SetCurItem( (EDA_ITEM*) aItem ); }

    /**
     * Function Clear
     * deletes all draw items and clears the project settings.
     */
    void Clear();

    /**
     * Free all the items from the schematic associated with the screen.
     *
     * This does not delete any sub hierarchies.
     */
    void FreeDrawList();

    /**
     * Function GetItem
     * checks \a aPosition within a distance of \a aAccuracy for items of type \a aFilter.
     * @param aPosition Position in drawing units.
     * @param aAccuracy The maximum distance within \a Position to check for an item.
     * @param aType The type of item to find or #NOT_USED to find any item type.
     * @return The item found that meets the search criteria or NULL if none found.
     */
    SCH_ITEM* GetItem( const wxPoint& aPosition, int aAccuracy = 0,
                       KICAD_T aType = NOT_USED ) const;

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC ) { };

    /**
     * Initialize or reinitialize the pointer
     * to the LIB_PART for each component found in m_drawList
     * must be called:
     *  in Draw function
     *  when loading a schematic file
     *  before creating a netlist (in case a library is modified)
     */
    void BuildSchCmpLinksToLibCmp();

    /**
     * Function Draw
     * draws all the items in the screen to \a aCanvas.
     * note: this function is useful only for schematic.
     * library editor and library viewer do not use a draw list, and therefore
     * draws nothing
     * @param aCanvas The canvas item to draw on.
     * @param aDC The device context to draw on.
     * @param aDrawMode The drawing mode.
     * @param aColor The drawing color.
     */
    void Draw( EDA_DRAW_PANEL* aCanvas, wxDC* aDC, GR_DRAWMODE aDrawMode,
               EDA_COLOR_T aColor = UNSPECIFIED_COLOR );

    /**
     * Function Plot
     * plots all the schematic objects to \a aPlotter.
     * note: this function is useful only for schematic.
     * library editor and library viewer do not use a draw list, and therefore
     * plots nothing
     *
     * @param aPlotter The plotter object to plot to.
     */
    void Plot( PLOTTER* aPlotter );

    /**
     * Function Remove
     * removes \a aItem from the schematic associated with this screen.
     *
     * @note The removed item is not deleted.  It is only unlinked from the item list.
     * @param aItem Item to be removed from schematic.
     */
    void Remove( SCH_ITEM* aItem );

    /**
     * Function DeleteItem
     * removes \a aItem from the linked list and deletes the object.  If \a aItem is
     * is a schematic sheet label, it is removed from the screen associated with the
     * sheet that contains the label to be deleted.
     * @param aItem The schematic object to be deleted from the screen.
     */
    void DeleteItem( SCH_ITEM* aItem );

    bool CheckIfOnDrawList( SCH_ITEM* st );

    /**
     * Function SchematicCleanUp
     * performs routine schematic cleaning including breaking wire and buses and
     * deleting identical objects superimposed on top of each other.
     *
     * @param aCanvas The window to draw on.
     * @param aDC The device context used for drawing to \a aCanvas.
     * @return True if any schematic clean up was performed.
     */
    bool SchematicCleanUp( EDA_DRAW_PANEL* aCanvas = NULL, wxDC* aDC = NULL );

    /**
     * Function TestDanglingEnds
     * tests all of the connectible objects in the schematic for unused connection points.
     * @param aDC - The device context to draw the dangling status indicators.
     * @param aCanvas - The window to draw on.
     * @return True if any dangling ends were found.
     */
    bool TestDanglingEnds( EDA_DRAW_PANEL* aCanvas = NULL, wxDC* aDC = NULL );

    /**
     * Function ExtractWires
     * extracts the old wires, junctions and buses.  If \a aCreateCopy is true, replace
     * extracted items with a copy of the original.  Old items are to be put in undo list,
     * and the new ones can be modified by clean up safely.  If an abort draw segmat command
     * is made, the old wires must be put back into #m_drawList, and the copies must be
     * deleted.  This is because previously stored undo commands can handle pointers on wires
     * or buses, and we do not delete wires or buses, we must put them in undo list.
     *
     * Because cleanup deletes and/or modify bus and wires, it is easier is to put
     * all the existing  wires in undo list and use a new copy of wires for cleanup.
     */
    void ExtractWires( DLIST< SCH_ITEM >& aList, bool aCreateCopy );

    /**
     * Function ReplaceWires
     * replaces all of the wires, buses, and junctions in the screen with \a aWireList.
     *
     * @param aWireList List of wires to replace the existing wires with.
     */
    void ReplaceWires( DLIST< SCH_ITEM >& aWireList );

    /**
     * Function MarkConnections
     * add all wires and junctions connected to \a aSegment which are not connected any
     * component pin to \a aItemList.
     * @param aSegment The segment to test for connections.
     */
    void MarkConnections( SCH_LINE* aSegment );

    /**
     * Functions GetConnection
     * adds all of the wires and junctions to \a aList that make up a connection to the
     * object at \a aPosition.
     * @param aPosition The position of the first connection object in drawing units.
     * @param aList The pick list to add the connect item to.
     * @param aFullConnection If true all the objects that make up this connection are
     *                        add to \a aList.  Otherwise, only the objects up to the first
     *                        node are added.
     * @return The number of items added to \a aList.
     */
    int GetConnection( const wxPoint& aPosition, PICKED_ITEMS_LIST& aList, bool aFullConnection );

    /**
     * Function BreakSegment
     * checks every wire and bus for a intersection at \a aPoint and break into two segments
     * at \a aPoint if an intersection is found.
     * @param aPoint Test this point for an intersection.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegment( const wxPoint& aPoint );

    /**
     * Function BreakSegmentsOnJunctions
     * tests all junctions and bus entries in the schematic for intersections with wires and
     * buses and breaks any intersections into multiple segments.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegmentsOnJunctions();

    /* full undo redo management : */
    // use BASE_SCREEN::PushCommandToUndoList( PICKED_ITEMS_LIST* aItem )
    // use BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aItem )

    /**
     * Function ClearUndoORRedoList
     * free the undo or redo list from List element
     *  Wrappers are deleted.
     *  data pointed by wrappers are deleted if not in use in schematic
     *  i.e. when they are copy of a schematic item or they are no more in use (DELETED)
     * @param aList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    virtual void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to \a aFile in "*.sch" format.
     *
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Clear the state flags of all the items in the screen.
     */
    void ClearDrawingState();

    int CountConnectedItems( const wxPoint& aPos, bool aTestJunctions ) const;

    /**
     * Function IsJunctionNeeded
     * tests if a junction is required for the items at \a aPosition on the screen.
     * <p>
     * A junction is required at \a aPosition if the following criteria are satisfied:
     * <ul>
     * <li>one wire midpoint, one or more wire endpoints and no junction.</li>
     * <li>three or more wire endpoints and no junction.</li>
     * <li>two wire midpoints and no junction</li>
     * <li>one wire midpoint, a component pin, and no junction.</li>
     * <li>three wire endpoints, a component pin, and no junction.</li>
     * </ul>
     * </p>
     * @param aPosition The position to test.
     * @return True if a junction is required at \a aPosition.
     */
    bool IsJunctionNeeded( const wxPoint& aPosition );

    /**
     * Function IsTerminalPoint
     * tests if \a aPosition is a connection point on \a aLayer.
     *
     * @param aPosition Position to test.
     * @param aLayer The layer type to test against.  Valid layer types are #LAYER_NOTES,
     *               #LAYER_BUS, and #LAYER_WIRE.
     * @return True if \a Position is a connection point on \a aLayer.
     */
    bool IsTerminalPoint( const wxPoint& aPosition, int aLayer );

    /**
     * Function GetPin
     * test the screen for a component pin item at \a aPosition.
     * @param aPosition Position to test.
     * @param aComponent The component if a pin was found, otherwise NULL.
     * @param aEndPointOnly Set to true to test if \a aPosition is the connection
     *                      point of the pin.
     * @return The pin item if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxPoint& aPosition, SCH_COMPONENT** aComponent = NULL,
                     bool aEndPointOnly = false ) const;

    /**
     * Function GetSheet
     * returns a sheet object pointer that is named \a aName.
     *
     * @note The screen hierarchy is not descened.
     * @param aName is the case insensitive name of the sheet.
     * @return A pointer to the SCH_SHEET object found or NULL.
     */
    SCH_SHEET* GetSheet( const wxString& aName );

    /**
     * Function GetSheetLabel
     * test the screen if \a aPosition is a sheet label object.
     * @param aPosition The position to test.
     * @return The sheet label object if found otherwise NULL.
     */
    SCH_SHEET_PIN* GetSheetLabel( const wxPoint& aPosition );

    /**
     * Function ClearAnnotation
     * clears the annotation for the components in \a aSheetPath on the screen.
     * @param aSheetPath The sheet path of the component annotation to clear.  If NULL then
     *                   the entire hierarchy is cleared.
     */
    void ClearAnnotation( SCH_SHEET_PATH* aSheetPath );

    /**
     * Function GetHierarchicalItems
     * adds all schematic sheet and component object in the screen to \a aItems.
     * @param aItems Hierarchical item list to fill.
     */
    void GetHierarchicalItems( EDA_ITEMS& aItems );

    /**
     * Function GetNode
     * returns all the items at \a aPosition that form a node.
     *
     * @param aPosition The wxPoint to test for node items.
     * @param aList A #EDA_ITEMS container to place the items found.
     * @return The number of node items found at \a aPosition.
     */
    int GetNode( const wxPoint& aPosition, EDA_ITEMS& aList );

    /**
     * Function GetWireOrBus
     * returns a wire or bus item located at \a aPosition.
     *
     * @param aPosition The wxPoint to test for node items.
     * @return The SCH_LINE* of the wire or bus item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_LINE* GetWireOrBus( const wxPoint& aPosition );

    /**
     * Function GetLine
     * returns a line item located at \a aPosition.
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
     * Function GetLabel
     * returns a label item located at \a aPosition.
     *
     * @param aPosition The wxPoint to test for label items.
     * @param aAccuracy Amount to inflate the item hit test bounding box.
     * @return The SCH_TEXT* of the label item found at \a aPosition or NULL if item not
     *         found.
     */
    SCH_TEXT* GetLabel( const wxPoint& aPosition, int aAccuracy = 0 );

    /**
     * Function SetFootprintField
     * searches screen for a component with \a aReference and set the footprint field to
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
     * Function SelectBlockItems
     * creates a list of items found when a block command is initiated.  The items selected
     * depend on the block command.  If the drag block command is issued, than any items
     * connected to items in the block are also selected.
     */
    void SelectBlockItems();

    /**
     * Function UpdatePickList
     * adds all the items in the screen within the block selection rectangle to the pick list.
     * @return The number of items in the pick list.
     */
    int UpdatePickList();

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
#endif
};


/**
 * Class SCH_SCREENS
 * is a container class that holds multiple SCH_SCREENs in a hierarchy.
 * Individual SCH_SCREENs are unique, and correspond to .sch files.
 */
class SCH_SCREENS
{
private:
    std::vector< SCH_SCREEN* > m_screens;
    unsigned int               m_index;

public:
    SCH_SCREENS();
    ~SCH_SCREENS();
    int GetCount() const { return m_screens.size(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int aIndex ) const;

    /**
     * Function ClearAnnotation
     * clears the annotation for all components in the hierarchy.
     */
    void ClearAnnotation();

    /**
     * Function SchematicCleanUp
     * merges and breaks wire segments in the entire schematic hierarchy.
     */
    void SchematicCleanUp();

    /**
     * Function ReplaceDuplicateTimeStamps
     * test all sheet and component objects in the schematic for duplicate time stamps
     * an replaces them as necessary.  Time stamps must be unique in order for complex
     * hierarchies know which components go to which sheets.
     * @return The number of duplicate time stamps replaced.
     */
    int ReplaceDuplicateTimeStamps();

    /**
     * Function DeleteAllMarkers
     * deletes all electronic rules check markers of \a aMarkerType from all the screens in
     * the list.
     * @param aMarkerType Type of markers to be deleted.
     */
    void DeleteAllMarkers( int aMarkerType );

    /**
     * Function GetMarkerCount
     * returns the number of ERC markers of \a aMarkerType from all of the screens in the list.
     *
     * @param aMarkerType Indicates the type of marker to count.  A value less then zero
     *                    indicates all markers are counted.
     * @return int count of the markers found.
     */
    int GetMarkerCount( int aMarkerType = -1 );

private:
    void AddScreenToList( SCH_SCREEN* aScreen );
    void BuildScreenList( EDA_ITEM* aItem );
};

#endif /* CLASS_SCREEN_H */
