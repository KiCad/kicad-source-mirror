/********************************************************/
/* Modification de la place, orient, nombre des MODULES */
/********************************************************/

/* Fichier MODULES.Cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "protos.h"

#include "drag.h"

/* fonctions externes */

/* Fonctions locales */
static int  ChangeSideMaskLayer( int masque );
static void Exit_Module( WinEDA_DrawPanel* Panel, wxDC* DC );

/* Variables locales : */
static int ModuleInitOrient;    // Lors des moves, val init de l'orient (pour annulation)
static int ModuleInitLayer;     // Lors des moves, val init de la couche (pour annulation)


/*************************************************************************/
void Show_Pads_On_Off( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* module )
/**************************************************************************/

/* Fonction appelee lors de l'activation/desactivation de la visualisation
 *  des Pads du module en deplacement
 *  Effacement ou visu des Pads selon conditions initiales
 */
{
    D_PAD* pt_pad;
    bool   pad_fill_tmp;

    if( module == 0 )
        return;

    pad_fill_tmp = DisplayOpt.DisplayPadFill;
    DisplayOpt.DisplayPadFill = FALSE; /* Trace en SKETCH */
    pt_pad = module->m_Pads;
    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
    {
        pt_pad->Draw( panel, DC, g_Offset_Module, GR_XOR );
    }

    DisplayOpt.DisplayPadFill = pad_fill_tmp;
}


/***************************************************************************/
/* Fonction appelee lors de l'activation/desactivation de la visualisation */
/*	du rastnest du module en deplacement									  */
/*	Effacement ou visu du rastnest selon conditions initiales			  */
/***************************************************************************/
void Rastnest_On_Off( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* module )
{
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->m_Parent;

    frame->build_ratsnest_module( DC, module );
    frame->trace_ratsnest_module( DC );
}


/***************************************************/
MODULE* WinEDA_BasePcbFrame::GetModuleByName()
/***************************************************/

/* Get a module name from user and return a pointer to the corresponding module
 */
{
    wxString modulename;
    MODULE*  module = NULL;

    Get_Message( _( "Footprint name:" ), modulename, this );
    if( !modulename.IsEmpty() )
    {
        module = m_Pcb->m_Modules;
        while( module )
        {
            if( module->m_Reference->m_Text.CmpNoCase( modulename ) == 0 )
                break;
            module = module->Next();
        }
    }
    return module;
}


/**********************************************************************/
void WinEDA_PcbFrame::StartMove_Module( MODULE* module, wxDC* DC )
/**********************************************************************/
{
    if( module == NULL )
        return;

    SetCurItem( module );
    m_Pcb->m_Status_Pcb &= ~CHEVELU_LOCAL_OK;
    module->m_Flags |= IS_MOVED;
    ModuleInitOrient = module->m_Orient;
    ModuleInitLayer  = module->GetLayer();

    /* Effacement chevelu general si necessaire */
    if( g_Show_Ratsnest )
        DrawGeneralRatsnest( DC );

    if( g_DragSegmentList ) /* Anormal ! */
    {
        EraseDragListe();
    }

    if( g_Drag_Pistes_On )
    {
        Build_Drag_Liste( DrawPanel, DC, module );
    }

    m_Pcb->m_Status_Pcb     |= DO_NOT_SHOW_GENERAL_RASTNEST;
    DrawPanel->ManageCurseur = Montre_Position_Empreinte;
    DrawPanel->ForceCloseManageCurseur = Exit_Module;
    DrawPanel->m_AutoPAN_Request = TRUE;

    // effacement module a l'ecran:
    module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

    // Reaffichage
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
}


