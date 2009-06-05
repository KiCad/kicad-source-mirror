/*****************************************************************/
/* Operations sur Blocks : deplacement, rotation, effacement ... */
/*****************************************************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "pcbnew.h"
#include "autorout.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN

/* Routines Locales */

static void             DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/* Variables locales :*/
static bool Block_Include_Modules     = TRUE;
static bool Block_Include_Tracks      = TRUE;
static bool Block_Include_Zones       = TRUE;
static bool Block_Include_Draw_Items  = TRUE;
static bool Block_Include_Edges_Items = TRUE;
static bool Block_Include_PcbTextes   = TRUE;

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
    ~WinEDA_ExecBlockCmdFrame()
    {
    }


private:
    void    ExecuteCommand( wxCommandEvent& event );
    void    Cancel( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_ExecBlockCmdFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_ExecBlockCmdFrame::ExecuteCommand )
EVT_BUTTON( wxID_CANCEL, WinEDA_ExecBlockCmdFrame::Cancel )
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

    nocmd = frame->ShowModal();
    frame->Destroy();

    parent->GetScreen()->m_Curseur = oldpos;

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;

    parent->DrawPanel->SetCursor(
        parent->DrawPanel->m_PanelCursor = parent->DrawPanel->m_PanelDefaultCursor );

    return nocmd ? FALSE : TRUE;
}


/******************************************************************************/
WinEDA_ExecBlockCmdFrame::WinEDA_ExecBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                                                    const wxString&      title ) :
    wxDialog( parent, -1, title, wxPoint( -1, -1 ), wxDefaultSize,
              DIALOG_STYLE )
