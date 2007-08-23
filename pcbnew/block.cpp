/*****************************************************************/
/* Operations sur Blocks : deplacement, rotation, effacement ... */
/*****************************************************************/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN

/* Routines Locales */

static void             DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

static EDA_BaseStruct*  IsStructInBox( DrawBlockStruct& blocklocate, EDA_BaseStruct* PtStruct );
static TRACK*           IsSegmentInBox( DrawBlockStruct& blocklocate, TRACK* PtSegm );
static MODULE*          IsModuleInBox( DrawBlockStruct& blocklocate, MODULE* Module );

/* Variables locales :*/
static bool Block_Include_Modules     = TRUE;
static bool Block_Include_Tracks      = TRUE;
static bool Block_Include_Zones       = TRUE;
static bool Block_Include_Draw_Items  = TRUE;
static bool Block_Include_Edges_Items = TRUE;
static bool Block_Include_PcbTextes   = TRUE;

enum id_block_cmd {
    ID_ACCEPT_BLOCK_COMMAND = 8000,
    ID_CANCEL_BLOCK_COMMAND
};

/************************************/
/* class WinEDA_ExecBlockCmdFrame */
/************************************/

class WinEDA_ExecBlockCmdFrame : public wxDialog
{
private:

    WinEDA_BasePcbFrame* m_Parent;
    wxCheckBox*          m_Include_Modules;
    wxCheckBox*          m_Include_Tracks;
    wxCheckBox*          m_Include_Zones;
    wxCheckBox*          m_Include_Draw_Items;
    wxCheckBox*          m_Include_Edges_Items;
    wxCheckBox*          m_Include_PcbTextes;

public:

    // Constructor and destructor
    WinEDA_ExecBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                              const wxString&      title );
    ~WinEDA_ExecBlockCmdFrame( void )
    {
    }


private:
    void    ExecuteCommand( wxCommandEvent& event );
    void    Cancel( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_ExecBlockCmdFrame, wxDialog )
EVT_BUTTON( ID_ACCEPT_BLOCK_COMMAND, WinEDA_ExecBlockCmdFrame::ExecuteCommand )
EVT_BUTTON( ID_CANCEL_BLOCK_COMMAND, WinEDA_ExecBlockCmdFrame::Cancel )
END_EVENT_TABLE()


/**************************************************************/
static bool InstallBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                                  const wxString&      title )
/**************************************************************/
{
    int     nocmd;
    wxPoint oldpos = parent->GetScreen()->m_Curseur;

    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_ExecBlockCmdFrame* frame = new WinEDA_ExecBlockCmdFrame( parent, title );
    nocmd = frame->ShowModal(); frame->Destroy();
    parent->GetScreen()->     m_Curseur = oldpos;
    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;

    parent->DrawPanel->SetCursor(
        parent->DrawPanel->m_PanelCursor = parent->DrawPanel->m_PanelDefaultCursor );

    return nocmd ? FALSE : TRUE;
}


/******************************************************************************/
WinEDA_ExecBlockCmdFrame::WinEDA_ExecBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                                                    const wxString&      title ) :
    wxDialog( parent, -1, title, wxPoint( -1, -1 ), wxSize( 280, 220 ),
              DIALOG_STYLE )
/******************************************************************************/
{
    wxPoint   pos;
    wxButton* Button;

    m_Parent = parent;
    SetFont( *g_DialogFont );
    Centre();

    /* Creation des boutons de commande */
    pos.x  = 170; pos.y = 10;
    Button = new wxButton( this, ID_ACCEPT_BLOCK_COMMAND,
                           _( "Ok" ), pos );
    Button->SetForegroundColour( *wxRED );

    pos.y += Button->GetDefaultSize().y + 20;
    Button = new wxButton( this, ID_CANCEL_BLOCK_COMMAND,
                           _( "Cancel" ), pos );
    Button->SetForegroundColour( *wxBLUE );

    pos.x = 5; pos.y = 20;

    // Selection des options :
    m_Include_Modules = new wxCheckBox( this, -1, _( "Include Modules" ), pos );
    m_Include_Modules->SetValue( Block_Include_Modules );

    pos.y += 20;
    m_Include_Tracks = new wxCheckBox( this, -1, _( "Include tracks" ), pos );
    m_Include_Tracks->SetValue( Block_Include_Tracks );

    pos.y += 20;
    m_Include_Zones = new wxCheckBox( this, -1, _( "Include zones" ), pos );
    m_Include_Zones->SetValue( Block_Include_Zones );

    pos.y += 20;
    m_Include_PcbTextes = new wxCheckBox( this, -1,
                                          _( "Include Text on copper layers" ), pos );
    m_Include_PcbTextes->SetValue( Block_Include_PcbTextes );

    pos.y += 20;
    m_Include_Draw_Items = new wxCheckBox( this, -1, _( "Include drawings" ), pos );
    m_Include_Draw_Items->SetValue( Block_Include_Draw_Items );

    pos.y += 20;
    m_Include_Edges_Items = new wxCheckBox( this, -1, _( "Include egde layer" ), pos );
    m_Include_Edges_Items->SetValue( Block_Include_Edges_Items );
}