/**************************************************/
void Exit_Module( WinEDA_DrawPanel* Panel, wxDC* DC )
/***************************************************/
/* fonction de sortie de l'application */
{
    DRAG_SEGM*           pt_drag;
    TRACK*               pt_segm;
    MODULE*              module;
    WinEDA_BasePcbFrame* pcbframe = (WinEDA_BasePcbFrame*) Panel->m_Parent;

    module = (MODULE*) pcbframe->m_CurrentScreen->GetCurItem();
    pcbframe->m_Pcb->m_Status_Pcb &= ~CHEVELU_LOCAL_OK;

    if( module )
    {
        // effacement module a l'ecran:
        DrawModuleOutlines( Panel, DC, module );
        /* restitution de l'empreinte si move ou effacement copie*/
        if( module->m_Flags & IS_MOVED )
        {
            /* Move en cours : remise a l'etat d'origine */
            if( g_Drag_Pistes_On )
            {
                /* Effacement des segments dragges */
                pt_drag = g_DragSegmentList;
                for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
                {
                    pt_segm = pt_drag->m_Segm;
                    pt_segm->Draw( Panel, DC, GR_XOR );
                }
            }

            /* Remise en etat d'origine des segments dragges */
            pt_drag = g_DragSegmentList;
            for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
            {
                pt_segm = pt_drag->m_Segm; pt_segm->SetState( EDIT, OFF );
                pt_drag->SetInitialValues();
                pt_segm->Draw( Panel, DC, GR_OR );
            }

            EraseDragListe();
            module->m_Flags = 0;
        }

        if( module->m_Flags & IS_NEW )
        {
            module ->DeleteStructure();
            module = NULL;
            pcbframe->m_Pcb->m_Status_Pcb = 0;
            pcbframe->build_liste_pads();
        }
    }

    /* Reaffichage du module a l'ecran */
    if( module )
    {
        if( ModuleInitOrient != module->m_Orient )
            pcbframe->Rotate_Module( NULL, module, ModuleInitOrient, FALSE );
        if( ModuleInitLayer != module->GetLayer() )
            pcbframe->Change_Side_Module( module, NULL );
        module->Draw( Panel, DC, wxPoint( 0, 0 ), GR_OR );
    }
    g_Drag_Pistes_On     = FALSE;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
}


/**********************************************************/
MODULE* WinEDA_BasePcbFrame::Copie_Module( MODULE* module )
/**********************************************************/
/* copie le module "module" en position courante */
{
    MODULE* newmodule;

    if( module == NULL )
        return NULL;

    m_CurrentScreen->SetModify();

    /* Duplication du module */
    m_Pcb->m_Status_Pcb = 0;
    newmodule = new MODULE( m_Pcb );
    newmodule->Copy( module );
    newmodule->m_Parent = m_Pcb;
    newmodule->AddToChain( module );
    newmodule->m_Flags = IS_NEW;

    build_liste_pads();

    newmodule->Display_Infos( this );
    m_Pcb->m_Status_Pcb &= ~CHEVELU_LOCAL_OK;
    return newmodule;
}


