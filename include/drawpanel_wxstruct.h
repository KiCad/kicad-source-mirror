/****************************************************/
/*				drawpanel_wxstruct.h:				*/
/* descriptions des principales classes utilisees:	*/
/*	ici classe: "WinEDA_DrawPanel",  "BASE_SCREEN"	*/
/*****************************************************/

/* Doit etre inclus dans "wxstruch.h"
 */

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H


#ifndef eda_global
#define eda_global extern
#endif

#include "colors.h"

/****************************************************/
/* classe representant un ecran graphique de dessin */
/****************************************************/

class WinEDA_DrawPanel : public EDA_DRAW_PANEL
{
public:
    int               m_Ident;
    WinEDA_DrawFrame* m_Parent;
    EDA_Rect          m_ClipBox;    /* position et taille de la fenetre de trace
                                     *  pour les "RePaint" d'ecran */
    wxPoint           m_CursorStartPos;     // utile dans controles du mouvement curseur
    int               m_Scroll_unit;        //	Valeur de l'unite de scroll en pixels pour les barres de scroll
    int               m_ScrollButt_unit;    //	Valeur de l'unite de scroll en pixels pour les boutons de scroll

    bool              m_AbortRequest;       // Flag d'arret de commandes longues
    bool              m_AbortEnable;        // TRUE si menu ou bouton Abort doit etre affiche

    bool              m_AutoPAN_Enable;     // TRUE pour autoriser auto pan (autorisation g��ale)
    bool              m_AutoPAN_Request;    // TRUE pour auto pan (lorsque auto pan n�essaire)

    bool              m_IgnoreMouseEvents;  // TRUE pour ne par traiter les evenements souris

    bool              m_Block_Enable;       // TRUE pour autoriser Bloc Commandes (autorisation g��ale)
    int               m_CanStartBlock;      // >= 0 (ou >= n) si un bloc peut demarrer
    // (filtrage des commandes de debut de bloc )
    int               m_PanelDefaultCursor; // Current mouse cursor default shape id for this window
    int               m_PanelCursor;        // Current mouse cursor shape id for this window
    int               m_CursorLevel;        // Index for cursor redraw in XOR mode

    /* Cursor management (used in editing functions) */
    void              (*ManageCurseur)(WinEDA_DrawPanel * panel, wxDC * DC, bool erase);/* Fonction d'affichage sur deplacement souris
                                                                                         *  si erase : effacement ancien affichage */
    void              (*ForceCloseManageCurseur)(WinEDA_DrawPanel * panel, wxDC * DC);/* Fonction de fermeture forc�
                                                                                       *  de la fonction ManageCurseur */

public:

    // Constructor and destructor
    WinEDA_DrawPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos, const wxSize& size );
    ~WinEDA_DrawPanel( void ) { }
    /****************************/
    BASE_SCREEN* GetScreen( void ) { return m_Parent->m_CurrentScreen; }

    void    PrepareGraphicContext( wxDC* DC );
    wxPoint CalcAbsolutePosition( const wxPoint& rel_pos );
    bool    IsPointOnDisplay( wxPoint ref_pos );
    void    OnPaint( wxPaintEvent& event );
    void    OnSize( wxSizeEvent& event );
    void    SetBoundaryBox( void );
    void    ReDraw( wxDC* DC, bool erasebg = TRUE );
    void    PrintPage( wxDC* DC, bool Print_Sheet_Ref, int PrintMask );
    void    DrawBackGround( wxDC* DC );
    void    m_Draw_Auxiliary_Axis( wxDC* DC, int drawmode );
    void    OnEraseBackground( wxEraseEvent& event );
    void    OnActivate( wxActivateEvent& event );

    /* Mouse and keys events */
    void    OnMouseEvent( wxMouseEvent& event );
    void    OnMouseLeaving( wxMouseEvent& event );
    void    OnKeyEvent( wxKeyEvent& event );

    /*************************/

    void    EraseScreen( wxDC* DC );
    void    OnScrollWin( wxCommandEvent& event );
    void    OnScroll( wxScrollWinEvent& event );

    void    SetZoom( int mode );
    int     GetZoom( void );
    void    SetGrid( const wxSize& size );
    wxSize  GetGrid( void );

    void    AddMenuZoom( wxMenu* MasterMenu );
    void    OnRightClick( wxMouseEvent& event );
    void    Process_Popup_Zoom( wxCommandEvent& event );
    void    Process_Special_Functions( wxCommandEvent& event );
    wxPoint CursorRealPosition( const wxPoint& ScreenPos );
    wxPoint CursorScreenPosition( void );
    wxPoint GetScreenCenterRealPosition( void );
    void    MouseToCursorSchema( void );
    void    MouseTo( const wxPoint& Mouse );

    /* Cursor functions */
    void    Trace_Curseur( wxDC* DC, int color = WHITE );   // Draw the user cursor (grid cursor)
    void    CursorOff( wxDC* DC );                          // remove the grid cursor from the display
    void    CursorOn( wxDC* DC );                           // display the grid cursor

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
    wxPoint         m_BlockLastCursorPosition;/* Last Mouse position in block command
                                               *  = last cursor position in move commands
                                               *  = 0,0 in block paste */