/**********************************************************************/
void WinEDA_ExecBlockCmdFrame::Cancel( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( 1 );
}


/*******************************************************************/
void WinEDA_ExecBlockCmdFrame::ExecuteCommand( wxCommandEvent& event )
/*******************************************************************/
{
    Block_Include_Modules     = m_Include_Modules->GetValue();
    Block_Include_Tracks      = m_Include_Tracks->GetValue();
    Block_Include_Zones       = m_Include_Zones->GetValue();
    Block_Include_Draw_Items  = m_Include_Draw_Items->GetValue();
    Block_Include_Edges_Items = m_Include_Edges_Items->GetValue();
    Block_Include_PcbTextes   = m_Include_PcbTextes->GetValue();

    EndModal( 0 );
}


/*************************************************/
int WinEDA_PcbFrame::ReturnBlockCommand( int key )
/*************************************************/

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
{
    int cmd = 0;

    switch( key )
    {
    default:
        cmd = key & 0x255;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_ROTATE;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_INVERT;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/*****************************************************/
void WinEDA_PcbFrame::HandleBlockPlace( wxDC* DC )
/*****************************************************/
/* Routine to handle the BLOCK PLACE commande */
{
    bool err = FALSE;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "Error in HandleBlockPLace : ManageCurseur = NULL" ) );
    }
    GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Move( DC );
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Duplicate( DC );
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        break;

    case BLOCK_PASTE:
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd()
    default:
        break;
    }

    GetScreen()->SetModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->BlockLocate.m_Flags   = 0;
    GetScreen()->BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->BlockLocate.m_Command = BLOCK_IDLE;
    if( GetScreen()->BlockLocate.m_BlockDrawStruct )
    {
        DisplayError( this, wxT( "Error in HandleBlockPLace DrawStruct != NULL" ) );
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
    }

    DisplayToolMsg( wxEmptyString );
}


/**********************************************/
int WinEDA_PcbFrame::HandleBlockEnd( wxDC* DC )
/**********************************************/

/* Routine de gestion de la commande BLOCK END
 *  returne :
 *  0 si aucun compos ant selectionne
 *  1 sinon
 *  -1 si commande terminée et composants trouvés (block delete, block save)
 */
{
    int endcommande = TRUE;

    if( DrawPanel->ManageCurseur )
        switch( GetScreen()->BlockLocate.m_Command )
        {
        case  BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
            break;

        case BLOCK_DRAG:            /* Drag (not used, for future enhancements)*/
        case BLOCK_MOVE:            /* Move */
        case BLOCK_COPY:            /* Copy */
        case BLOCK_PRESELECT_MOVE:  /* Move with preselection list*/
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_MOVE;
            endcommande = FALSE;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            break;

        case BLOCK_DELETE: /* Delete */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Delete( DC );
            break;

        case BLOCK_ROTATE: /* Rotation */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Rotate( DC );
            break;

        case BLOCK_INVERT: /* Flip */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Invert( DC );
            break;

        case BLOCK_SAVE: /* Save (not used, for future enhancements)*/
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;
            if( GetScreen()->BlockLocate.m_BlockDrawStruct != NULL )
            {
                DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );

//				SaveStruct(GetScreen()->BlockLocate.m_BlockDrawStruct);
            }
            break;

        case BLOCK_PASTE:
            break;

        case BLOCK_ZOOM: /* Window Zoom */

            //Turn off the redraw block routine now so it is not displayed
            // with one corner at the new center of the screen
            DrawPanel->ManageCurseur = NULL;
            Window_Zoom( GetScreen()->BlockLocate );
            break;

        default:
            break;
        }

    if( endcommande == TRUE )
    {
        GetScreen()->BlockLocate.m_Flags   = 0;
        GetScreen()->BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->BlockLocate.m_Command = BLOCK_IDLE;
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        DisplayToolMsg( wxEmptyString );
    }

    return endcommande;
}


/**************************************************************************/
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/**************************************************************************/

/* Retrace le contour du block de repositionnement des structures a déplacer
 */
{
    int          Color;
    BASE_SCREEN* screen = panel->GetScreen();

    Color = YELLOW; GRSetDrawMode( DC, g_XorMode );

    /* Effacement ancien cadre */
    if( erase )
    {
        screen->BlockLocate.Draw( panel, DC );
        if( screen->BlockLocate.m_MoveVector.x || screen->BlockLocate.m_MoveVector.y )
        {
            screen->BlockLocate.Offset( screen->BlockLocate.m_MoveVector );
            screen->BlockLocate.Draw( panel, DC );
            screen->BlockLocate.Offset( -screen->BlockLocate.m_MoveVector.x,
                                        -screen->BlockLocate.m_MoveVector.y );
        }
    }

    if( panel->m_Parent->GetScreen()->BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->BlockLocate.m_MoveVector.x = screen->m_Curseur.x - screen->BlockLocate.GetRight();
        screen->BlockLocate.m_MoveVector.y = screen->m_Curseur.y - screen->BlockLocate.GetBottom();
    }

    screen->BlockLocate.Draw( panel, DC );
    if( screen->BlockLocate.m_MoveVector.x || screen->BlockLocate.m_MoveVector.y )
    {
        screen->BlockLocate.Offset( screen->BlockLocate.m_MoveVector );
        screen->BlockLocate.Draw( panel, DC );
        screen->BlockLocate.Offset( -screen->BlockLocate.m_MoveVector.x,
                                    -screen->BlockLocate.m_MoveVector.y );
    }
}