/**********************************************************************************/
void Montre_Position_Empreinte( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/**********************************************************************************/

/* redessin du contour de l'empreinte  lors des deplacements de la souris
 */
{
    MODULE* module = (MODULE*) panel->m_Parent->m_CurrentScreen->GetCurItem();

    if(  module == NULL )
        return;

    /* efface ancienne position */
    if( erase )
    {
        DrawModuleOutlines( panel, DC, module );
    }

    /* Redessine le module a la nouvelle place */
    g_Offset_Module.x = module->m_Pos.x - panel->m_Parent->m_CurrentScreen->m_Curseur.x;
    g_Offset_Module.y = module->m_Pos.y - panel->m_Parent->m_CurrentScreen->m_Curseur.y;
    DrawModuleOutlines( panel, DC, module );

    Dessine_Segments_Dragges( panel, DC );
}


/**************************************************************/
bool WinEDA_PcbFrame::Delete_Module( MODULE* module, wxDC* DC )
/**************************************************************/

/*
 *  Commande Delete Module :
 *  Suppression d'une empreinte
 *  les pointeurs divers sont mis a jour
 */
{
    EDA_BaseStruct* PtBack, * PtNext;
    wxString        msg;

    /* Si l'empreinte est selectee , on ne peut pas l'effacer ! */
    if( module == NULL )
        return FALSE;

    /* Confirmation de l'effacement */
    module->Display_Infos( this );

    msg << _( "Delete Module" ) << wxT( " " ) << module->m_Reference->m_Text
        << wxT( "  (" ) << _( "Value " ) << module->m_Value->m_Text
        << wxT( ") ?" );
    if( !IsOK( this, msg ) )
    {
        return FALSE;
    }

    m_CurrentScreen->SetModify();

    /* Erase rastnest if needed */
    if( g_Show_Ratsnest )
        DrawGeneralRatsnest( DC );

    /* Effacement du module a l'ecran */
    if( DC )
        module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

    /* Suppression du chainage */
    PtBack = module->Pback;
    PtNext = module->Pnext;
    if( PtBack == (EDA_BaseStruct*) m_Pcb )
    {
        m_Pcb->m_Modules = (MODULE*) PtNext;
    }
    else
    {
        PtBack->Pnext = PtNext;
    }
    if( PtNext )
        PtNext->Pback = PtBack;

    /* Sauvegarde en buffer des undelete */
    SaveItemEfface( module, 1 );

    m_Pcb->m_Status_Pcb = 0;
    build_liste_pads();
    ReCompile_Ratsnest_After_Changes( DC );
    return TRUE;
}


/**********************************************************************/
void WinEDA_BasePcbFrame::Change_Side_Module( MODULE* Module, wxDC* DC )
/**********************************************************************/

/* Change de cote un composant : il y a inversion MIROIR autour de l'axe X
 *  Le changement n'est fait que si la couche est
 *      - CUIVRE ou CMP
 *      Si DC == NULL, il n'y a pas de redessin du composant et du chevelu
 */
{
    D_PAD*          pt_pad;
    TEXTE_MODULE*   pt_texte;
    EDGE_MODULE*    pt_edgmod;
    EDA_BaseStruct* PtStruct;

    if( Module == NULL )
        return;
    if( (Module->GetLayer() != CMP_N) && (Module->GetLayer() != COPPER_LAYER_N) )
        return;

    m_CurrentScreen->SetModify();

    if( !(Module->m_Flags & IS_MOVED) )
    {
        m_Pcb->m_Status_Pcb &= ~( LISTE_CHEVELU_OK | CONNEXION_OK);
        if( DC )
            Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

        /* Effacement chevelu general si necessaire */
        if( DC && g_Show_Ratsnest )
            DrawGeneralRatsnest( DC );

        /* Init des variables utilisees dans la routine Dessine_Drag_segment() */
        g_Offset_Module.x = 0;
        g_Offset_Module.y = 0;
    }
    else    // Module en deplacement
    {
        /* efface empreinte ( vue en contours) si elle a ete deja dessinee */
        if( DC )
        {
            DrawModuleOutlines( DrawPanel, DC, Module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
    }

    /* mise a jour du Flag de l'empreinte et des couches des contours et textes */
    Module->SetLayer( ChangeSideNumLayer( Module->GetLayer() ) );

    /* Inversion miroir de l'orientation */
    Module->m_Orient = -Module->m_Orient;
    NORMALIZE_ANGLE_POS( Module->m_Orient );

    /* Inversion miroir + layers des pastilles */
    pt_pad = Module->m_Pads;
    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
    {
        pt_pad->m_Pos.y      -= Module->m_Pos.y;
        pt_pad->m_Pos.y       = -pt_pad->m_Pos.y;
        pt_pad->m_Pos.y      += Module->m_Pos.y;
        pt_pad->m_Pos0.y      = -pt_pad->m_Pos0.y;
        pt_pad->m_Offset.y    = -pt_pad->m_Offset.y;
        pt_pad->m_DeltaSize.y = -pt_pad->m_DeltaSize.y;
        NEGATE_AND_NORMALIZE_ANGLE_POS( pt_pad->m_Orient );

        /* change cote pour pastilles surfaciques */
        pt_pad->m_Masque_Layer = ChangeSideMaskLayer( pt_pad->m_Masque_Layer );
    }

    /* Inversion miroir de la Reference et mise en miroir : */
    pt_texte = Module->m_Reference;
    pt_texte->m_Pos.y -= Module->m_Pos.y;
    pt_texte->m_Pos.y  = -pt_texte->m_Pos.y;
    pt_texte->m_Pos.y += Module->m_Pos.y;
    pt_texte->m_Pos0.y = pt_texte->m_Pos0.y;
    pt_texte->m_Miroir = 1;
    NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );
    pt_texte->SetLayer( Module->GetLayer() );
    pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

    if( Module->GetLayer() == COPPER_LAYER_N )
        pt_texte->SetLayer( SILKSCREEN_N_CU );

    if( Module->GetLayer() == CMP_N )
        pt_texte->SetLayer( SILKSCREEN_N_CMP );

    if( (Module->GetLayer() == SILKSCREEN_N_CU)
       || (Module->GetLayer() == ADHESIVE_N_CU) || (Module->GetLayer() == COPPER_LAYER_N) )
        pt_texte->m_Miroir = 0;

    /* Inversion miroir de la Valeur et mise en miroir : */
    pt_texte = Module->m_Value;
    pt_texte->m_Pos.y -= Module->m_Pos.y;
    pt_texte->m_Pos.y  = -pt_texte->m_Pos.y;
    pt_texte->m_Pos.y += Module->m_Pos.y;
    pt_texte->m_Pos0.y = pt_texte->m_Pos0.y;
    pt_texte->m_Miroir = 1;
    NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );
    pt_texte->SetLayer( Module->GetLayer() );
    pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

    if( Module->GetLayer() == COPPER_LAYER_N )
        pt_texte->SetLayer( SILKSCREEN_N_CU );

    if( Module->GetLayer() == CMP_N )
        pt_texte->SetLayer( SILKSCREEN_N_CMP );

    if( (Module->GetLayer() == SILKSCREEN_N_CU)
       || (Module->GetLayer() == ADHESIVE_N_CU) || (Module->GetLayer() == COPPER_LAYER_N) )
        pt_texte->m_Miroir = 0;

    /* Inversion miroir des dessins de l'empreinte : */
    PtStruct = Module->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->Type() )
        {
        case TYPEEDGEMODULE:
            pt_edgmod = (EDGE_MODULE*) PtStruct;
            pt_edgmod->m_Start.y -= Module->m_Pos.y;
            pt_edgmod->m_Start.y  = -pt_edgmod->m_Start.y;
            pt_edgmod->m_Start.y += Module->m_Pos.y;
            pt_edgmod->m_End.y   -= Module->m_Pos.y;
            pt_edgmod->m_End.y    = -pt_edgmod->m_End.y;
            pt_edgmod->m_End.y   += Module->m_Pos.y;
            /* inversion des coords locales */
            pt_edgmod->m_Start0.y = -pt_edgmod->m_Start0.y;
            pt_edgmod->m_End0.y   = -pt_edgmod->m_End0.y;
            if( pt_edgmod->m_Shape == S_ARC )
            {
                pt_edgmod->m_Angle = -pt_edgmod->m_Angle;
            }

            pt_edgmod->SetLayer( ChangeSideNumLayer( pt_edgmod->GetLayer() ) );
            break;

        case TYPETEXTEMODULE:
            /* Inversion miroir de la position et mise en miroir : */
            pt_texte = (TEXTE_MODULE*) PtStruct;
            pt_texte->m_Pos.y -= Module->m_Pos.y;
            pt_texte->m_Pos.y  = -pt_texte->m_Pos.y;
            pt_texte->m_Pos.y += Module->m_Pos.y;
            pt_texte->m_Pos0.y = pt_texte->m_Pos0.y;
            pt_texte->m_Miroir = 1;
            NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );

            pt_texte->SetLayer( Module->GetLayer() ); 
            pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

            if( Module->GetLayer() == COPPER_LAYER_N )
                pt_texte->SetLayer( SILKSCREEN_N_CU );

            if( Module->GetLayer() == CMP_N )
                pt_texte->SetLayer( SILKSCREEN_N_CMP );

            if(  Module->GetLayer() == SILKSCREEN_N_CU
              || Module->GetLayer() == ADHESIVE_N_CU
              || Module->GetLayer() == COPPER_LAYER_N )
            {
                pt_texte->m_Miroir = 0;
            }

            break;

        default:
            DisplayError( this, wxT( "Unknown Draw Type" ) ); break;
        }
    }

    /* calcul du rectangle d'encadrement */
    Module->Set_Rectangle_Encadrement();

    Module->Display_Infos( this );

    if( !(Module->m_Flags & IS_MOVED) ) /* Inversion simple */
    {
        if( DC )
        {
            Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            
            /* affichage chevelu general si necessaire */
            ReCompile_Ratsnest_After_Changes( DC );
        }
    }
    else
    {
        if( DC )
        {
            DrawModuleOutlines( DrawPanel, DC, Module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
        m_Pcb->m_Status_Pcb &= ~CHEVELU_LOCAL_OK;
    }
}


/*********************************************/
static int ChangeSideMaskLayer( int masque )
/*********************************************/

/* Routine de recalcul du masque-layer lors des
 *  echanges cote cu/cmp pour les couches CU/CMP specialisees
 *  (cuivre, serigr., pate , soudure)
 */
{
    int newmasque;

    newmasque = masque & ~(CUIVRE_LAYER | CMP_LAYER |
                           SILKSCREEN_LAYER_CU | SILKSCREEN_LAYER_CMP |
                           ADHESIVE_LAYER_CU | ADHESIVE_LAYER_CMP |
                           SOLDERMASK_LAYER_CU | SOLDERMASK_LAYER_CMP |
                           SOLDERPASTE_LAYER_CU | SOLDERPASTE_LAYER_CMP |
                           ADHESIVE_LAYER_CU | ADHESIVE_LAYER_CMP);

    if( masque & CUIVRE_LAYER )
        newmasque |= CMP_LAYER;
    if( masque & CMP_LAYER )
        newmasque |= CUIVRE_LAYER;

    if( masque & SILKSCREEN_LAYER_CU )
        newmasque |= SILKSCREEN_LAYER_CMP;
    if( masque & SILKSCREEN_LAYER_CMP )
        newmasque |= SILKSCREEN_LAYER_CU;

    if( masque & ADHESIVE_LAYER_CU )
        newmasque |= ADHESIVE_LAYER_CMP;
    if( masque & ADHESIVE_LAYER_CMP )
        newmasque |= ADHESIVE_LAYER_CU;

    if( masque & SOLDERMASK_LAYER_CU )
        newmasque |= SOLDERMASK_LAYER_CMP;
    if( masque & SOLDERMASK_LAYER_CMP )
        newmasque |= SOLDERMASK_LAYER_CU;

    if( masque & SOLDERPASTE_LAYER_CU )
        newmasque |= SOLDERPASTE_LAYER_CMP;
    if( masque & SOLDERPASTE_LAYER_CMP )
        newmasque |= SOLDERPASTE_LAYER_CU;

    if( masque & ADHESIVE_LAYER_CU )
        newmasque |= ADHESIVE_LAYER_CMP;
    if( masque & ADHESIVE_LAYER_CMP )
        newmasque |= ADHESIVE_LAYER_CU;

    return newmasque;
}


/*************************************/
int ChangeSideNumLayer( int oldlayer )
/*************************************/

/* Routine de recalcul du numero de couche lors des
 *  echanges cote cu/cmp pour les couches CU/CMP specialisees
 *  (cuivre, serigr., pate , soudure)
 */
{
    int newlayer;

    switch( oldlayer )
    {
    case COPPER_LAYER_N:
        newlayer = CMP_N; break;

    case CMP_N:
        newlayer = COPPER_LAYER_N; break;

    case SILKSCREEN_N_CU:
        newlayer = SILKSCREEN_N_CMP; break;

    case SILKSCREEN_N_CMP:
        newlayer = SILKSCREEN_N_CU; break;

    case ADHESIVE_N_CU:
        newlayer = ADHESIVE_N_CMP; break;

    case ADHESIVE_N_CMP:
        newlayer = ADHESIVE_N_CU; break;

    case SOLDERMASK_N_CU:
        newlayer = SOLDERMASK_N_CMP; break;

    case SOLDERMASK_N_CMP:
        newlayer = SOLDERMASK_N_CU; break;

    case SOLDERPASTE_N_CU:
        newlayer = SOLDERPASTE_N_CMP; break;

    case SOLDERPASTE_N_CMP:
        newlayer = SOLDERPASTE_N_CU; break;

    default:
        newlayer = oldlayer;
    }

    return newlayer;
}


/*****************************************************************/
void WinEDA_BasePcbFrame::Place_Module( MODULE* module, wxDC* DC )
/*****************************************************************/

/* Place a l'endroit pointe par la souris le module deja existant selectionne
 *  auparavant.
 *  Entree: module = num du module a replacer
 *          DC ( si NULL: pas d'affichage a l'�ran
 *  Sortie :
 *          mise a jour des nouvelles coord des differents elements du module
 *          affichage a l'ecran du module
 */
{
    TRACK*     pt_segm;
    DRAG_SEGM* pt_drag;
    wxPoint    newpos;

    if( module == 0 )
        return;

    m_CurrentScreen->SetModify();
    m_Pcb->m_Status_Pcb &= ~( LISTE_CHEVELU_OK | CONNEXION_OK);

    if( g_Show_Module_Ratsnest && (m_Pcb->m_Status_Pcb & LISTE_PAD_OK) && DC )
        trace_ratsnest_module( DC );

    newpos = m_CurrentScreen->m_Curseur;

    module->SetPosition( newpos );
    if( DC )
        module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );

    /* Tracage des segments dragges et liberation memoire */
    if( g_DragSegmentList )
    {
        pt_drag = g_DragSegmentList;
        for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
        {
            pt_segm = pt_drag->m_Segm;
            pt_segm->SetState( EDIT, OFF );
            if( DC )
                pt_segm->Draw( DrawPanel, DC, GR_OR );
        }

        EraseDragListe();
    }

    /* affichage chevelu general si necessaire */
    ReCompile_Ratsnest_After_Changes( DC );

    module->Display_Infos( this );

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    module->m_Flags  = 0;
    g_Drag_Pistes_On = FALSE;
}