public:
    DrawBlockStruct( void );
    ~DrawBlockStruct( void );
    void    SetMessageBlock( WinEDA_DrawFrame* frame );
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC );
};


/*******************************************************************/
/* Class to handle how to draw a screen (a board, a schematic ...) */
/*******************************************************************/
class BASE_SCREEN : public EDA_BaseStruct
{
public:
    int             m_Type;                     /* indicateur: type d'ecran */
    wxPoint         m_DrawOrg;                  /* offsets pour tracer le circuit sur l'ecran */
    wxPoint         m_Curseur;                  /* Screen cursor coordinate (on grid) in user units. */
    wxPoint         m_MousePosition;            /* Mouse cursor coordinate (off grid) in user units. */
    wxPoint         m_MousePositionInPixels;    /* Mouse cursor coordinate (off grid) in pixels. */
    wxPoint         m_O_Curseur;                /* Relative Screen cursor coordinate (on grid) in user units.
                                                 * (coordinates from last reset position)*/
    wxPoint         m_ScrollbarPos;             // Position effective des Curseurs de scroll
    wxSize          m_ScrollbarNumber;          /* Valeur effective des Nombres de Scrool
                                                 * c.a.d taille en unites de scroll de la surface totale affichable */
    wxPoint         m_StartVisu;                // Coord absolues du 1er pixel visualis�a l'ecran (en nombre de pixels)
    
    wxSize          m_SizeVisu;         /* taille en pixels de l'ecran (fenetre de visu
                                         * Utile pour recadrer les affichages lors de la
                                         * navigation dans la hierarchie */
    bool            m_Center;           // TRUE: coord algebriques, FALSE: coord >= 0
    bool            m_FirstRedraw;

    /* Gestion des editions */
    EDA_BaseStruct* EEDrawList;         /* Object list (main data) for schematic */
    EDA_BaseStruct* m_UndoList;         /* Object list for the undo command (old data) */
    EDA_BaseStruct* m_RedoList;         /* Object list for the redo command (old data) */
    int             m_UndoRedoCountMax; /* undo/Redo command Max depth */

    /* block control */
    DrawBlockStruct BlockLocate;    /* Bock description for block commands */

    /* Page description */
    Ki_PageDescr*   m_CurrentSheet;
    int             m_SheetNumber, m_NumberOfSheet;/* gestion hierarchie: numero de sousfeuille
                                                    *  et nombre de feuilles. Root: SheetNumber = 1 */
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
    EDA_BaseStruct* m_CurrentItem;          /* Current selected object */