/************************************************/
void WinEDA_BasePcbFrame::Block_Delete( wxDC* DC )
/************************************************/

/*
 *  routine d'effacement du block deja selectionne
 */
{
    BOARD_ITEM*     PtStruct, * NextS;
    int             masque_layer;

    if( !InstallBlockCmdFrame( this, _( "Delete Block" ) ) )
        return;

    GetScreen()->SetModify();
    GetScreen()->BlockLocate.Normalize();
    GetScreen()->SetCurItem( NULL );

    /* Effacement des modules */
    if( Block_Include_Modules )
    {
        MODULE* module;
        Affiche_Message( _( "Delete Footprints" ) );
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = (MODULE*) NextS )
        {
            NextS = module->Next();
            if( IsModuleInBox( GetScreen()->BlockLocate, module ) == NULL )
                continue;
            /* le module est ici bon a etre efface */
            module->m_Flags = 0;
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            DeleteStructure( module );
            m_Pcb->m_Status_Pcb = 0;
        }
    }

    /* Effacement des Pistes */
    if( Block_Include_Tracks )
    {
        TRACK* pt_segm;

        Affiche_Message( _( "Delete tracks" ) );
        for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) NextS )
        {
            NextS = pt_segm->Next();
            if( IsSegmentInBox( GetScreen()->BlockLocate, pt_segm ) )
            {       
                /* la piste est ici bonne a etre efface */
                pt_segm->Draw( DrawPanel, DC, GR_XOR );
                DeleteStructure( pt_segm );
            }
        }
    }

    /* Effacement des Elements De Dessin */
    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    Affiche_Message( _( "Delete draw layers" ) );
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = NextS )
    {
        NextS = PtStruct->Next();

        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, GR_XOR );
            DeleteStructure( PtStruct );
            break;

        case TYPETEXTE:
            if( !Block_Include_PcbTextes )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* le texte est ici bon a etre efface */
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            /* Suppression du texte en Memoire*/
            DeleteStructure( PtStruct );
            break;

        case TYPEMIRE:
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            DeleteStructure( PtStruct );
            break;

        case TYPECOTATION:
                #undef STRUCT
                #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            DeleteStructure( PtStruct );
            break;

        default:
            break;
        }
    }

    /* Effacement des Zones */
    if( Block_Include_Zones )
    {
        TRACK* pt_segm;

        Affiche_Message( _( "Delete zones" ) );
        for( pt_segm = m_Pcb->m_Zone; pt_segm != NULL; pt_segm = (TRACK*) NextS )
        {
            NextS = pt_segm->Next();
            if( IsSegmentInBox( GetScreen()->BlockLocate, pt_segm ) )
            {    /* la piste est ici bonne a etre efface */
                pt_segm->Draw( DrawPanel, DC, GR_XOR );
                DeleteStructure( pt_segm );
            }
        }
    }

    /* Rafraichissement de l'ecran : */
    RedrawActiveWindow( DC, TRUE );
    if( g_Show_Ratsnest )
        Compile_Ratsnest( DC, TRUE );
}


/****************************************************/
void WinEDA_BasePcbFrame::Block_Rotate( wxDC* DC )
/****************************************************/