/***********************************************************************/
void WinEDA_BasePcbFrame::Rotate_Module( wxDC* DC, MODULE* module,
                                         int angle, bool incremental )
/***********************************************************************/

/*
 *  Fait tourner l'empreinte de angle degres, dans le sens < 0.
 *  Si incremental == TRUE, la rotation est faite a partir de la derniere orientation,
 *  sinon le module est mis dans l'orientation absolue angle.
 *  Si DC == NULL, le composant n'est pas redessine.
 *  Sinon, il est efface, tourne et redessine
 */
{
    if( module == NULL )
        return;

    m_CurrentScreen->SetModify();

    /* efface ancienne position */
    if( !(module->m_Flags & IS_MOVED) ) /* Rotation simple */
    {
        if( DC )
        {
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            /* Reaffichage chevelu general si necessaire */
            if( g_Show_Ratsnest )
                DrawGeneralRatsnest( DC );
        }
    }
    else
    {
        /* reaffiche module en mouvement */
        if( DC )
        {
            DrawModuleOutlines( DrawPanel, DC, module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
    }

    m_Pcb->m_Status_Pcb &= ~(LISTE_CHEVELU_OK | CONNEXION_OK);

    if( incremental )
        module->SetOrientation( module->m_Orient + angle );
    else
        module->SetOrientation( angle );

    module->Display_Infos( this );

    if( DC )
    {
        if( !(module->m_Flags & IS_MOVED) ) /* Rotation simple */
        {
            module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
            
            /* Reaffichage chevelu general si necessaire */
            ReCompile_Ratsnest_After_Changes( DC );
        }
        else
        {
            /* reaffiche module en mouvement */
            DrawModuleOutlines( DrawPanel, DC, module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
    }
}


/*************************************************/
/* Redessine en mode XOR la silouhette du module */
/*************************************************/

void DrawModuleOutlines( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* module )
{
    int    pad_fill_tmp;
    D_PAD* pt_pad;

    if( module == NULL )
        return;
    module->DrawEdgesOnly( panel, DC, g_Offset_Module, GR_XOR );

    if( g_Show_Pads_Module_in_Move )
    {
        pad_fill_tmp = DisplayOpt.DisplayPadFill;
        DisplayOpt.DisplayPadFill = SKETCH;  /* Trace en SKETCH en deplacement */

        pt_pad = module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
        {
            pt_pad->Draw( panel, DC, g_Offset_Module, GR_XOR );
        }

        DisplayOpt.DisplayPadFill = pad_fill_tmp;
    }

    if( g_Show_Module_Ratsnest && panel )
    {
        WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->m_Parent;
        frame->build_ratsnest_module( DC, module );
        frame->trace_ratsnest_module( DC );
    }
}