    /* Valeurs du pas de grille et du zoom */
public:
    wxSize      m_Grid;         /* pas de la grille (peut differer en X et Y) */
    wxSize*     m_GridList;     /* Liste des valeurs standard de grille */
    wxRealPoint m_UserGrid;     /* pas de la grille utilisateur */
    int         m_UserGridUnit; /* unit�grille utilisateur (0 = inch, 1 = mm */
    int         m_Diviseur_Grille;
    bool        m_UserGridIsON;
    int*        m_ZoomList;     /* Liste des coefficients standard de zoom */
    int         m_Zoom;         /* coeff de ZOOM */

public:
    BASE_SCREEN( int idscreen );
    ~BASE_SCREEN( void );

    void                    InitDatas( void );/* Inits completes des variables */
    wxSize                  ReturnPageSize( void );
    int                     GetInternalUnits( void );

    wxPoint                 CursorRealPosition( const wxPoint& ScreenPos );

    /* general Undo/Redo command control */
    virtual void            ClearUndoRedoList( void );
    virtual void            AddItemToUndoList( EDA_BaseStruct* item );
    virtual void            AddItemToRedoList( EDA_BaseStruct* item );
    virtual EDA_BaseStruct* GetItemFromUndoList( void );
    virtual EDA_BaseStruct* GetItemFromRedoList( void );

    /* Manipulation des flags */
    void    SetRefreshReq( void ) { m_FlagRefreshReq = 1; }
    void    ClrRefreshReq( void ) { m_FlagRefreshReq = 0; }
    void    SetModify( void ) { m_FlagModified = 1; m_FlagSave = 0; }
    void    ClrModify( void ) { m_FlagModified = 0; m_FlagSave = 1; }
    void    SetSave( void ) { m_FlagSave = 1; }
    void    ClrSave( void ) { m_FlagSave = 0; }
    int     IsModify( void ) { return m_FlagModified & 1;  }
    int     IsRefreshReq( void ) { return m_FlagRefreshReq & 1;  }
    int     IsSave( void ) { return m_FlagSave & 1;  }

    
    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.  
     * This is intentionally not inlined so we can set breakpoints on the 
     * activity easier in base_screen.cpp.
     * @param current Any object derived from EDA_BaseStruct
     */
    void            SetCurItem( EDA_BaseStruct* current );
    EDA_BaseStruct* GetCurItem() const {  return m_CurrentItem; }
    
    /* fonctions relatives au zoom */
    int     GetZoom( void );                /* retourne le coeff de zoom */
    void    SetZoom( int coeff );           /* ajuste le coeff de zoom a coeff */
    void    SetZoomList( int* zoomlist );   /* init liste des zoom (NULL terminated) */
    void    SetNextZoom( void );            /* ajuste le prochain coeff de zoom */
    void    SetPreviousZoom( void );        /* ajuste le precedent coeff de zoom */
    void    SetFirstZoom( void );           /* ajuste le coeff de zoom a 1*/
    void    SetLastZoom( void );            /* ajuste le coeff de zoom au max */

    /* fonctions relatives a la grille */
    wxSize  GetGrid( void );                    /* retourne la grille */
    void    SetGrid( const wxSize& size );
    void    SetGridList( wxSize* sizelist );    /* init liste des grilles (NULL terminated) */
    void    SetNextGrid( void );                /* ajuste le prochain coeff de grille */
    void    SetPreviousGrid( void );            /* ajuste le precedent coeff de grille */
    void    SetFirstGrid( void );               /* ajuste la grille au mini*/
    void    SetLastGrid( void );                /* ajuste la grille au max */

    
    /**
     * Function RefPos
     * returns the reference position, coming from either the mouse position or the
     * the cursor position.
     * @param useMouse If true, return mouse posistion, else cursor's.
     * @return wxPoint - The reference point, either the mouse position or 
     *   the cursor position.
     */
    wxPoint RefPos( bool useMouse )
    {
        return useMouse ? m_MousePosition : m_Curseur;
    }
    

#if defined (DEBUG)
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "BASE_SCREEN" );
    }
#endif
};


#endif  /* PANEL_WXSTRUCT_H */
