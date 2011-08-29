/********************************************/
/* Definitions for the EESchema program:    */
/********************************************/

#ifndef CLASS_DRAWSHEET_H
#define CLASS_DRAWSHEET_H

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>
#include "sch_text.h"


class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_SHEET_PATH;
class DANGLING_END_ITEM;
class SCH_EDIT_FRAME;


#define MIN_SHEET_WIDTH  500
#define MIN_SHEET_HEIGHT 150


/**
 * Pin (label) used in sheets to create hierarchical schematics.
 *
 * A SCH_SHEET_PIN is used to create a hierarchical sheet in the same way a
 * pin is used in a component.  It connects the objects in the sheet object
 * to the objects in the schematic page to the objects in the page that is
 * represented by the sheet.  In a sheet object, a SCH_SHEET_PIN must be
 * connected to a wire, bus, or label.  In the schematic page represented by
 * the sheet, it corresponds to a hierarchical label.
 */

class SCH_SHEET_PIN : public SCH_HIERLABEL
{
private:
    int m_Number;       ///< Label number use for saving sheet label to file.
                        ///< Sheet label numbering begins at 2.
                        ///< 0 is reserved for the sheet name.
                        ///< 1 is reserve for the sheet file name.
    int m_Edge;         /* For pin labels only: sheet edge (0 to 3) of the pin
                         * m_Edge define on which edge the pin is positioned:
                         *        0: pin on left side
                         *        1: pin on right side
                         *        2: pin on top side
                         *        3: pin on bottom side
                         *  for compatibility reasons, this does not follow same values as text
                         *  orientation.
                         */

    virtual EDA_ITEM* doClone() const;

    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;

public:
    SCH_SHEET_PIN( SCH_SHEET* parent,
                   const wxPoint& pos = wxPoint( 0, 0 ),
                   const wxString& text = wxEmptyString );

    SCH_SHEET_PIN( const SCH_SHEET_PIN& aSheetLabel );

    ~SCH_SHEET_PIN() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_SHEET_PIN" );
    }

    bool operator ==( const SCH_SHEET_PIN* aPin ) const;

    virtual void Draw( EDA_DRAW_PANEL* aPanel,
                       wxDC*           aDC,
                       const wxPoint&  aOffset,
                       int             aDraw_mode,
                       int             aColor = -1 );

    /**
     * Function CreateGraphicShape (virtual)
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aPoints = a buffer to fill with polygon corners coordinates
     * @param aPos = Position of the shape
     */
    virtual void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos );

    virtual void SwapData( SCH_ITEM* aItem );

    /**
     * Get the sheet label number.
     *
     * @return Number of the sheet label.
     */
    int GetNumber() const { return m_Number; }

    /**
     * Set the sheet label number.
     *
     * @param aNumber - New sheet number label.
     */
    void SetNumber( int aNumber );

    void SetEdge( int aEdge );

    int GetEdge() const;

    /**
     * Function ConstraintOnEdge
     * is used to adjust label position to edge based on proximity to vertical / horizontal edge
     * of the parent sheet.
     */
    void ConstraintOnEdge( wxPoint Pos );

    /**
     * Get the parent sheet object of this sheet pin.
     *
     * @return The sheet that is the parent of this sheet pin or NULL if it does
     *         not have a parent.
     */
    SCH_SHEET* GetParent() const { return (SCH_SHEET*) m_Parent; }

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic sheet hierarchical label from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read the sheet hierarchical label  from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the sheet
     *                    hierarchical label.
     * @return True if the sheet hierarchical label loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

#if defined(DEBUG)

    // comment inherited by Doxygen from Base_Struct
    void Show( int nestLevel, std::ostream& os );

#endif

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual void Mirror_X( int aXaxis_position );

    /**
     * Function Matches
     * Compare hierarchical pin name against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @param aAuxData - a pointer on auxiliary data, if needed.
     *        When searching string in REFERENCE field we must know the sheet path
     *          This param is used in this case
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if this item matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsConnectable() const { return true; }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_hierar_pin_xpm; }
};


typedef boost::ptr_vector<SCH_SHEET_PIN> SCH_SHEET_PINS;


/* class SCH_SHEET
 * This class is the sheet symbol placed in a schematic, and is the entry point
 * for a sub schematic
 */