/******************************************************************************/
{
    wxPoint   pos;
    wxButton* m_button1;
    wxButton* m_button2;

    m_Parent = parent;
    Centre();
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );
    this->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90,
                           false, wxEmptyString ) );

    /* Sizer 1 creation */
    wxFlexGridSizer* fgSizer1;
    fgSizer1 = new wxFlexGridSizer( 1, 1, 0, 0 );
    fgSizer1->SetFlexibleDirection( wxBOTH );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    // Selection des options :
    m_Include_Modules = new wxCheckBox( this, -1, _( "Include Modules" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Modules->SetValue( Block_Include_Modules );
    fgSizer1->Add( m_Include_Modules, 0, wxALL, 5 );

    m_Include_Tracks = new wxCheckBox( this, -1, _( "Include tracks" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Tracks->SetValue( Block_Include_Tracks );
    fgSizer1->Add( m_Include_Tracks, 0, wxALL, 5 );

    m_Include_Zones = new wxCheckBox( this, -1, _( "Include zones" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Zones->SetValue( Block_Include_Zones );
    fgSizer1->Add( m_Include_Zones, 0, wxALL, 5 );

    m_Include_PcbTextes = new wxCheckBox( this, -1,
                                          _( "Include Text on copper layers" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_PcbTextes->SetValue( Block_Include_PcbTextes );
    fgSizer1->Add( m_Include_PcbTextes, 0, wxALL, 5 );

    m_Include_Draw_Items = new wxCheckBox( this, -1, _( "Include drawings" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Draw_Items->SetValue( Block_Include_Draw_Items );
    fgSizer1->Add( m_Include_Draw_Items, 0, wxALL, 5 );

    m_Include_Edges_Items = new wxCheckBox( this, -1, _( "Include board outline layer" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Edges_Items->SetValue( Block_Include_Edges_Items );
    fgSizer1->Add( m_Include_Edges_Items, 0, wxALL, 5 );

    /* Sizer 2 creation */
    wxFlexGridSizer* fgSizer2;
    fgSizer2 = new wxFlexGridSizer( 1, 2, 0, 0 );
    fgSizer2->SetFlexibleDirection( wxBOTH );
    fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    /* Creation des boutons de commande */
    m_button2 = new wxButton( this, wxID_CANCEL, _( "Cancel" ), wxDefaultPosition, wxDefaultSize, 0 );
    fgSizer2->Add( m_button2, 0, wxALL, 5 );
    m_button1 = new wxButton( this, wxID_OK, _( "OK" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_button1->SetDefault();
    fgSizer2->Add( m_button1, 0, wxALL, 5 );

    fgSizer1->Add( fgSizer2, 1, wxALIGN_RIGHT, 5 );
    this->SetSizer( fgSizer1 );
    this->Layout();
    fgSizer1->Fit( this );
}


/**********************************************************************/
void WinEDA_ExecBlockCmdFrame::Cancel( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( -1 );
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
 *  0 si aucun composant selectionne
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

    Color = YELLOW;
    GRSetDrawMode( DC, g_XorMode );

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

    if( screen->BlockLocate.m_State != STATE_BLOCK_STOP )
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
    SetCurItem( NULL );

    /* Effacement des modules */
    if( Block_Include_Modules )
    {
        MODULE* module;
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = (MODULE*) NextS )
        {
            NextS = module->Next();
            if( module->HitTest( GetScreen()->BlockLocate ) )
            {
                module->m_Flags = 0;
                module->DeleteStructure();
                m_Pcb->m_Status_Pcb = 0;
            }
        }
    }

    /* Effacement des Pistes */
    if( Block_Include_Tracks )
    {
        TRACK* pt_segm;

        for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) NextS )
        {
            NextS = pt_segm->Next();
            if( pt_segm->HitTest( GetScreen()->BlockLocate ) )
            {
                /* la piste est ici bonne a etre efface */
                pt_segm->DeleteStructure();
            }
        }
    }

    /* Effacement des Elements De Dessin */
    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;

    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = NextS )
    {
        NextS = PtStruct->Next();

        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;

            /* l'element est ici bon a etre efface */
            PtStruct->Draw( DrawPanel, DC, GR_XOR );
            PtStruct->DeleteStructure();
            break;

        case TYPE_TEXTE:
            if( !Block_Include_PcbTextes )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* le texte est ici bon a etre efface */
            PtStruct->Draw( DrawPanel, DC, GR_XOR );
            /* Suppression du texte en Memoire*/
            PtStruct->DeleteStructure();
            break;

        case TYPE_MIRE:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre efface */
            PtStruct->DeleteStructure();
            break;

        case TYPE_COTATION:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            PtStruct->DeleteStructure();
            break;

        default:
            break;
        }
    }

    /* Effacement des Zones */
    if( Block_Include_Zones )
    {
        SEGZONE* pt_segm, *NextSegZ;

        Affiche_Message( _( "Delete zones" ) );
        for( pt_segm = m_Pcb->m_Zone; pt_segm != NULL; pt_segm = NextSegZ )
        {
            NextSegZ = pt_segm->Next();
            if( pt_segm->HitTest( GetScreen()->BlockLocate ) )
            {
                pt_segm->DeleteStructure();
            }
        }

        for ( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
        {
            if( m_Pcb->GetArea(ii)->HitTest( GetScreen()->BlockLocate ) )
            {
                m_Pcb->Delete(m_Pcb->GetArea(ii));
                ii--;	// because the current data was removed, ii points actually the next data
            }
        }
    }

    DrawPanel->Refresh( TRUE );
    Compile_Ratsnest( DC, TRUE );
}


/****************************************************/
void WinEDA_BasePcbFrame::Block_Rotate( wxDC* DC )
/****************************************************/

/**
 * Function Block_Rotate
 * Rotate 90 deg the selected block
 * The rotation centre is the centre of the block
 */
{
    MODULE* module;
    EDA_BaseStruct* PtStruct;
    int masque_layer;
    wxPoint oldpos;
    wxPoint centre;   	/* rotation centre for the rotation transform */

    if( !InstallBlockCmdFrame( this, _( "Rotate Block" ) ) )
        return;

    oldpos = GetScreen()->m_Curseur;
    GetScreen()->BlockLocate.Normalize();

    centre = GetScreen()->BlockLocate.Centre();	// This is the rotation centre

    GetScreen()->SetModify();

    /* Rotation des modules */
    if( Block_Include_Modules )
    {
        bool Show_Ratsnest_tmp = g_Show_Ratsnest; g_Show_Ratsnest = false;
        int Angle_Rot_Module   = 900;
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = module->Next() )
        {
            if( ! module->HitTest( GetScreen()->BlockLocate ) )
                continue;
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            /* Move the footprint before rotate it */
            RotatePoint( &module->m_Pos, centre, 900 );
            GetScreen()->m_Curseur = module->m_Pos;
            Place_Module( module, NULL );
            /* Rotate the footprint */
            Rotate_Module( DC, module, Angle_Rot_Module, TRUE );
        }

        /* regeneration des valeurs originelles */
        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Move and rotate the track segments */
    if( Block_Include_Tracks )
    {
        TRACK* track;
        track = m_Pcb->m_Track;
        while( track )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                RotatePoint( &track->m_Start, centre, 900 );
                RotatePoint( &track->m_End, centre, 900 );
            }
            track = track->Next();
        }
    }

    /* Move and rotate the zone fill segments, and outlines */
    if( Block_Include_Zones )
    {
        TRACK* track;

        Affiche_Message( _( "Zone rotation" ) );
        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {
                RotatePoint( &track->m_Start, centre, 900 );
                RotatePoint( &track->m_End, centre, 900 );
            }
            track = track->Next();
        }
        for ( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
        {
            if( m_Pcb->GetArea(ii)->HitTest( GetScreen()->BlockLocate ) )
            {
                m_Pcb->GetArea(ii)->Rotate(centre, 900);
            }
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    /* Move and rotate the graphic items */
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            #undef STRUCT
            #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            RotatePoint( &STRUCT->m_Start, centre, 900 );
            RotatePoint( &STRUCT->m_End, centre, 900 );
            break;

        case TYPE_TEXTE:
            #undef STRUCT
            #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            RotatePoint( &STRUCT->m_Pos, centre, 900 );
            STRUCT->m_Orient += 900;
            if( STRUCT->m_Orient >= 3600 )
                STRUCT->m_Orient -= 3600;
            break;

        case TYPE_MIRE:
            #undef STRUCT
            #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre modifie */
            RotatePoint( &STRUCT->m_Pos, centre, 900 );
            break;

        case TYPE_COTATION:
            #undef STRUCT
            #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            STRUCT->Rotate(centre, 900);
            break;

        default:
            break;
        }
    }

    DrawPanel->Refresh( TRUE );
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
{
#define INVERT( pos )       (pos) = centerY - ( (pos) - centerY )
#define INVERT_ANGLE( phi ) (phi) = -(phi)
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
        module = m_Pcb->m_Modules;
        for( ; module != NULL; module = module->Next() )
        {
            if( ! module->HitTest( GetScreen()->BlockLocate ) )
                continue;
            /* le module est ici bon a etre efface */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;

            /* calcul de la nouvelle position du Module */
            Ny = module->m_Pos.y;
            INVERT( Ny );
            GetScreen()->m_Curseur.x = module->m_Pos.x;
            GetScreen()->m_Curseur.y = Ny;
            Place_Module( module, NULL );

            /* inversion du module  */
            m_Pcb->Change_Side_Module( module, DC );

            /* regeneration des valeurs originelles */
            GetScreen()->m_Curseur = memo;
        }

        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track;

        track = m_Pcb->m_Track;
        while( track )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                INVERT( track->m_Start.y );
                INVERT( track->m_End.y );
                if( track->Type() != TYPE_VIA )
                {
                    track->SetLayer( ChangeSideNumLayer( track->GetLayer() ) );
                }
            }
            track = track->Next();
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        TRACK* track;

        track = (TRACK*) m_Pcb->m_Zone;
        while( track )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                INVERT( track->m_Start.y );
                INVERT( track->m_End.y );
                track->SetLayer( ChangeSideNumLayer( track->GetLayer() ) );
            }
            track = track->Next();
        }
        for ( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
        {
            if( m_Pcb->GetArea(ii)->HitTest( GetScreen()->BlockLocate ) )
            {
                m_Pcb->GetArea(ii)->Mirror( wxPoint(0, centerY) );
                m_Pcb->GetArea(ii)->SetLayer( ChangeSideNumLayer( m_Pcb->GetArea(ii)->GetLayer() ) );
            }
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            #undef STRUCT
            #define STRUCT ( (DRAWSEGMENT*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre selectionne */
            if( STRUCT->m_Shape == S_ARC )
            {
                INVERT_ANGLE( STRUCT->m_Angle );
            }
            INVERT( STRUCT->m_Start.y );
            INVERT( STRUCT->m_End.y );
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            break;

        case TYPE_TEXTE:
            #undef STRUCT
            #define STRUCT ( (TEXTE_PCB*) PtStruct )
            if( !Block_Include_PcbTextes )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* le texte est ici bon a etre selectionne*/
            INVERT( STRUCT->m_Pos.y );
            INVERT_ANGLE( STRUCT->m_Orient );
            if( (STRUCT->GetLayer() == COPPER_LAYER_N) || (STRUCT->GetLayer() == CMP_N) )
            {
                STRUCT->m_Mirror = not STRUCT->m_Mirror;      /* inverse miroir */
            }
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            break;

        case TYPE_MIRE:
            #undef STRUCT
            #define STRUCT ( (MIREPCB*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre modifie */
            INVERT( STRUCT->m_Pos.y );
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            break;

        case TYPE_COTATION:
            #undef STRUCT
            #define STRUCT ( (COTATION*) PtStruct )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! PtStruct->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre modifie */

            STRUCT->Mirror( wxPoint(0, centerY) );
            STRUCT->SetLayer( ChangeSideNumLayer( STRUCT->GetLayer() ) );
            break;

        default:
            break;
        }
    }

    DrawPanel->Refresh( TRUE );
    Compile_Ratsnest( DC, TRUE );
}


/************************************************/
void WinEDA_BasePcbFrame::Block_Move( wxDC* DC )
/************************************************/

/*
 *  Function to move items withing the selected block
 */
{
    int masque_layer;
    wxPoint oldpos;
    wxPoint MoveVector = GetScreen()->BlockLocate.m_MoveVector;

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
        oldpos = GetScreen()->m_Curseur;

        for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
        {
            if( ! module->HitTest( GetScreen()->BlockLocate ) )
                continue;

            /* le module est ici bon a etre deplace */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            GetScreen()->m_Curseur = module->m_Pos + MoveVector;
            Place_Module( module, NULL );
        }

        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        for( TRACK* track = m_Pcb->m_Track;  track;  track = track->Next() )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;
                track->m_Start += MoveVector;
                track->m_End   += MoveVector;
            }
        }
    }

    /* Deplacement des Segments de Zone */
    if( Block_Include_Zones )
    {
        for( TRACK* track = m_Pcb->m_Zone;  track;  track = track->Next() )
        {
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {                                           /* la piste est ici bonne a etre deplacee */
                track->m_Start += MoveVector;
                track->m_End   += MoveVector;
            }
        }
        for ( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
        {
            if( m_Pcb->GetArea(ii)->HitTest( GetScreen()->BlockLocate ) )
            {
                m_Pcb->GetArea(ii)->Move( MoveVector );
            }
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    for( BOARD_ITEM* item = m_Pcb->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
            #undef STRUCT
            #define STRUCT ( (DRAWSEGMENT*) item )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! item->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre efface */
            STRUCT->m_Start += MoveVector;
            STRUCT->m_End   += MoveVector;
            break;

        case TYPE_TEXTE:
            #undef STRUCT
            #define STRUCT ( (TEXTE_PCB*) item )
            if( !Block_Include_PcbTextes )
                break;
            if( ! item->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* le texte est ici bon a etre deplace */
            /* Redessin du Texte */
            STRUCT->m_Pos += MoveVector;
            break;

        case TYPE_MIRE:
            #undef STRUCT
            #define STRUCT ( (MIREPCB*) item )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! item->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre efface */
            STRUCT->m_Pos += MoveVector;
            break;

        case TYPE_COTATION:
            #undef STRUCT
            #define STRUCT ( (COTATION*) item )
            if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                break;
            if( ! item->HitTest( GetScreen()->BlockLocate ) )
                break;
            /* l'element est ici bon a etre efface */
            ( (COTATION*) item )->Move( wxPoint(MoveVector) );
            break;

        default:
            break;
        }
    }

    DrawPanel->Refresh( TRUE );
    Compile_Ratsnest( DC, TRUE );
}


/**************************************************/
void WinEDA_BasePcbFrame::Block_Duplicate( wxDC* DC )
/**************************************************/

/*
 *  routine de duplication des elements du block deja selectionne
 */
{
    int masque_layer;
    wxPoint oldpos;
    wxPoint MoveVector = GetScreen()->BlockLocate.m_MoveVector;

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
        bool Show_Ratsnest_tmp = g_Show_Ratsnest;
        g_Show_Ratsnest = false;
        oldpos = GetScreen()->m_Curseur;

        for( MODULE* module= m_Pcb->m_Modules;  module;  module = module->Next() )
        {
            MODULE* new_module;
            if( ! module->HitTest( GetScreen()->BlockLocate ) )
                continue;

            /* le module est ici bon a etre deplace */
            m_Pcb->m_Status_Pcb = 0;
            module->m_Flags = 0;
            new_module = new MODULE( m_Pcb );
            new_module->Copy( module );
            new_module->m_TimeStamp = GetTimeStamp();

            m_Pcb->m_Modules.PushFront( new_module );

            GetScreen()->m_Curseur = module->m_Pos + MoveVector;
            Place_Module( new_module, NULL );
        }

        GetScreen()->m_Curseur = oldpos;
        g_Show_Ratsnest = Show_Ratsnest_tmp;
    }

    /* Deplacement des Segments de piste */
    if( Block_Include_Tracks )
    {
        TRACK* track, * next_track, * new_track;
        track = m_Pcb->m_Track;
        while( track )
        {
            next_track = track->Next();
            if( track->HitTest( GetScreen()->BlockLocate ) )
            {
                /* la piste est ici bonne a etre deplacee */
                m_Pcb->m_Status_Pcb = 0;

                new_track = track->Copy();
                m_Pcb->m_Track.PushFront( new_track );

                new_track->m_Start += MoveVector;
                new_track->m_End += MoveVector;
            }
            track = next_track;
        }
    }

    /* Duplicate Zones */
    if( Block_Include_Zones )
    {
        for( SEGZONE* segzone = m_Pcb->m_Zone;  segzone;  segzone = segzone->Next() )
        {
            if( segzone->HitTest( GetScreen()->BlockLocate ) )
            {
                SEGZONE* new_segzone = (SEGZONE*) segzone->Copy();

                m_Pcb->m_Zone.PushFront( new_segzone );

                new_segzone->m_Start += MoveVector;
                new_segzone->m_End   += MoveVector;
            }
        }

        unsigned imax = m_Pcb->GetAreaCount();
        for ( unsigned ii = 0; ii < imax; ii++ )
        {
            if( m_Pcb->GetArea(ii)->HitTest( GetScreen()->BlockLocate ) )
            {
                ZONE_CONTAINER * new_zone = new ZONE_CONTAINER(m_Pcb);
                new_zone->Copy( m_Pcb->GetArea(ii) );
                new_zone->m_TimeStamp = GetTimeStamp();
                new_zone->Move( MoveVector );
                m_Pcb->Add(new_zone);
            }
        }
    }

    masque_layer = EDGE_LAYER;
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;
    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    for( BOARD_ITEM* item = m_Pcb->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
            {
                #undef STRUCT
                #define STRUCT ( (DRAWSEGMENT*) item )
                if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                    break;
                if( ! item->HitTest( GetScreen()->BlockLocate ) )
                    break;

                /* l'element est ici bon a etre copie */
                DRAWSEGMENT* new_drawsegment = new DRAWSEGMENT( m_Pcb );
                new_drawsegment->Copy( STRUCT );

                m_Pcb->Add( new_drawsegment );

                new_drawsegment->m_Start += MoveVector;
                new_drawsegment->m_End   += MoveVector;
            }
            break;

        case TYPE_TEXTE:
            {
                #undef STRUCT
                #define STRUCT ( (TEXTE_PCB*) item )
                if( !Block_Include_PcbTextes )
                    break;
                if( ! item->HitTest( GetScreen()->BlockLocate ) )
                    break;
                /* le texte est ici bon a etre deplace */
                TEXTE_PCB* new_pcbtext = new TEXTE_PCB( m_Pcb );
                new_pcbtext->Copy( STRUCT );

                m_Pcb->Add( new_pcbtext );

                /* Redessin du Texte */
                new_pcbtext->m_Pos += MoveVector;
            }
            break;

        case TYPE_MIRE:
            {
                #undef STRUCT
                #define STRUCT ( (MIREPCB*) item )
                if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                    break;
                if( ! item->HitTest( GetScreen()->BlockLocate ) )
                    break;
                /* l'element est ici bon a etre efface */
                MIREPCB* new_mire = new MIREPCB( m_Pcb );
                new_mire->Copy( STRUCT );

                m_Pcb->Add( new_mire );

                new_mire->m_Pos += MoveVector;
            }
            break;

        case TYPE_COTATION:
            {
                #undef STRUCT
                #define STRUCT ( (COTATION*) item )
                if( (g_TabOneLayerMask[STRUCT->GetLayer()] & masque_layer) == 0 )
                    break;
                if( ! item->HitTest( GetScreen()->BlockLocate ) )
                    break;
                /* l'element est ici bon a etre copie */
                COTATION* new_cotation = new COTATION( m_Pcb );
                new_cotation->Copy( STRUCT );

                m_Pcb->Add( new_cotation );

                new_cotation->Move( MoveVector );
            }
            break;

        default:
            break;
        }
    }

    DrawPanel->Refresh( TRUE );
    Compile_Ratsnest( DC, TRUE );

}