/*
 *  routine de Rotation de 90 deg du block deja selectionne
 *  les elements sont tournes autour du centre du block
 */
{
    MODULE* module;
    EDA_BaseStruct* PtStruct;
    int masque_layer;
    wxPoint oldpos;
    int Nx, Ny, centerX, centerY;   /* centre de rotation de l'ensemble des elements */

    if( !InstallBlockCmdFrame( this, _( "Rotate Block" ) ) )
        return;

    oldpos = GetScreen()->m_Curseur;
    GetScreen()->BlockLocate.Normalize();

    /* calcul du centre de Rotation */
    centerX = GetScreen()->BlockLocate.Centre().x;
    centerY = GetScreen()->BlockLocate.Centre().y;

    GetScreen()->SetModify();

    /* Rotation des modules */
    if( Block_Include_Modules )
    {
        Affiche_Message( _( "Footprint rotation" ) );
        bool Show_Ratsnest_tmp = g_Show_Ratsnest; g_Show_Ratsnest = false;
        int Angle_Rot_Module   = 900;
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = (MODULE*) module->Pnext )
        {
            if( IsModuleInBox( GetScreen()->BlockLocate, module ) == NULL )
                continue;
            /* le module est ici bon a etre modifie */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );

            /* calcul de la nouvelle position du Module */
            Nx = module->m_Pos.x; Ny = module->m_Pos.y;
            RotatePoint( &Nx, &Ny, centerX, centerY, 900 );
            GetScreen()->m_Curseur.x = Nx;
            GetScreen()->m_Curseur.y = Ny;
            Place_Module( module, DC );

            /* Rotation du module autour de son ancre */
            Rotate_Module( DC, module, Angle_Rot_Module, TRUE );
        }

        /* regeneration des valeurs originelles */
        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track;

        Affiche_Message( _( "Track rotation" ) );
        track = m_Pcb->m_Track;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                RotatePoint( &track->m_Start.x, &track->m_Start.y, centerX, centerY, 900 );
                RotatePoint( &track->m_End.x, &track->m_End.y, centerX, centerY, 900 );
                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = track->Next();
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        TRACK* track;

        Affiche_Message( _( "Zone rotation" ) );
        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                RotatePoint( &track->m_Start.x, &track->m_Start.y, centerX, centerY, 900 );
                RotatePoint( &track->m_End.x, &track->m_End.y, centerX, centerY, 900 );
                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = track->Next();
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    Affiche_Message( _( "Draw layers rotation" ) );
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, GR_XOR );
            RotatePoint( &STRUCT->m_Start.x, &STRUCT->m_Start.y, centerX, centerY, 900 );
            RotatePoint( &STRUCT->m_End.x, &STRUCT->m_End.y, centerX, centerY, 900 );
            Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, GR_OR );
            break;

        case TYPETEXTE:
                #undef STRUCT
                #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* le texte est ici bon a etre deplace */
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            /* Redessin du Texte */
            RotatePoint( &STRUCT->m_Pos.x, &STRUCT->m_Pos.y, centerX, centerY, 900 );
            STRUCT->m_Orient += 900;
            if( STRUCT->m_Orient >= 3600 )
                STRUCT->m_Orient -= 3600;

            STRUCT->CreateDrawData();
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPEMIRE:
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre modifie */
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            RotatePoint( &STRUCT->m_Pos.x, &STRUCT->m_Pos.y, centerX, centerY, 900 );
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPECOTATION:
                #undef STRUCT
                #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre modifie */
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

            RotatePoint( &STRUCT->m_Pos.x, &STRUCT->m_Pos.y, centerX, centerY, 900 );

            RotatePoint( &STRUCT->m_Text->m_Pos.x, &STRUCT->m_Text->m_Pos.y,
                         centerX, centerY, 900 );
            STRUCT->m_Text->m_Orient += 900;
            if( STRUCT->m_Text->m_Orient >= 3600 )
                STRUCT->m_Text->m_Orient -= 3600;
            if( (STRUCT->m_Text->m_Orient > 900)
               && (STRUCT->m_Text->m_Orient <2700) )
                STRUCT->m_Text->m_Orient -= 1800;

            RotatePoint( &STRUCT->Barre_ox, &STRUCT->Barre_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->Barre_fx, &STRUCT->Barre_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->TraitG_ox, &STRUCT->TraitG_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->TraitG_fx, &STRUCT->TraitG_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->TraitD_ox, &STRUCT->TraitD_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->TraitD_fx, &STRUCT->TraitD_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheG1_ox, &STRUCT->FlecheG1_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheG1_fx, &STRUCT->FlecheG1_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheG2_ox, &STRUCT->FlecheG2_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheG2_fx, &STRUCT->FlecheG2_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheD1_ox, &STRUCT->FlecheD1_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheD1_fx, &STRUCT->FlecheD1_fy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheD2_ox, &STRUCT->FlecheD2_oy, centerX, centerY, 900 );
            RotatePoint( &STRUCT->FlecheD2_fx, &STRUCT->FlecheD2_fy, centerX, centerY, 900 );
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        default:
            break;
        }
    }

    RedrawActiveWindow( DC, TRUE );
    if( g_Show_Ratsnest )
        Compile_Ratsnest( DC, TRUE );
}


/*****************************************************/
void WinEDA_BasePcbFrame::Block_Invert( wxDC* DC )
/*****************************************************/

/*
 *  routine d'inversion miroir deg du block deja selectionne
 *  les elements sont inverse / axe horizontal,
 *  l'axe d'inversion est la mediane horizontale du block
 */
