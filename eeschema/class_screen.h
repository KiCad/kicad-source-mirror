/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_SCREEN_H
#define CLASS_SCREEN_H

#ifndef eda_global
#define eda_global extern
#endif

#include "macros.h"
#include "base_struct.h"

/* Max number of sheets in a hierarchy project: */
#define NB_MAX_SHEET 500


/* Forward declarations */
class DrawSheetStruct;
extern DrawSheetStruct* g_RootSheet;


class SCH_SCREEN : public BASE_SCREEN
{
public:
    int m_RefCount; //how many sheets reference this screen?
                    //delete when it goes to zero.
    int m_ScreenNumber;
    int m_NumberOfScreen;
    SCH_SCREEN( int idtype, KICAD_T aType = SCREEN_STRUCT_TYPE );
    ~SCH_SCREEN();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_SCREEN" );
    }


    void            FreeDrawList(); // Free EESchema drawing list (does not delete the sub hierarchies)

    void Place( WinEDA_DrawFrame* frame, wxDC* DC ) { };
    void            RemoveFromDrawList( EDA_BaseStruct* DrawStruct ); /* remove DrawStruct from EEDrawList. */
    bool            CheckIfOnDrawList( EDA_BaseStruct* st );
    void            AddToDrawList( EDA_BaseStruct* DrawStruct );
    void            ClearUndoORRedoList( EDA_BaseStruct* List );

    bool            SchematicCleanUp( wxDC* DC = NULL );
    EDA_BaseStruct* ExtractWires( bool CreateCopy );

    /* full undo redo management : */
    virtual void    ClearUndoRedoList();
    virtual void    AddItemToUndoList( EDA_BaseStruct* item );
    virtual void    AddItemToRedoList( EDA_BaseStruct* item );
};


class DrawSheetLabelStruct : public EDA_BaseStruct,
    public EDA_TextStruct
{
public:
    int  m_Layer;
    int  m_Edge, m_Shape;
    bool m_IsDangling;  // TRUE non connected

public:
    DrawSheetLabelStruct( DrawSheetStruct* parent,
                          const wxPoint& pos = wxPoint( 0, 0 ),
                          const wxString& text = wxEmptyString );

    ~DrawSheetLabelStruct() { }
    virtual wxString GetClass() const
    {
        return wxT( "DrawSheetLabelStruct" );
    }


    DrawSheetLabelStruct*   GenCopy();

    DrawSheetLabelStruct* Next()
    { return (DrawSheetLabelStruct*) Pnext; }

    void                    Place( WinEDA_DrawFrame* frame, wxDC* DC );
    virtual void            Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                  int draw_mode, int Color = -1 );
};


/* class DrawSheetStruct
  * This class is the sheet symbol placed in a schematic, and is the entry point for a sub schematic
 */
WX_DEFINE_ARRAY( DrawSheetStruct *, SheetGrowArray );

class DrawSheetStruct : public EDA_BaseStruct /*public SCH_SCREEN*/    /* Gestion de la hierarchie */
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
    DrawSheetLabelStruct* m_Label;          /* Points de connection, linked list.*/
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


    void                Place( WinEDA_DrawFrame* frame, wxDC* DC );
    DrawSheetStruct*    GenCopy();
    void                Display_Infos( WinEDA_DrawFrame* frame );
    void                CleanupSheet( WinEDA_SchematicFrame* frame, wxDC* DC );
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
    void                SwapData( DrawSheetStruct* copyitem );
    void                DeleteAnnotation( bool recurse );
    int                 ComponentCount();
    bool                Load( WinEDA_SchematicFrame* frame );
    bool                SearchHierarchy( wxString filename, SCH_SCREEN** screen );
    bool                LocatePathOfScreen( SCH_SCREEN* screen, DrawSheetPath* list );
    int                 CountSheets();
	wxString			GetFileName(void);
	void				SetFileName(const wxString & aFilename);

    //void 		RemoveSheet(DrawSheetStruct* sheet);
    //to remove a sheet, just delete it
    //-- the destructor should take care of everything else.
};


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
#define DSLSZ 32
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
    wxString            Path();
    wxString            PathHumanReadable();
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

/********************************************************/
/* Class to handle the list of *screens* in a hierarchy */
/********************************************************/

// screens are unique, and correspond to .sch files.
WX_DEFINE_ARRAY( SCH_SCREEN *, ScreenGrowArray );
class EDA_ScreenList
{
private:
    ScreenGrowArray m_List;
    unsigned int    m_Index;

public:
    EDA_ScreenList()
    {
        m_Index = 0;
        BuildScreenList( g_RootSheet );
    }


    ~EDA_ScreenList() { }
    int GetCount() { return m_List.GetCount(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int index );

private:
    void            AddScreenToList( SCH_SCREEN* testscreen );
    void            BuildScreenList( EDA_BaseStruct* sheet );
};

#endif /* CLASS_SCREEN_H */
