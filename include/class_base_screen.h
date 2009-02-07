/**********************
*   class_base_screen.h
**********************/

/* define :
 *  class DrawBlockStruct used to handle block commands
 *  class BASE_SCREEN to handle how to draw a screen (a board, a schematic ...)
 */

#ifndef  __CLASS_BASE_SCREEN_H__
#define  __CLASS_BASE_SCREEN_H__

#include "base_struct.h"


// Forward declarations:
class SCH_ITEM;
class Ki_PageDescr;


/**************************/
/*  class DrawBlockStruct */
/**************************/
/* Definition d'un block pour les fonctions sur block (block move, ..) */
typedef enum {
    /* definition de l'etat du block */
    STATE_NO_BLOCK,             /* Block non initialise */
    STATE_BLOCK_INIT,           /* Block initialise: 1er point defini */
    STATE_BLOCK_END,            /* Block initialise: 2eme point defini */
    STATE_BLOCK_MOVE,           /* Block en deplacement */
    STATE_BLOCK_STOP            /* Block fixe (fin de deplacement) */
} BlockState;

/* codes des differentes commandes sur block: */
typedef enum {
    BLOCK_IDLE,
    BLOCK_MOVE,
    BLOCK_COPY,
    BLOCK_SAVE,
    BLOCK_DELETE,
    BLOCK_PASTE,
    BLOCK_DRAG,
    BLOCK_ROTATE,
    BLOCK_INVERT,
    BLOCK_ZOOM,
    BLOCK_ABORT,
    BLOCK_PRESELECT_MOVE,
    BLOCK_SELECT_ITEMS_ONLY,
    BLOCK_MIRROR_X,
    BLOCK_MIRROR_Y
} CmdBlockType;


class DrawBlockStruct : public EDA_BaseStruct, public EDA_Rect
{
public:
    BlockState      m_State;        /* Etat (enum BlockState) du block */
    CmdBlockType    m_Command;      /* Type (enum CmdBlockType) d'operation */
    EDA_BaseStruct* m_BlockDrawStruct;  /* pointeur sur la structure
                                         *   selectionnee dans le bloc */
    int             m_Color;        /* Block Color */
    wxPoint         m_MoveVector;   /* Move distance in move, drag, copy ... command */
    wxPoint         m_BlockLastCursorPosition; /* Last Mouse position in block command
                                                *  = last cursor position in move commands
                                                *  = 0,0 in block paste */

public:
    DrawBlockStruct();
    ~DrawBlockStruct();
    void SetMessageBlock( WinEDA_DrawFrame* frame );
    void Draw( WinEDA_DrawPanel* panel, wxDC* DC );
};


/* Simple class for handling grid arrays. */
class GRID_TYPE
{
public:
    int    m_Id;
    wxRealPoint m_Size;
};


/* Declare array of wxSize for grid list implementation. */
#include <wx/dynarray.h>
WX_DECLARE_OBJARRAY( GRID_TYPE, GridArray );


/*******************************************************************/
/* Class to handle how to draw a screen (a board, a schematic ...) */
/*******************************************************************/
class BASE_SCREEN : public EDA_BaseStruct
{
public:
    wxPoint m_DrawOrg;                          /* offsets pour tracer le circuit sur l'ecran */
    wxPoint m_Curseur;                          /* Screen cursor coordinate (on grid) in user units. */
    wxPoint m_MousePosition;                    /* Mouse cursor coordinate (off grid) in user units. */
    wxPoint m_MousePositionInPixels;            /* Mouse cursor coordinate (off grid) in pixels. */
    wxPoint m_O_Curseur;                        /* Relative Screen cursor coordinate (on grid) in user units.
                                                 * (coordinates from last reset position)*/
    wxPoint m_ScrollbarPos;                     // Position effective des Curseurs de scroll
    wxSize  m_ScrollbarNumber;                  /* Valeur effective des Nombres de Scrool
                                                 * c.a.d taille en unites de scroll de la surface totale affichable */
    wxPoint m_StartVisu;                        // Coord absolues du 1er pixel visualis�a l'ecran (en nombre de pixels)

    wxSize  m_SizeVisu;                 /* taille en pixels de l'ecran (fenetre de visu
                                         * Utile pour recadrer les affichages lors de la
                                         * navigation dans la hierarchie */
    bool    m_Center;                   /* fix the coordinate (0,0) position on screen : if TRUE (0,0) in centered on screen
                                         *  TRUE: when coordiantaes can be < 0 and > 0   all but schematic
                                         *  FALSE: when coordinates can be only >= 0    Schematic
                                         */
    bool            m_FirstRedraw;

    /* Gestion des editions */
    SCH_ITEM*       EEDrawList;             /* Object list (main data) for schematic */
    EDA_BaseStruct* m_UndoList;             /* Object list for the undo command (old data) */
    EDA_BaseStruct* m_RedoList;             /* Object list for the redo command (old data) */
    int             m_UndoRedoCountMax;     /* undo/Redo command Max depth */

    /* block control */
    DrawBlockStruct BlockLocate;    /* Bock description for block commands */

    /* Page description */
    Ki_PageDescr*   m_CurrentSheetDesc;
    int             m_ScreenNumber;
    int             m_NumberOfScreen;