#define INVERT( pos )       (pos) = centerY - ( (pos) - centerY )
#define INVERT_ANGLE( phi ) (phi) = -(phi)
{
    MODULE* module;
    EDA_BaseStruct* PtStruct;
    int masque_layer;
    wxPoint memo;
    int Ny, centerY;/* position de l'axe d'inversion de l'ensemble des elements */

    if( !InstallBlockCmdFrame( this, _( "Block mirroring" ) ) )
        return;

    memo = GetScreen()->m_Curseur;
    GetScreen()->BlockLocate.Normalize();

    /* calcul du centre d'inversion */
    centerY = GetScreen()->BlockLocate.Centre().y;

    GetScreen()->SetModify();

    /* Inversion des modules */
    if( Block_Include_Modules )
    {
        bool Show_Ratsnest_tmp = g_Show_Ratsnest; g_Show_Ratsnest = false;
        Affiche_Message( _( "Footprint mirroring" ) );
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = (MODULE*) module->Pnext )
        {
            if( IsModuleInBox( GetScreen()->BlockLocate, module ) == NULL )
                continue;
            /* le module est ici bon a etre efface */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );

            /* calcul de la nouvelle position du Module */
            Ny = module->m_Pos.y;
            INVERT( Ny );
            GetScreen()->m_Curseur.x = module->m_Pos.x;
            GetScreen()->m_Curseur.y = Ny;
            Place_Module( module, DC );

            /* inversion du module  */
            Change_Side_Module( module, DC );

            /* regeneration des valeurs originelles */
            GetScreen()->m_Curseur = memo;
        }

        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track;

        Affiche_Message( _( "Track mirroring" ) );
        track = m_Pcb->m_Track;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                INVERT( track->m_Start.y );
                INVERT( track->m_End.y );
                if( track->m_StructType != TYPEVIA )
                {
                    track->SetLayer( ChangeSideNumLayer( track->GetLayer() ) );
                }

                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = (TRACK*) track->Pnext;
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        TRACK* track;

        Affiche_Message( _( "Zone mirroring" ) );
        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                INVERT( track->m_Start.y );
                INVERT( track->m_End.y );
                track->SetLayer( ChangeSideNumLayer( track->GetLayer() ) );
                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = (TRACK*) track->Pnext;
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    Affiche_Message( _( "Draw layers mirroring" ) );
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre selectionne */
            Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, GR_XOR );
            if( STRUCT->m_Shape == S_ARC )
            {
                INVERT_ANGLE( STRUCT->m_Angle );
            }
            INVERT( STRUCT->m_Start.y );
            INVERT( STRUCT->m_End.y );
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, GR_OR );
            break;

        case TYPETEXTE:
                #undef STRUCT
                #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* le texte est ici bon a etre selectionne*/
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            /* Redessin du Texte */
            INVERT( STRUCT->m_Pos.y );
            INVERT_ANGLE( STRUCT->m_Orient );
            if( (STRUCT->GetLayer() == CUIVRE_N) || (STRUCT->GetLayer() == CMP_N) )
            {
                STRUCT->m_Miroir ^= 1;      /* inverse miroir */
            }
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            STRUCT->CreateDrawData();
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPEMIRE:
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre modifie */
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            INVERT( STRUCT->m_Pos.y );
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPECOTATION:
                #undef STRUCT
                #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre modifie */
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

            INVERT( STRUCT->m_Pos.y );
            INVERT( STRUCT->m_Text->m_Pos.y );
            INVERT_ANGLE( STRUCT->m_Text->m_Orient );
            if( STRUCT->m_Text->m_Orient >= 3600 )
                STRUCT->m_Text->m_Orient -= 3600;
            if( (STRUCT->m_Text->m_Orient > 900)
               && (STRUCT->m_Text->m_Orient <2700) )
                STRUCT->m_Text->m_Orient -= 1800;

            INVERT( STRUCT->Barre_oy );
            INVERT( STRUCT->Barre_fy );
            INVERT( STRUCT->TraitG_oy );
            INVERT( STRUCT->TraitG_fy );
            INVERT( STRUCT->TraitD_oy );
            INVERT( STRUCT->TraitD_fy );
            INVERT( STRUCT->FlecheG1_oy );
            INVERT( STRUCT->FlecheG1_fy );
            INVERT( STRUCT->FlecheG2_oy );
            INVERT( STRUCT->FlecheG2_fy );
            INVERT( STRUCT->FlecheD1_oy );
            INVERT( STRUCT->FlecheD1_fy );
            INVERT( STRUCT->FlecheD2_oy );
            INVERT( STRUCT->FlecheD2_fy );

            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );

            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        default:
            break;
        }
    }

    RedrawActiveWindow( DC, TRUE );
    if( g_Show_Ratsnest )
        Compile_Ratsnest( DC, TRUE );
}


/************************************************/
void WinEDA_BasePcbFrame::Block_Move( wxDC* DC )
/************************************************/