class SCH_SHEET : public SCH_ITEM
{
    SCH_SCREEN* m_AssociatedScreen;   ///< Screen that contains the physical data for
                                      ///< the sheet.  In complex hierarchies multiple
                                      ///< sheets can share a common screen.
    SCH_SHEET_PINS m_pins;            ///< List of sheet connection points.
    wxString m_FileName;              /* also in SCH_SCREEN (redundant),
                                       * but need it here for loading after
                                       * reading the sheet description from
                                       * file. */

public:
    wxString m_SheetName;             /* this is equivalent to C101 for
                                       * components: it is stored in F0 ...
                                       * of the file. */
public:
    int         m_SheetNameSize;        /* Size (height) of the text, used to
                                         * draw the sheet name */
    int         m_FileNameSize;         /* Size (height) of the text, used to
                                         * draw the file name */
    wxPoint     m_Pos;
    wxSize      m_Size;                 /* Position and Size of *sheet symbol */

public:
    SCH_SHEET( const wxPoint& pos = wxPoint( 0, 0 ) );

    SCH_SHEET( const SCH_SHEET& aSheet );

    ~SCH_SHEET();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_SHEET" );
    }

    SCH_SCREEN* GetScreen() { return m_AssociatedScreen; }

    /**
     * Function SetScreen
     * sets the screen associated with this sheet to \a aScreen.
     * <p>
     * The screen reference counting is performed by SetScreen.  If \a aScreen is not
     * the same as the current screen, the current screen reference count is decremented
     * and \a aScreen becomes the screen for the sheet.  If the current screen reference
     * count reaches zero, the current screen is deleted.  NULL is a valid value for
     * \a aScreen.
     * </p>
     * @param aScreen The new screen to associate with the sheet.
     */
    void SetScreen( SCH_SCREEN* aScreen );

    /**
     * Function GetScreenCount
     * returns the number of times the associated screen for the sheet is being used.  If
     * no screen is associated with the sheet, then zero is returned.
     */
    int GetScreenCount() const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic sheet from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read the component from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the sheet.
     * @return True if the sheet loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    void Place( SCH_EDIT_FRAME* frame, wxDC* DC );

    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /* there is no member for orientation in sch_sheet, to preserve file
     * format, we detect orientation based on pin edges
     */
    bool IsVerticalOrientation() const;

    /**
     * Add aSheetPin to the sheet.
     *
     * Note: Once a sheet pin is added to the sheet, it is owned by the sheet.
     *       Do not delete the sheet pin object or you will likely get a segfault
     *       when the sheet is destroyed.
     *
     * @param aSheetPin The sheet pin item to add to the sheet.
     */
    void AddPin( SCH_SHEET_PIN* aSheetPin );

    SCH_SHEET_PINS& GetPins() { return m_pins; }

    SCH_SHEET_PINS& GetPins() const
    {
        return const_cast< SCH_SHEET_PINS& >( m_pins );
    }

    /**
     * Remove \a aSheetPin from the sheet.
     *
     * @param aSheetPin The sheet pin item to remove from the sheet.
     */
    void RemovePin( SCH_SHEET_PIN* aSheetPin );

    /**
     * Delete sheet label which do not have a corresponding hierarchical label.
     *
     * Note: Make sure you save a copy of the sheet in the undo list before calling
     *       CleanupSheet() otherwise any unreferenced sheet labels will be lost.
     */
    void CleanupSheet();

    /**
     * Return the sheet pin item found at \a aPosition in the sheet.
     *
     * @param aPosition The position to check for a sheet pin.
     *
     * @return The sheet pin found at \a aPosition or NULL if no sheet pin is found.
     */
    SCH_SHEET_PIN* GetPin( const wxPoint& aPosition );

    /**
     * Checks if the sheet already has a sheet pin named \a aName.
     *
     * @param aName Name of the sheet pin to search for.
     *
     * @return  True if sheet pin with \a aName is found, otherwise false.
     */
    bool HasPin( const wxString& aName );

    bool HasPins() { return !m_pins.empty(); }

    /**
     * Check all sheet labels against schematic for undefined hierarchical labels.
     *
     * @return True if there are any undefined labels.
     */
    bool HasUndefinedPins();

    /**
     * Function GetMinWidth
     * returns the minimum width of the sheet based on the widths of the sheet pin text.
     *
     * <p>
     * The minimum sheet width is determined by the width of the bounding box of each
     * hierarchical sheet pin.  If two pins are horizontally adjacent ( same Y position )
     * to each other, the sum of the bounding box widths is used.  If at some point in
     * the future sheet objects can be rotated or pins can be placed in the vertical
     * orientation, this function will need to be changed.
     * </p>
     *
     * @return The minimum width the sheet can be resized.
     */
    int GetMinWidth() const;

    /**
     * Function GetMinHeight
     * returns the minimum height that the sheet can be resized based on the sheet pin
     * positions.
     *
     * <p>
     * The minimum width of a sheet is determined by the Y axis location of the bottom
     * most sheet pin.  If at some point in the future sheet objects can be rotated or
     * pins can be placed in the vertical orientation, this function will need to be
     * changed.
     * </p>
     *
     * @return The minimum height the sheet can be resized.
     */
    int GetMinHeight() const;

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Draw
     *  Draw the hierarchical sheet shape
     *  @param aPanel = the current DrawPanel
     *  @param aDC = the current Device Context
     *  @param aOffset = draw offset (usually wxPoint(0,0))
     *  @param aDrawMode = draw mode
     *  @param aColor = color used to draw sheet. Usually -1 to use the normal
     * color for sheet items
     */
    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               const wxPoint&  aOffset,
               int             aDrawMode,
               int             aColor = -1 );

    /**
     * Function GetBoundingBox
     *  @return an EDA_RECT giving the bounding box of the sheet
     */
    EDA_RECT GetBoundingBox() const;

    /**
     * Function GetResizePos
     * returns the position of the lower right corner of the sheet in drawing units.
     *
     * @return A wxPoint containing lower right corner of the sheet in drawing units.
     */
    wxPoint GetResizePosition() const;

    virtual void SwapData( SCH_ITEM* aItem );

    /**
     * Function ComponentCount
     *  count our own components, without the power components.
     *  @return the component count.
     */
    int ComponentCount();

    /**
     * Function Load.
     *  for the sheet: load the file m_FileName
     *  if a screen already exists, the file is already read.
     *  m_AssociatedScreen point on the screen, and its m_RefCount is
     * incremented
     *  else creates a new associated screen and load the data file.
     *  @param aFrame = a SCH_EDIT_FRAME pointer to the maim schematic frame
     *  @return true if OK
     */
    bool Load( SCH_EDIT_FRAME* aFrame );

    /**
     * Function SearchHierarchy
     *  search the existing hierarchy for an instance of screen "FileName".
     *  @param aFilename = the filename to find
     *  @param aScreen = a location to return a pointer to the screen (if found)
     *  @return bool if found, and a pointer to the screen
     */
    bool SearchHierarchy( const wxString& aFilename, SCH_SCREEN** aScreen );

    /**
     * Function LocatePathOfScreen
     *  search the existing hierarchy for an instance of screen "FileName".
     *  don't bother looking at the root sheet - it must be unique,
     *  no other references to its m_AssociatedScreen otherwise there would be
     *  loops
     *  in the hierarchy.
     *  @param  aScreen = the SCH_SCREEN* screen that we search for
     *  @param aList = the SCH_SHEET_PATH*  that must be used
     *  @return true if found
     */
    bool LocatePathOfScreen( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aList );

    /**
     * Function CountSheets
     * calculates the number of sheets found in "this"
     * this number includes the full subsheets count
     * @return the full count of sheets+subsheets contained by "this"
     */
    int CountSheets();

    /**
     * Function GetFileName
     * return the filename corresponding to this sheet
     * @return a wxString containing the filename
     */
    wxString GetFileName( void ) const;

    // Set a new filename without changing anything else
    void SetFileName( const wxString& aFilename )
    {
        m_FileName = aFilename;
    }

    bool ChangeFileName( SCH_EDIT_FRAME* aFrame, const wxString& aFileName );

    //void      RemoveSheet(SCH_SHEET* sheet);
    //to remove a sheet, just delete it
    //-- the destructor should take care of everything else.

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;

        BOOST_FOREACH( SCH_SHEET_PIN& pin, m_pins )
        {
            pin.Move( aMoveVector );
        }
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

    /**
     * Compare schematic sheet file and sheet name against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @param aAuxData - a pointer on auxiliary data, if needed.
     *        When searching string in REFERENCE field we must know the sheet path
     *          This param is used in this case
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     *
     * @return True if this item matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    /**
     * Resize this sheet to aSize and adjust all of the labels accordingly.
     *
     * @param aSize - The new size for this sheet.
     */
    void Resize( const wxSize& aSize );

    /**
     * Function GetSheetNamePosition
     * @return the position of the anchor of sheet name text
     */
    wxPoint GetSheetNamePosition();

    /**
     * Function GetFileNamePosition
     * @return the position of the anchor of filename text
     */
    wxPoint GetFileNamePosition();

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const;

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual bool IsConnectable() const { return true; }

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    virtual SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                                 const KICAD_T scanTypes[] );

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return add_hierarchical_subsheet_xpm; }

#if defined(DEBUG)

    // comment inherited by Doxygen from Base_Struct
    void Show( int nestLevel, std::ostream& os );

#endif

protected:

    /**
     * Renumber the sheet pins in the sheet.
     *
     * This method is used internally by SCH_SHEET to update the pin numbering
     * when the pin list changes.  Make sure you call this method any time a
     * sheet pin is added or removed.
     */
    void renumberPins();

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
};


typedef std::vector< SCH_SHEET* > SCH_SHEETS;

#endif /* CLASS_DRAWSHEET_H */