    wxString        m_FileName;
    wxString        m_Title;            /* titre de la feuille */
    wxString        m_Date;             /* date de mise a jour */
    wxString        m_Revision;         /* code de revision */
    wxString        m_Company;          /* nom du proprietaire */
    wxString        m_Commentaire1;
    wxString        m_Commentaire2;
    wxString        m_Commentaire3;
    wxString        m_Commentaire4;

private:
    /* indicateurs divers */
    char            m_FlagRefreshReq;       /* indique que l'ecran doit redessine */
    char            m_FlagModified;         // indique modif du PCB,utilise pour eviter une sortie sans sauvegarde
    char            m_FlagSave;             // indique sauvegarde auto faite
    EDA_BaseStruct* m_CurrentItem;          ///< Currently selected object

    /* Valeurs du pas de grille et du zoom */
public:
    wxRealPoint     m_Grid;             /* Current grid. */
    GridArray  m_GridList;
    bool       m_UserGridIsON;

    wxArrayInt m_ZoomList;          /* Array of standard zoom coefficients. */
    int        m_Zoom;              /* Current zoom coefficient. */
    int        m_ZoomScalar;        /* Allow zooming to non-integer increments. */

public:
    BASE_SCREEN( KICAD_T aType = SCREEN_STRUCT_TYPE );
    ~BASE_SCREEN();

    BASE_SCREEN* Next() const { return (BASE_SCREEN*) Pnext; }
    BASE_SCREEN* Back() const { return (BASE_SCREEN*) Pback; }


    /**
     * Function setCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param current Any object derived from EDA_BaseStruct
     */
    void SetCurItem( EDA_BaseStruct* current ) {  m_CurrentItem = current; }
    EDA_BaseStruct* GetCurItem() const { return m_CurrentItem; }

    void                    InitDatas();    /* Inits completes des variables */

    wxSize                  ReturnPageSize( void );
    virtual int             GetInternalUnits( void );

    /** Function CursorRealPosition
     * @return the position in user units of location ScreenPos
     * @param ScreenPos = the screen (in pixel) position co convert
     */
    wxPoint                 CursorRealPosition( const wxPoint& ScreenPos );

    /* general Undo/Redo command control */
    virtual void            ClearUndoRedoList();
    virtual void            AddItemToUndoList( EDA_BaseStruct* item );
    virtual void            AddItemToRedoList( EDA_BaseStruct* item );
    virtual EDA_BaseStruct* GetItemFromUndoList();
    virtual EDA_BaseStruct* GetItemFromRedoList();

    /* Manipulation des flags */
    void    SetRefreshReq() { m_FlagRefreshReq = 1; }
    void    ClrRefreshReq() { m_FlagRefreshReq = 0; }
    void    SetModify() { m_FlagModified = 1; m_FlagSave = 0; }
    void    ClrModify() { m_FlagModified = 0; m_FlagSave = 1; }
    void    SetSave() { m_FlagSave = 1; }
    void    ClrSave() { m_FlagSave = 0; }
    int     IsModify() { return m_FlagModified & 1;  }
    int     IsRefreshReq() { return m_FlagRefreshReq & 1;  }
    int     IsSave() { return m_FlagSave & 1;  }


    //----<zoom stuff>----------------------------------------------------------

    /** Function GetScalingFactor
     * @return the the current scale used to draw items on screen
     * draw coordinates are user coordinates * GetScalingFactor( )
     */
    double  GetScalingFactor()
    {
        return (double) m_ZoomScalar / GetZoom();
    }


    /** Function SetScalingFactor
     * @param the the current scale used to draw items on screen
     * draw coordinates are user coordinates * GetScalingFactor( )
     */
    void   SetScalingFactor( double aScale );

    /** Function GetZoom
     * @return the current zoom factor
     * Note: the zoom factor is NOT the scaling factor
     *       the scaling factor is m_ZoomScalar * GetZoom()
     */
    int    GetZoom() const;

    /**
     * Function SetZoom
     * adjusts the current zoom factor
     */
    void   SetZoom( int coeff );

    /**
     * Function SetZoomList
     * sets the list of zoom factors.
     * @param aZoomList An array of zoom factors in ascending order, zero terminated
     */
    void   SetZoomList( const wxArrayInt& zoomlist );

    int    Scale( int coord );
    void   Scale( wxPoint& pt );
    void   Scale( wxSize& sz );
    void   Scale( wxRealPoint& sz );

    int    Unscale( int coord );
    void   Unscale( wxPoint& pt );
    void   Unscale( wxSize& sz );

    void   SetNextZoom();                   /* ajuste le prochain coeff de zoom */
    void   SetPreviousZoom();               /* ajuste le precedent coeff de zoom */
    void   SetFirstZoom();                  /* ajuste le coeff de zoom a 1*/
    void   SetLastZoom();                   /* ajuste le coeff de zoom au max */

    //----<grid stuff>----------------------------------------------------------
    wxRealPoint GetGrid();                      /* retourne la grille */
    void   SetGrid( const wxRealPoint& size );
    void   SetGrid( int );
    void   SetGridList( GridArray& sizelist );
    void   AddGrid( const GRID_TYPE& grid );
    void   AddGrid( const wxRealPoint& size, int id );
    void   AddGrid( const wxRealPoint& size, int units, int id );


    /**
     * Function RefPos
     * returns the reference position, coming from either the mouse position or the
     * the cursor position.
     * @param useMouse If true, return mouse position, else cursor's.
     * @return wxPoint - The reference point, either the mouse position or
     *   the cursor position.
     */
    wxPoint RefPos( bool useMouse )
    {
        return useMouse ? m_MousePosition : m_Curseur;
    }


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "BASE_SCREEN" );
    }


#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


#endif  /* #ifndef __CLASS_BASE_SCREEN_H__ */