/*
 *  Function to move items withing the selected block
 */
{
    MODULE* module;
    EDA_BaseStruct* PtStruct;
    int masque_layer;
    int deltaX, deltaY;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    if( !InstallBlockCmdFrame( this, _( "Move Block" ) ) )
        return;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
    GetScreen()->SetModify();
    GetScreen()->BlockLocate.Normalize();

    /* Deplacement des modules */
    if( Block_Include_Modules )
    {
        bool Show_Ratsnest_tmp = g_Show_Ratsnest; g_Show_Ratsnest = false;
        Affiche_Message( _( "Move footprints" ) );
        module = m_Pcb->m_Modules;
        oldpos = GetScreen()->m_Curseur;

        for( ; module != NULL; module = (MODULE*) module->Pnext )
        {
            if( IsModuleInBox( GetScreen()->BlockLocate, module ) == NULL )
                continue;
            /* le module est ici bon a etre deplace */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );

            /* calcul du deplacement pour la routine Place_Module */
            /* calcul du vecteur de deplacement */
            GetScreen()->m_Curseur.x = module->m_Pos.x + GetScreen()->BlockLocate.m_MoveVector.x;
            GetScreen()->m_Curseur.y = module->m_Pos.y + GetScreen()->BlockLocate.m_MoveVector.y;
            Place_Module( module, DC );
        }

        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* calcul du vecteur de deplacement pour les deplacements suivants */
    deltaX = GetScreen()->BlockLocate.m_MoveVector.x;
    deltaY = GetScreen()->BlockLocate.m_MoveVector.y;

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track;

        Affiche_Message( _( "Move tracks" ) );
        track = m_Pcb->m_Track;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                track->m_Start.x += deltaX; track->m_Start.y += deltaY;
                track->m_End.x   += deltaX; track->m_End.y += deltaY;
                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = (TRACK*) track->Pnext;
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        TRACK* track;

        Affiche_Message( _( "Move zones" ) );
        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                track->Draw( DrawPanel, DC, GR_XOR );   // effacement
                track->m_Start.x += deltaX; track->m_Start.y += deltaY;
                track->m_End.x   += deltaX; track->m_End.y += deltaY;
                track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = (TRACK*) track->Pnext;
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    Affiche_Message( _( "Move draw layers" ) );
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            Trace_DrawSegmentPcb( DrawPanel, DC, STRUCT, GR_XOR );
            STRUCT->m_Start.x += deltaX; STRUCT->m_Start.y += deltaY;
            STRUCT->m_End.x   += deltaX; STRUCT->m_End.y += deltaY;
            Trace_DrawSegmentPcb( DrawPanel, DC, STRUCT, GR_OR );
            break;

        case TYPETEXTE:
                #undef STRUCT
                #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* le texte est ici bon a etre deplace */
            STRUCT->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            /* Redessin du Texte */
            STRUCT->m_Pos.x += deltaX; STRUCT->m_Pos.y += deltaY;
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPEMIRE:
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            STRUCT->m_Pos.x += deltaX; STRUCT->m_Pos.y += deltaY;
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        case TYPECOTATION:
                #undef STRUCT
                #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            STRUCT->m_Pos.x += deltaX;
            STRUCT->m_Pos.y += deltaY;
            STRUCT->m_Text->m_Pos.x += deltaX;
            STRUCT->m_Text->m_Pos.y += deltaY;
            STRUCT->Barre_ox    += deltaX; STRUCT->Barre_oy += deltaY;
            STRUCT->Barre_fx    += deltaX; STRUCT->Barre_fy += deltaY;
            STRUCT->TraitG_ox   += deltaX; STRUCT->TraitG_oy += deltaY;
            STRUCT->TraitG_fx   += deltaX; STRUCT->TraitG_fy += deltaY;
            STRUCT->TraitD_ox   += deltaX; STRUCT->TraitD_oy += deltaY;
            STRUCT->TraitD_fx   += deltaX; STRUCT->TraitD_fy += deltaY;
            STRUCT->FlecheG1_ox += deltaX; STRUCT->FlecheG1_oy += deltaY;
            STRUCT->FlecheG1_fx += deltaX; STRUCT->FlecheG1_fy += deltaY;
            STRUCT->FlecheG2_ox += deltaX; STRUCT->FlecheG2_oy += deltaY;
            STRUCT->FlecheG2_fx += deltaX; STRUCT->FlecheG2_fy += deltaY;
            STRUCT->FlecheD1_ox += deltaX; STRUCT->FlecheD1_oy += deltaY;
            STRUCT->FlecheD1_fx += deltaX; STRUCT->FlecheD1_fy += deltaY;
            STRUCT->FlecheD2_ox += deltaX; STRUCT->FlecheD2_oy += deltaY;
            STRUCT->FlecheD2_fx += deltaX; STRUCT->FlecheD2_fy += deltaY;
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;

        default:
            break;
        }
    }

    DrawPanel->Refresh( TRUE );;
    if( g_Show_Ratsnest )
        Compile_Ratsnest( DC, TRUE );
}


/**************************************************/
void WinEDA_BasePcbFrame::Block_Duplicate( wxDC* DC )
/**************************************************/

