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


/* Type des labels sur sheet (Labels sur hierarchie) et forme des Global-Labels*/
typedef enum {
    NET_INPUT,
    NET_OUTPUT,
    NET_BIDI,
    NET_TRISTATE,
    NET_UNSPECIFIED,
    NET_TMAX        /* Derniere valeur: fin de tableau */
} TypeSheetLabel;

/* Messages correspondants aux types ou forme des labels */
#ifdef MAIN
const char*        SheetLabelType[] =
{
    "Input",
    "Output",
    "BiDi",
    "3State",
    "UnSpc",
    "?????"
};
#else
extern const char* SheetLabelType[];
#endif

/* Description du graphisme des icones associes aux types des Global_Labels */
#ifdef MAIN
int TemplateIN_HN[] = { 6, 0, 0, -1, -1, -2, -1, -2, 1, -1, 1, 0, 0 };
int TemplateIN_HI[] = { 6, 0, 0, 1, 1, 2, 1, 2, -1, 1, -1, 0, 0 };
int TemplateIN_BOTTOM[] = { 6, 0, 0, 1, -1, 1, -2, -1, -2, -1, -1, 0, 0 };
int TemplateIN_UP[] = { 6, 0, 0, 1, 1, 1, 2, -1, 2, -1, 1, 0, 0 };
    
int TemplateOUT_HN[] = { 6, -2, 0, -1, 1, 0, 1, 0, -1, -1, -1, -2, 0 };
int TemplateOUT_HI[] = { 6, 2, 0, 1, -1, 0, -1, 0, 1, 1, 1, 2, 0 };
int TemplateOUT_BOTTOM[] = { 6, 0, -2, 1, -1, 1, 0, -1, 0, -1, -1, 0, -2 };
int TemplateOUT_UP[] = { 6, 0, 2, 1, 1, 1, 0, -1, 0, -1, 1, 0, 2 };
    
int TemplateUNSPC_HN[] = { 5, 0, -1, -2, -1, -2, 1, 0, 1, 0, -1 };
int TemplateUNSPC_HI[] = { 5, 0, -1, 2, -1, 2, 1, 0, 1, 0, -1 };
int TemplateUNSPC_BOTTOM[] = { 5, 1, 0, 1, -2, -1, -2, -1, 0, 1, 0 };
int TemplateUNSPC_UP[] = { 5, 1, 0, 1, 2, -1, 2, -1, 0, 1, 0 };
    
int TemplateBIDI_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
int TemplateBIDI_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
int TemplateBIDI_BOTTOM[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
int TemplateBIDI_UP[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };
    
int Template3STATE_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
int Template3STATE_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
int Template3STATE_BOTTOM[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
int Template3STATE_UP[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };
    
int* TemplateShape[5][4] =
{
    { TemplateIN_HN,     TemplateIN_UP,     TemplateIN_HI,     TemplateIN_BOTTOM     },
    { TemplateOUT_HN,    TemplateOUT_UP,    TemplateOUT_HI,    TemplateOUT_BOTTOM    },
    { TemplateBIDI_HN,   TemplateBIDI_UP,   TemplateBIDI_HI,   TemplateBIDI_BOTTOM   },
    { Template3STATE_HN, Template3STATE_UP, Template3STATE_HI, Template3STATE_BOTTOM },
    { TemplateUNSPC_HN,  TemplateUNSPC_UP,  TemplateUNSPC_HI,  TemplateUNSPC_BOTTOM  }
};
#else
extern int* TemplateShape[5][4];
#endif


/* Forward declarations */
class DrawSheetStruct;


class SCH_SCREEN : public BASE_SCREEN
{
public:
    SCH_SCREEN( int idtype, KICAD_T aType = SCREEN_STRUCT_TYPE );
    ~SCH_SCREEN();
    virtual wxString GetClass() const
    {
        return wxT("SCH_SCREEN");
    }
    

    void            FreeDrawList(); // Free EESchema drawing list (does not delete the sub hierarchies)

    void Place( WinEDA_DrawFrame* frame, wxDC* DC ) { };
    void            RemoveFromDrawList( EDA_BaseStruct* DrawStruct );/* remove DrawStruct from EEDrawList. */
    void            ClearUndoORRedoList( EDA_BaseStruct* List );

    bool            SchematicCleanUp( wxDC* DC = NULL );
    EDA_BaseStruct* ExtractWires( bool CreateCopy );

    /* full undo redo management : */
    virtual void    ClearUndoRedoList();
    virtual void    AddItemToUndoList( EDA_BaseStruct* item );
    virtual void    AddItemToRedoList( EDA_BaseStruct* item );
};


class DrawSheetLabelStruct : public EDA_BaseStruct, public EDA_TextStruct
{
public:
    int  m_Layer;
    int  m_Edge, m_Shape;
    bool m_IsDangling;  // TRUE si non connecté

public:
    DrawSheetLabelStruct( DrawSheetStruct* parent,
       const wxPoint& pos = wxPoint( 0, 0 ), 
       const wxString& text = wxEmptyString );
    
    ~DrawSheetLabelStruct() { }
    virtual wxString GetClass() const
    {
        return wxT("DrawSheetLabelStruct");
    }
    
    DrawSheetLabelStruct*   GenCopy();

    DrawSheetLabelStruct* Next()
    { return (DrawSheetLabelStruct*) Pnext; }
    
    void                    Place( WinEDA_DrawFrame* frame, wxDC* DC );
    virtual void            Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                  int draw_mode, int Color = -1 );
};


class DrawSheetStruct : public SCH_SCREEN    /* Gestion de la hierarchie */
{
public:
    wxString m_SheetName;
    int      m_SheetNameSize;

//	wxString m_FileName; in SCH_SCREEN
    int      m_FileNameSize;
    wxPoint  m_Pos;
    wxSize   m_Size;                    /* Position and Size of sheet symbol */
    int      m_Layer;
    DrawSheetLabelStruct* m_Label;      /* Points de connection */
    int      m_NbLabel;                 /* Nombre de points de connexion */

public:
    DrawSheetStruct( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~DrawSheetStruct();
    virtual wxString GetClass() const
    {
        return wxT("DrawSheetStruct");
    }
    
    void                Place( WinEDA_DrawFrame* frame, wxDC* DC );
    DrawSheetStruct*    GenCopy();
    void                Display_Infos( WinEDA_DrawFrame* frame );
    void                CleanupSheet( WinEDA_SchematicFrame* frame, wxDC* DC );
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
    void                SwapData( DrawSheetStruct* copyitem );
};


/* Class to handle the list of screens in a hierarchy */
class EDA_ScreenList
{
private:
    int          m_Count;
    SCH_SCREEN** m_List;
    int          m_Index;

public:
    EDA_ScreenList( EDA_BaseStruct* DrawStruct );
    ~EDA_ScreenList();
    int GetCount() { return m_Count; }
    SCH_SCREEN*     GetFirst();
    SCH_SCREEN*     GetNext();
    SCH_SCREEN*     GetScreen( int index );
    void            UpdateSheetNumberAndDate();

private:
    SCH_SCREEN**    BuildScreenList( SCH_SCREEN** ScreenList,
                                     EDA_BaseStruct* DrawStruct, int* Count );
};

#endif /* CLASS_SCREEN_H */
