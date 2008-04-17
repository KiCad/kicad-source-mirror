/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_DRAWSHEET_H
#define CLASS_DRAWSHEET_H

#ifndef eda_global
#define eda_global extern
#endif

#include "base_struct.h"

extern DrawSheetStruct* g_RootSheet;


class Hierarchical_PIN_Sheet_Struct : public SCH_ITEM,
    public EDA_TextStruct
{
public:
    int  m_Edge, m_Shape;
    bool m_IsDangling;  // TRUE non connected
    int m_Number;       // used to numbered labels when writing data on file . m_Number >= 2
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


    Hierarchical_PIN_Sheet_Struct*   GenCopy();

    Hierarchical_PIN_Sheet_Struct* Next()
    { return (Hierarchical_PIN_Sheet_Struct*) Pnext; }

    void                    Place( WinEDA_SchematicFrame* frame, wxDC* DC );
    void                    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                  int draw_mode, int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;
};


/* class DrawSheetStruct
  * This class is the sheet symbol placed in a schematic, and is the entry point for a sub schematic
 */
WX_DEFINE_ARRAY( DrawSheetStruct *, SheetGrowArray );

class DrawSheetStruct : public SCH_ITEM /*public SCH_SCREEN*/    /* Gestion de la hierarchie */
{
public:
    wxString              m_SheetName;  //this is equivalent to C101 for components:
    // it is stored in F0 ... of the file.
private:
    wxString              m_FileName;   //also in SCH_SCREEN (redundant),
                                        //but need it here for loading after
                                        //reading the sheet description from file.
public:
    int                   m_SheetNameSize;	// Size (height) of the text, used to draw the name

    int                   m_FileNameSize;	// Size (height) of the text, used to draw the name
    wxPoint               m_Pos;
    wxSize                m_Size;           /* Position and Size of sheet symbol */
    int                   m_Layer;
    Hierarchical_PIN_Sheet_Struct* m_Label;          /* Points de connection, linked list.*/
    int                   m_NbLabel;        /* Nombre de points de connexion */
    SCH_SCREEN*           m_AssociatedScreen;   /* Associated Screen which handle the physical data
                                                  * In complex hierarchies we can have many DrawSheetStruct using the same data
                                                 */
    int                   m_SheetNumber;    // sheet number (used for info)
    int                   m_NumberOfSheets; // Sheets count in the whole schematic (used for info)

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
    bool    Save( FILE* aFile ) const;

    void                Place( WinEDA_SchematicFrame* frame, wxDC* DC );
    DrawSheetStruct*    GenCopy();
    void                Display_Infos( WinEDA_DrawFrame* frame );
    void                CleanupSheet( WinEDA_SchematicFrame* frame, wxDC* DC );
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
    EDA_Rect            GetBoundingBox();
    void                SwapData( DrawSheetStruct* copyitem );
    void                DeleteAnnotation( bool recurse );
    int                 ComponentCount();
    bool                Load( WinEDA_SchematicFrame* frame );
    bool                SearchHierarchy( wxString filename, SCH_SCREEN** screen );
    bool                LocatePathOfScreen( SCH_SCREEN* screen, DrawSheetPath* list );
    int                 CountSheets();
    wxString            GetFileName(void);
    void                SetFileName(const wxString & aFilename); // Set a new filename without changing anything else
    bool                ChangeFileName(WinEDA_SchematicFrame * aFrame, const wxString & aFileName); // Set a new filename and manage data and associated screen

    //void 		RemoveSheet(DrawSheetStruct* sheet);
    //to remove a sheet, just delete it
    //-- the destructor should take care of everything else.
};


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
#define DSLSZ 32    // Max number of levels for a sheet path
class DrawSheetPath
{
public:
    int m_numSheets;
    DrawSheetStruct* m_sheets[DSLSZ];

    DrawSheetPath();
    ~DrawSheetPath() { };
    void                Clear() { m_numSheets = 0; }
    int                 Cmp( DrawSheetPath& d );
    DrawSheetStruct*    Last();
    SCH_SCREEN*         LastScreen();
    EDA_BaseStruct*     LastDrawList();
    void                Push( DrawSheetStruct* sheet );
    DrawSheetStruct*    Pop();
    /** Function Path
    * the path uses the time stamps which do not changes even when editing sheet parameters
    * a path is something like / (root) or /34005677 or /34005677/00AE4523
    */
    wxString            Path();
    /** Function PathHumanReadable
    * Return the sheet path in a readable form, i.e.
    * as a path made from sheet names.
    * (the "normal" path uses the time stamps which do not changes even when editing sheet parameters)
    */
    wxString            PathHumanReadable();
    /** Function UpdateAllScreenReferences
     * Update the reference and the m_Multi parameter (part selection) for all components on a screen
     * depending on the actual sheet path.
     * Mandatory in complex hierarchies because sheets use the same screen (basic schematic)
     * but with different references and part selection according to the displayed sheet
    */
    void                UpdateAllScreenReferences();

    bool operator       =( const DrawSheetPath& d1 );

    bool operator       ==( const DrawSheetPath& d1 );

    bool operator       !=( const DrawSheetPath& d1 );
};


/*******************************************************/
/* Class to handle the list of *Sheets* in a hierarchy */
/*******************************************************/

// sheets are not unique - can have many sheets with the same
// filename and the same SCH_SCREEN reference.
class EDA_SheetList
{
private:
    DrawSheetPath* m_List;
    int            m_count;     /* Number of sheets included in hierarchy,
                                  * starting at the given sheet in constructor . the given sheet is counted
                                 */
    int            m_index;
    DrawSheetPath  m_currList;

public:
    EDA_SheetList( DrawSheetStruct* sheet )
    {
        m_index = 0;
        m_count = 0;
        m_List  = NULL;
        if( sheet == NULL )
            sheet = g_RootSheet;
        BuildSheetList( sheet );
    }


    ~EDA_SheetList()
    {
        if( m_List )
        {
            free( m_List );
        }
        m_List = NULL;
    }


    int GetCount() { return m_count; }
    DrawSheetPath*  GetFirst();
    DrawSheetPath*  GetNext();
    DrawSheetPath*  GetSheet( int index );

private:
    void            BuildSheetList( DrawSheetStruct* sheet );
};

#endif /* CLASS_DRAWSHEET_H */