/*
 *  routine de duplication des elements du block deja selectionne
 */
{
    MODULE* module;
    EDA_BaseStruct* PtStruct;
    int masque_layer;
    int deltaX, deltaY;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    if( !InstallBlockCmdFrame( this, _( "Copy Block" ) ) )
        return;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
    GetScreen()->SetModify();
    GetScreen()->BlockLocate.Normalize();

    /* Module copy */
    if( Block_Include_Modules )
    {
        bool Show_Ratsnest_tmp = g_Show_Ratsnest; g_Show_Ratsnest = false;
        Affiche_Message( _( "Module copy" ) );
        module = m_Pcb->m_Modules;
        oldpos = GetScreen()->m_Curseur;

        for( ; module != NULL; module = (MODULE*) module->Pnext )
        {
            MODULE* new_module;
            if( IsModuleInBox( GetScreen()->BlockLocate, module ) == NULL )
                continue;
            /* le module est ici bon a etre deplace */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            new_module = new MODULE( m_Pcb );
            new_module->Copy( module );
            new_module->m_TimeStamp = GetTimeStamp();
            new_module->Pnext = m_Pcb->m_Modules;
            new_module->Pback = m_Pcb;
            m_Pcb->m_Modules->Pback = new_module;
            m_Pcb->m_Modules = new_module;
            /* calcul du deplacement pour la routine Place_Module */
            /* calcul du vecteur de deplacement */
            GetScreen()->m_Curseur.x = module->m_Pos.x + GetScreen()->BlockLocate.m_MoveVector.x;
            GetScreen()->m_Curseur.y = module->m_Pos.y + GetScreen()->BlockLocate.m_MoveVector.y;
            Place_Module( new_module, DC );
        }

        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* calcul du vecteur de deplacement pour les deplacements suivants */
    deltaX = GetScreen()->BlockLocate.m_MoveVector.x;
    deltaY = GetScreen()->BlockLocate.m_MoveVector.y;

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track, * next_track, * new_track;

        Affiche_Message( _( "Track copy" ) );
        track = m_Pcb->m_Track;
        while( track )
        {
            next_track = track->Next();
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {   /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                new_track = track->Copy( 1 );
                new_track->Insert( m_Pcb, NULL );
                new_track->m_Start.x += deltaX; new_track->m_Start.y += deltaY;
                new_track->m_End.x   += deltaX; new_track->m_End.y += deltaY;
                new_track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = next_track;
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        TRACK* track, * next_track, * new_track;

        Affiche_Message( _( "Zone copy" ) );
        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            next_track = track->Next();
            if( IsSegmentInBox( GetScreen()->BlockLocate, track ) )
            {  /* la piste est ici bonne a etre deplacee */
                new_track = new TRACK( m_Pcb );
                new_track = track->Copy( 1 );
                new_track->Insert( m_Pcb, NULL );
                new_track->m_Start.x += deltaX; new_track->m_Start.y += deltaY;
                new_track->m_End.x   += deltaX; new_track->m_End.y += deltaY;
                new_track->Draw( DrawPanel, DC, GR_OR ); // reaffichage
            }
            track = next_track;
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    Affiche_Message( _( "Draw layers copy" ) );
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
        {
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre copie */
            DRAWSEGMENT* new_drawsegment = new DRAWSEGMENT( m_Pcb );
            new_drawsegment->Copy( STRUCT );
            new_drawsegment->Pnext   = m_Pcb->m_Drawings;
            new_drawsegment->Pback   = m_Pcb;
            m_Pcb->m_Drawings->Pback = new_drawsegment;
            m_Pcb->m_Drawings = new_drawsegment;
            new_drawsegment->m_Start.x += deltaX; new_drawsegment->m_Start.y += deltaY;
            new_drawsegment->m_End.x   += deltaX; new_drawsegment->m_End.y += deltaY;
            Trace_DrawSegmentPcb( DrawPanel, DC, new_drawsegment, GR_OR );
            break;
        }

        case TYPETEXTE:
        {
                #undef STRUCT
                #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* le texte est ici bon a etre deplace */
            TEXTE_PCB* new_pcbtext = new TEXTE_PCB( m_Pcb );
            new_pcbtext->Copy( STRUCT );
            new_pcbtext->Pnext = m_Pcb->m_Drawings;
            new_pcbtext->Pback = m_Pcb;
            m_Pcb->m_Drawings->Pback = new_pcbtext;
            m_Pcb->m_Drawings = new_pcbtext;
            /* Redessin du Texte */
            new_pcbtext->m_Pos.x += deltaX; new_pcbtext->m_Pos.y += deltaY;
            new_pcbtext->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;
        }

        case TYPEMIRE:
        {
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre efface */
            MIREPCB* new_mire = new MIREPCB( m_Pcb );
            new_mire->Copy( STRUCT );
            new_mire->Pnext = m_Pcb->m_Drawings;
            new_mire->Pback = m_Pcb;
            m_Pcb->m_Drawings->Pback = new_mire;
            m_Pcb->m_Drawings  = new_mire;
            new_mire->m_Pos.x += deltaX; new_mire->m_Pos.y += deltaY;
            new_mire->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;
        }

        case TYPECOTATION:
        {
                #undef STRUCT
                #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( IsStructInBox( GetScreen()->BlockLocate, PtStruct ) == NULL )
                break;
            /* l'element est ici bon a etre copie */
            COTATION* new_cotation = new COTATION( m_Pcb );
            new_cotation->Copy( STRUCT );
            new_cotation->Pnext      = m_Pcb->m_Drawings;
            new_cotation->Pback      = m_Pcb;
            m_Pcb->m_Drawings->Pback = new_cotation;
            m_Pcb->m_Drawings      = new_cotation;
            new_cotation->m_Pos.x += deltaX;
            new_cotation->m_Pos.y += deltaY;
            new_cotation->m_Text->m_Pos.x += deltaX;
            new_cotation->m_Text->m_Pos.y += deltaY;
            new_cotation->Barre_ox    += deltaX; new_cotation->Barre_oy += deltaY;
            new_cotation->Barre_fx    += deltaX; new_cotation->Barre_fy += deltaY;
            new_cotation->TraitG_ox   += deltaX; new_cotation->TraitG_oy += deltaY;
            new_cotation->TraitG_fx   += deltaX; new_cotation->TraitG_fy += deltaY;
            new_cotation->TraitD_ox   += deltaX; new_cotation->TraitD_oy += deltaY;
            new_cotation->TraitD_fx   += deltaX; new_cotation->TraitD_fy += deltaY;
            new_cotation->FlecheG1_ox += deltaX; new_cotation->FlecheG1_oy += deltaY;
            new_cotation->FlecheG1_fx += deltaX; new_cotation->FlecheG1_fy += deltaY;
            new_cotation->FlecheG2_ox += deltaX; new_cotation->FlecheG2_oy += deltaY;
            new_cotation->FlecheG2_fx += deltaX; new_cotation->FlecheG2_fy += deltaY;
            new_cotation->FlecheD1_ox += deltaX; new_cotation->FlecheD1_oy += deltaY;
            new_cotation->FlecheD1_fx += deltaX; new_cotation->FlecheD1_fy += deltaY;
            new_cotation->FlecheD2_ox += deltaX; new_cotation->FlecheD2_oy += deltaY;
            new_cotation->FlecheD2_fx += deltaX; new_cotation->FlecheD2_fy += deltaY;
            new_cotation->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            break;
        }

        default:
            break;
        }
    }
}


/*******************************************************************/
static EDA_BaseStruct* IsStructInBox( DrawBlockStruct& blocklocate,
                                      EDA_BaseStruct*  PtStruct )
/******************************************************************/

/* Teste si la structure PtStruct est inscrite dans le block selectionne
 */
{
    switch( PtStruct->m_StructType )
    {
    case TYPEDRAWSEGMENT:
            #undef STRUCT
            #define STRUCT ( (DRAWSEGMENT*) PtStruct )
        if( !blocklocate.Inside( STRUCT->m_Start.x, STRUCT->m_Start.y ) )
            return NULL;
        if( !blocklocate.Inside( STRUCT->m_End.x, STRUCT->m_End.y ) )
            return NULL;
        return PtStruct;

    case TYPETEXTE:
            #undef STRUCT
            #define STRUCT ( (TEXTE_PCB*) PtStruct )
        if( !blocklocate.Inside( STRUCT->m_Pos.x, STRUCT->m_Pos.y ) )
            return NULL;
        return PtStruct;

    case TYPEMIRE:
            #undef STRUCT
            #define STRUCT ( (MIREPCB*) PtStruct )
        if( !blocklocate.Inside( STRUCT->m_Pos.x, STRUCT->m_Pos.y ) )
            return NULL;
        return PtStruct;

    case TYPECOTATION:
            #undef STRUCT
            #define STRUCT ( (COTATION*) PtStruct )
        if( !blocklocate.Inside( STRUCT->m_Pos.x, STRUCT->m_Pos.y ) )
            return NULL;
        return PtStruct;

    default:
        return NULL;
    }

    return NULL;
}


/**************************************************************************/
static TRACK* IsSegmentInBox( DrawBlockStruct& blocklocate, TRACK* PtSegm )
/**************************************************************************/

/* Teste si la structure PtStruct est inscrite dans le block selectionne
 *  Retourne PtSegm si oui
 *          NULL si non
 */
{
    if( blocklocate.Inside( PtSegm->m_Start.x, PtSegm->m_Start.y ) )
        return PtSegm;

    if( blocklocate.Inside( PtSegm->m_End.x, PtSegm->m_End.y ) )
        return PtSegm;

    return NULL;
}


/****************************************************************************/
static MODULE* IsModuleInBox( DrawBlockStruct& blocklocate, MODULE* Module )
/****************************************************************************/

/* Teste si le Module est inscrit dans le block selectionne
 *  Retourne Module si oui
 *          NULL si non
 */
{
    bool is_out_of_box = FALSE;

    Module->SetRectangleExinscrit();

    if( Module->m_RealBoundaryBox.m_Pos.x < blocklocate.GetX() )
        is_out_of_box = TRUE;
    if( Module->m_RealBoundaryBox.m_Pos.y < blocklocate.GetY() )
        is_out_of_box = TRUE;
    if( Module->m_RealBoundaryBox.GetRight() > blocklocate.GetRight() )
        is_out_of_box = TRUE;
    if( Module->m_RealBoundaryBox.GetBottom() > blocklocate.GetBottom() )
        is_out_of_box = TRUE;

    if( is_out_of_box )
        return NULL;
    return Module;
}
