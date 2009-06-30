/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_DRAWSHEET_H
#define CLASS_DRAWSHEET_H

#include "base_struct.h"

extern DrawSheetStruct* g_RootSheet;

/* class Hierarchical_PIN_Sheet_Struct
 *  a Hierarchical_PIN_Sheet_Struct is for a hierarchical sheet like a pin for a component
 *  At root level, a Hierarchical_PIN_Sheet_Struct must be connected to a wire, bus or label
 *  A sheet level it corresponds to a hierarchical label.
 */
class Hierarchical_PIN_Sheet_Struct : public SCH_ITEM,
    public EDA_TextStruct
{
public:
    int  m_Edge, m_Shape;
    bool m_IsDangling;  // TRUE non connected
    int  m_Number;      // used to numbered labels when writing data on file . m_Number >= 2
                        // value 0 is for sheet name and 1 for sheet filename

public:
    Hierarchical_PIN_Sheet_Struct( DrawSheetStruct* parent,
                                   const wxPoint& pos = wxPoint( 0, 0 ),
                                   const wxString& text = wxEmptyString );

    ~Hierarchical_PIN_Sheet_Struct() { }

    virtual wxString GetClass() const
    {
        return wxT( "Hierarchical_PIN_Sheet_Struct" );
    }


    Hierarchical_PIN_Sheet_Struct* GenCopy();

    Hierarchical_PIN_Sheet_Struct* Next() { return (Hierarchical_PIN_Sheet_Struct*) Pnext; }

    void                           Place( WinEDA_SchematicFrame* frame, wxDC* DC );
    void                           Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                         int draw_mode, int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool                           Save( FILE* aFile ) const;

#if defined (DEBUG)

    // comment inherited by Doxygen from Base_Struct
    void                           Show( int nestLevel, std::ostream& os );

#endif

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    /** function CreateGraphicShape
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aCorner_list = list to fill with polygon corners coordinates
     * @param Pos = Position of the shape
     */
    void CreateGraphicShape( std::vector <wxPoint>& aCorner_list, const wxPoint& Pos );
};


/* class DrawSheetStruct
 * This class is the sheet symbol placed in a schematic, and is the entry point for a sub schematic
 */

class DrawSheetStruct : public SCH_ITEM
{
public:
    wxString m_SheetName;               /*this is equivalent to C101 for components:
                                         * it is stored in F0 ... of the file. */
private:
    wxString m_FileName;                /*also in SCH_SCREEN (redundant),
                                         * but need it here for loading after
                                         * reading the sheet description from file. */
public:
    int         m_SheetNameSize;                        /* Size (height) of the text, used to draw the sheet name */
    int         m_FileNameSize;                         /* Size (height) of the text, used to draw the file name */
    wxPoint     m_Pos;
    wxSize      m_Size;                                 /* Position and Size of sheet symbol */
    int         m_Layer;
    Hierarchical_PIN_Sheet_Struct* m_Label;             /* Points de connection, linked list.*/
    int         m_NbLabel;                              /* Pins sheet (corresponding to hierarchical labels) count */
    SCH_SCREEN* m_AssociatedScreen;             /* Associated Screen which handle the physical data
                                                 * In complex hierarchies we can have many DrawSheetStruct using the same data
                                                 */

public:
    DrawSheetStruct( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~DrawSheetStruct();
    virtual wxString GetClass() const
    {
        return wxT( "DrawSheetStruct" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool             Save( FILE* aFile ) const;

    void             Place( WinEDA_SchematicFrame* frame, wxDC* DC );
    DrawSheetStruct* GenCopy();
    void             DisplayInfo( WinEDA_DrawFrame* frame );

    /** Function CleanupSheet
     * Delete pinsheets which are not corresponding to a hierarchal label
     * @param aRedraw = true to redraw Sheet
     * @param aFrame = the schematic frame
     */
    void             CleanupSheet( WinEDA_SchematicFrame* frame, bool aRedraw );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    /** Function Draw
     *  Draw the hierarchical sheet shape
     *  @param aPanel = the current DrawPanel
     *  @param aDc = the current Device Context
     *  @param aOffset = draw offset (usually wxPoint(0,0))
     *  @param aDrawMode = draw mode
     *  @param aColor = color used to draw sheet. Usually -1 to use the normal color for sheet items
     */
    void             Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           int aDrawMode, int aColor = -1 );

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    /** Function GetBoundingBox
     *  @return an EDA_Rect giving the bouding box of the sheet
     */
    EDA_Rect         GetBoundingBox();

    void             SwapData( DrawSheetStruct* copyitem );

    /** Function ComponentCount
     *  count our own components, without the power components.
     *  @return the component count.
     */
    int              ComponentCount();

    /** Function Load.
     *  for the sheet: load the file m_FileName
     *  if a screen already exists, the file is already read.
     *  m_AssociatedScreen point on the screen, and its m_RefCount is incremented
     *  else creates a new associated screen and load the data file.
     *  @param aFrame = a WinEDA_SchematicFrame pointer to the maim schematic frame
     *  @return true if OK
     */
    bool             Load( WinEDA_SchematicFrame* aFrame );

    /** Function SearchHierarchy
     *  search the existing hierarchy for an instance of screen "FileName".
     *  @param aFilename = the filename to find
     *  @param aFilename = a location to return a pointer to the screen (if found)
     *  @return bool if found, and a pointer to the screen
     */
    bool             SearchHierarchy( wxString aFilename, SCH_SCREEN** aScreen );

    /** Function LocatePathOfScreen
     *  search the existing hierarchy for an instance of screen "FileName".
     *  don't bother looking at the root sheet - it must be unique,
     *  no other references to its m_AssociatedScreen otherwise there would be loops
     *  in the hierarchy.
     *  @param  aScreen = the SCH_SCREEN* screen that we search for
     *  @param aList = the DrawSheetPath*  that must be used
     *  @return true if found
     */
    bool             LocatePathOfScreen( SCH_SCREEN* aScreen, DrawSheetPath* aList );

    /** Function CountSheets
     * calculates the number of sheets found in "this"
     * this number includes the full subsheets count
     * @return the full count of sheets+subsheets contained by "this"
     */
    int              CountSheets();

    /** Function GetFileName
     * return the filename corresponding to this sheet
     * @return a wxString containing the filename
     */
    wxString         GetFileName( void );

    // Set a new filename without changing anything else
    void             SetFileName( const wxString& aFilename )
    {
        m_FileName = aFilename;
    }


    /** Function ChangeFileName
     * Set a new filename and manage data and associated screen
     * The main difficulty is the filename change in a complex hierarchy.
     * - if new filename is not already used: change to the new name (and if an existing file is found, load it on request)
     * - if new filename is already used (a complex hierarchy) : reference the sheet.
     * @param aFileName = the new filename
     * @param aFrame = the schematic frame
     */
    bool ChangeFileName( WinEDA_SchematicFrame* aFrame, const wxString& aFileName );

    //void      RemoveSheet(DrawSheetStruct* sheet);
    //to remove a sheet, just delete it
    //-- the destructor should take care of everything else.

#if defined (DEBUG)

    // comment inherited by Doxygen from Base_Struct
    void Show( int nestLevel, std::ostream& os );

#endif
};

#endif /* CLASS_DRAWSHEET_H */
