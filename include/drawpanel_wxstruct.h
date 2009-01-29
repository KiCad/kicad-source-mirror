/********************************************************/
/* drawpanel_wxstruct.h:				*/
/* descriptions des principales classes utilisees:	*/
/*	ici classe: "WinEDA_DrawPanel",  "BASE_SCREEN"	*/
/********************************************************/

/* Doit etre inclus dans "wxstruch.h"
 */

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H


#ifndef eda_global
 #define eda_global extern
#endif

#include "colors.h"

class SCH_ITEM;
class BASE_SCREEN;
class Ki_PageDescr;

/* Simple class for handling grid arrays. */
class GRID_TYPE
{
public:
    int    m_Id;
    wxSize m_Size;
};


/* Declare array of wxSize for grid list implementation. */
#include <wx/dynarray.h>
WX_DECLARE_OBJARRAY( GRID_TYPE, GridArray );


/****************************************************/
/* classe representant un ecran graphique de dessin */
/****************************************************/

class WinEDA_DrawPanel : public wxScrolledWindow
{
public:
    WinEDA_DrawFrame* m_Parent;
    EDA_Rect          m_ClipBox;            // the clipbox used in screen redraw (usually gives the visible area in internal units)
    wxPoint           m_CursorStartPos;     // utile dans controles du mouvement curseur
    int               m_ScrollButt_unit;    //	Valeur de l'unite de scroll en pixels pour les boutons de scroll

    bool              m_AbortRequest;       // Flag d'arret de commandes longues
    bool              m_AbortEnable;        // TRUE si menu ou bouton Abort doit etre affiche

    bool              m_AutoPAN_Enable;     // TRUE to allow auto pan
    bool              m_AutoPAN_Request;    // TRUE to request an auto pan (will be made only if m_AutoPAN_Enable = true)

    int               m_IgnoreMouseEvents;  //  when non-zero (true), then ignore mouse events

    bool              m_Block_Enable;       // TRUE to accept Block Commands
    int               m_CanStartBlock;      // >= 0 (or >= n) if a block can start
    bool              m_PrintIsMirrored;    // True when drawing in mirror mode. Used in draw arc function,
                                            // because arcs are oriented, and in mirror mode, orientations are reversed
    // usefull to avoid false start block in certain cases (like switch from a sheet to an other scheet
    int               m_PanelDefaultCursor; // Current mouse cursor default shape id for this window
    int               m_PanelCursor;        // Current mouse cursor shape id for this window
    int               m_CursorLevel;        // Index for cursor redraw in XOR mode

    /* Cursor management (used in editing functions) */
    void              (*ManageCurseur)( WinEDA_DrawPanel* panel, wxDC* DC, bool erase ); /* Fonction d'affichage sur deplacement souris
                                                                                         *  si erase : effacement ancien affichage */
    void              (*ForceCloseManageCurseur)( WinEDA_DrawPanel* panel, wxDC* DC ); /* Fonction de fermeture forc�
                                                                                       *  de la fonction ManageCurseur */

public:

    // Constructor and destructor
    WinEDA_DrawPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos, const wxSize& size );
    ~WinEDA_DrawPanel() { }

    /****************************/
    BASE_SCREEN* GetScreen();


    void         PrepareGraphicContext( wxDC* DC );
    bool         IsPointOnDisplay( wxPoint ref_pos );
    void         OnPaint( wxPaintEvent& event );
    void         OnSize( wxSizeEvent& event );
    void         SetBoundaryBox();
    void         ReDraw( wxDC* DC, bool erasebg = TRUE );
    void         PrintPage( wxDC* DC, bool Print_Sheet_Ref, int PrintMask, bool aPrintMirrorMode );
    void         DrawBackGround( wxDC* DC );
    void         m_Draw_Auxiliary_Axis( wxDC* DC, int drawmode );
    void         OnEraseBackground( wxEraseEvent& event );
    void         OnActivate( wxActivateEvent& event );

    /* Mouse and keys events */
    void         OnMouseWheel( wxMouseEvent& event );
    void         OnMouseEvent( wxMouseEvent& event );
    void         OnMouseLeaving( wxMouseEvent& event );
    void         OnKeyEvent( wxKeyEvent& event );

    void         OnPan( wxCommandEvent& event );

    /*************************/

    void         EraseScreen( wxDC* DC );
    void         OnScrollWin( wxCommandEvent& event );
    void         OnScroll( wxScrollWinEvent& event );

    void         SetZoom( int mode );
    int          GetZoom();
    void         SetGrid( const wxSize& size );
    wxSize       GetGrid();

    void         AddMenuZoom( wxMenu* MasterMenu );
    bool         OnRightClick( wxMouseEvent& event );
    void         OnPopupGridSelect( wxCommandEvent& event );
    void         Process_Special_Functions( wxCommandEvent& event );
    wxPoint      CursorRealPosition( const wxPoint& ScreenPos );
    wxPoint      CursorScreenPosition();

    /**
     * Function PostDirtyRect
     * appends the given rectangle in pcb units to the DrawPanel's invalid
     * region list so that very soon (but not immediately), this rectangle
     * along with any other recently posted rectangles is redrawn.  Conversion
     * to pixels is done in here.
     * @param aRect The rectangle to append, it must be orthogonal
     *   (vertical and horizontal edges only), and it must be [,) in nature, i.e.
     *   [pos, dim) == [inclusive, exclusive)
     */
    void         PostDirtyRect( EDA_Rect aRect );

    /**
     * Function ConvertPcbUnitsToPixelsUnits
     * converts pos and size of the given EDA_Rect to pos and size in pixels,
     * relative to the current draw area (origin 0,0 is the left top visible
     * corner of draw area) according to the current scroll and zoom.
     * @param aRect = the rectangle to convert
     */
    void         ConvertPcbUnitsToPixelsUnits( EDA_Rect* aRect );

    /**
     * Function ConvertPcbUnitsToPixelsUnits
     * converts a given wxPoint position (in internal units) to units of pixels,
     * relative to the current draw area (origin 0,0 is the left top visible
     * corner of draw area) according to the current scroll and zoom.
     * @param aPosition = the position to convert
     */
    void         ConvertPcbUnitsToPixelsUnits( wxPoint* aPosition );

    wxPoint      GetScreenCenterRealPosition( void );
    void         MouseToCursorSchema();
    void         MouseTo( const wxPoint& Mouse );

    /* Cursor functions */
    void         Trace_Curseur( wxDC* DC, int color = WHITE );  // Draw the user cursor (grid cursor)
    void         CursorOff( wxDC* DC );                         // remove the grid cursor from the display
    void         CursorOn( wxDC* DC );                          // display the grid cursor

    DECLARE_EVENT_TABLE()
};


/**************************/
/*  class DrawBlockStruct */
/**************************/
/* Definition d'un block pour les fonctions sur block (block move, ..) */
typedef enum {                  /* definition de l'etat du block */
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
    char            m_FlagRefreshReq;    /* indique que l'ecran doit redessine */
    char            m_FlagModified;      // indique modif du PCB,utilise pour eviter une sortie sans sauvegarde
    char            m_FlagSave;          // indique sauvegarde auto faite
    EDA_BaseStruct* m_CurrentItem;       ///< Currently selected object

    /* Valeurs du pas de grille et du zoom */
public:
    wxSize      m_Grid;            /* Current grid. */
    GridArray   m_GridList;
    bool        m_UserGridIsON;

    int         m_Diviseur_Grille;
    wxArrayInt  m_ZoomList;        /* Array of standard zoom coefficients. */
    int         m_Zoom;            /* Current zoom coefficient. */
    int         m_ZoomScalar;      /* Allow zooming to non-integer increments. */

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

    /**
     * Function GetZoom
     * returns the current zoom factor
     */
    int     GetZoom() const;

    /**
     * Function SetZoom
     * adjusts the current zoom factor
     */
    void    SetZoom( int coeff );

    /**
     * Function SetZoomList
     * sets the list of zoom factors.
     * @param aZoomList An array of zoom factors in ascending order, zero terminated
     */
    void    SetZoomList( const wxArrayInt& zoomlist );

    int     Scale( int coord );
    void    Scale( wxPoint& pt );
    void    Scale( wxSize& sz );
    int     Unscale( int coord );
    void    Unscale( wxPoint& pt );
    void    Unscale( wxSize& sz );

    void    SetNextZoom();               /* ajuste le prochain coeff de zoom */
    void    SetPreviousZoom();           /* ajuste le precedent coeff de zoom */
    void    SetFirstZoom();              /* ajuste le coeff de zoom a 1*/
    void    SetLastZoom();               /* ajuste le coeff de zoom au max */

    //----<grid stuff>----------------------------------------------------------
    wxSize  GetGrid();                     /* retourne la grille */
    void    SetGrid( const wxSize& size );
    void    SetGrid( int );
    void    SetGridList( GridArray& sizelist );
    void    AddGrid( const GRID_TYPE& grid );
    void    AddGrid( const wxSize& size, int id );
    void    AddGrid( const wxRealPoint& size, int units, int id );


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


#endif  /* #ifndef PANEL_WXSTRUCT_H */
