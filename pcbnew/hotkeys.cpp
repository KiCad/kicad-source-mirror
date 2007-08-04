/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "id.h"

#include "protos.h"

/* Routines locales */

/* variables externes */

/***********************************************************/
void WinEDA_PcbFrame::OnHotKey( wxDC* DC, int hotkey,
                                EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Gestion des commandes rapides (Raccourcis claviers) concernant l'element
 *  sous le courseur souris
 *  Les majuscules/minuscules sont indifferenciees
 *  touche DELETE: Effacement (Module ou piste selon commande en cours)
 *  touche V: Place via en cours de trace de piste
 *  touche R: Rotation module
 *  touche S: Change couche module (Composant <-> Cuivre)
 *  touche M: Start Move module
 *  touche G: Start Drag module
 */
{
    bool PopupOn = GetScreen()->m_CurrentItem
                   && GetScreen()->m_CurrentItem->m_Flags;
                   
    bool ItemFree = (GetScreen()->m_CurrentItem == 0 )
                    || (GetScreen()->m_CurrentItem->m_Flags == 0);

    if( hotkey == 0 )
        return;

    // code Ctrl A = 1, Ctr B = 2 ..., remapped, (more easy to understand in switch)
    if( hotkey & GR_KB_CTRL )
        hotkey += 'A' - 1;

    MODULE* module = NULL;

    if( hotkey <= 0xFF )
        hotkey = toupper( hotkey );

    switch( hotkey )
    {
    case WXK_DELETE:
    case WXK_NUMPAD_DELETE:
        OnHotkeyDeleteItem( DC, DrawStruct );
        break;

    case WXK_BACK:
        if( m_ID_current_state == ID_TRACK_BUTT && GetScreen()->m_Active_Layer <= CMP_N )
        {
            bool ItemFree = (GetScreen()->m_CurrentItem == NULL )
                            || (GetScreen()->m_CurrentItem->m_Flags == 0);
            if( ItemFree )
            {
                // no track is currently being edited - select a segment and remove it.
                DrawStruct = PcbGeneralLocateAndDisplay();

                // don't let backspace delete modules!!
                if( DrawStruct && (DrawStruct->m_StructType == TYPETRACK
                                   || DrawStruct->m_StructType == TYPEVIA) )
                    Delete_Segment( DC, (TRACK*) DrawStruct );
                GetScreen()->SetModify();
            }
            else if( GetScreen()->m_CurrentItem->m_StructType == TYPETRACK  )
            {
                // then an element is being edited - remove the last segment.
                GetScreen()->m_CurrentItem =
                    Delete_Segment( DC, (TRACK*) GetScreen()->m_CurrentItem );
                GetScreen()->SetModify();
            }
        }
        break;

    case WXK_END:
        DrawPanel->MouseToCursorSchema();
        End_Route( (TRACK*) (GetScreen()->m_CurrentItem), DC );
        break;

    case 'F' + GR_KB_CTRL:
    {
        wxCommandEvent evt;
        evt.SetId( ID_FIND_ITEMS );
        Process_Special_Functions( evt );
    }
        break;

    case 'O' + GR_KB_CTRL:
    {
        // try not to duplicate save, load code etc.
        wxCommandEvent evt;
        evt.SetId( ID_LOAD_FILE );
        Files_io( evt );
    }
        break;

    case 'S' + GR_KB_CTRL:
    {
        // try not to duplicate save, load code etc.
        wxCommandEvent evt;
        evt.SetId( ID_SAVE_BOARD );
        Files_io( evt );
    }
        break;

    case 'V':      // Switch to alternate layer and Place a via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( ItemFree )
        {
            Other_Layer_Route( NULL, DC );
            break;
        }
        if( GetScreen()->m_CurrentItem->m_StructType != TYPETRACK )
            return;
        if( (GetScreen()->m_CurrentItem->m_Flags & IS_NEW) == 0 )
            return;
        Other_Layer_Route( (TRACK*) GetScreen()->m_CurrentItem, DC );
        if( DisplayOpt.ContrastModeDisplay )
            GetScreen()->SetRefreshReq();
        break;

        // Footprint edition:
    case 'L':      // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( ItemFree )
            module = Locate_Prefered_Module( m_Pcb, CURSEUR_OFF_GRILLE | VISIBLE_ONLY );
        else if( GetScreen()->m_CurrentItem->m_StructType == TYPEMODULE )
            module = (MODULE*) GetScreen()->m_CurrentItem;
        if( module )
        {
            GetScreen()->m_CurrentItem = module;
            module->SetLocked( !module->IsLocked() );
            module->Display_Infos( this );
        }
        break;

    case 'G':       // Start move (and drag) module
    case 'M':       // Start move module
        if( PopupOn )
            break;

    case 'R':       // Rotation
    case 'S':       // move to other side
        if( ItemFree )
        {
            module = Locate_Prefered_Module( m_Pcb,
                                             CURSEUR_OFF_GRILLE | IGNORE_LOCKED | VISIBLE_ONLY
#if defined (USE_MATCH_LAYER)
                                             | MATCH_LAYER
#endif
                     );
            if( module == NULL )      // no footprint found
            {
                module = Locate_Prefered_Module( m_Pcb, CURSEUR_OFF_GRILLE );
                if( module )      
                {
                    // a footprint is found, but locked or on an other layer
                    if( module->IsLocked() )
                    {
                        wxString msg;
                        
                        msg.Printf( _("Footprint %s found, but locked"),
                            module->m_Reference->m_Text.GetData() );
                        
                        DisplayInfo( this, msg );
                    }
                    module = NULL;
                }
            }
        }
        else if( GetScreen()->m_CurrentItem->m_StructType == TYPEMODULE )
        {
            module = (MODULE*) GetScreen()->m_CurrentItem;

            // @todo: might need to add a layer check in if() below
            if( (GetScreen()->m_CurrentItem->m_Flags == 0)
               && module->IsLocked() )
                module = NULL; // do not move, rotate ... it.
        }
        if( module == NULL )
            break;

        GetScreen()->m_CurrentItem = module;

        switch( hotkey )
        {
        case 'R':          // Rotation
            Rotate_Module( DC, module, 900, TRUE );
            break;

        case 'S':          // move to other side
            Change_Side_Module( module, DC );
            break;

        case 'G':          // Start move (and drag) module
            g_Drag_Pistes_On = TRUE;

            // fall through

        case 'M':          // Start move module
            StartMove_Module( module, DC );
            break;
        }

        module->Display_Infos( this );
        break;
    }
}


/***********************************************************/
void WinEDA_ModuleEditFrame::OnHotKey( wxDC* DC, int hotkey,
                                       EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Gestion des commandes rapides (Raccourcis claviers) concernant l'element
 *  sous le courseur souris
 *  Les majuscules/minuscules sont indifferenciees
 */
{
    bool PopupOn = GetScreen()->m_CurrentItem
                   && GetScreen()->m_CurrentItem->m_Flags;

    if( hotkey == 0 )
        return;

    switch( hotkey )
    {
    case WXK_DELETE:
    case WXK_NUMPAD_DELETE:
        if( PopupOn )
            break;
        break;

    case 'r':      // Rotation
    case 'R':
        break;

    case 'y':      // Mirror Y (drawlibpart)
    case 'Y':
        break;

    case 'x':      // Mirror X (drawlibpart)
    case 'X':
        break;

    case 'n':
    case 'N':      // Orient 0, no mirror (drawlibpart)
        break;

    case 'm':
    case 'M':      // Start move drawlibpart
        if( PopupOn )
            break;
        break;
    }
}


/******************************************************************************/
bool WinEDA_PcbFrame::OnHotkeyDeleteItem( wxDC* DC, EDA_BaseStruct* DrawStruct )
/******************************************************************************/

/* Efface l'item pointe par la souris, en reponse a la touche "Del"
 *  Effet dependant de l'outil selectionne:
 *      Outil trace de pistes
 *          Efface le segment en cours ou la piste si pas d'element
 *      Outil module:
 *          Efface le module.
 */
{
    bool ItemFree = (GetScreen()->m_CurrentItem == NULL )
                    || (GetScreen()->m_CurrentItem->m_Flags == 0);

    switch( m_ID_current_state )
    {
    case ID_TRACK_BUTT:
        if( GetScreen()->m_Active_Layer > CMP_N )
            return FALSE;
        if( ItemFree )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct && DrawStruct->m_StructType != TYPETRACK )
                return FALSE;
            Delete_Track( DC, (TRACK*) DrawStruct );
        }
        else if( GetScreen()->m_CurrentItem->m_StructType == TYPETRACK  )
        {
            GetScreen()->m_CurrentItem =
                Delete_Segment( DC, (TRACK*) GetScreen()->m_CurrentItem );
            GetScreen()->SetModify();
            return TRUE;
        }
        break;

    case ID_COMPONENT_BUTT:
        if( ItemFree )
        {
            MODULE* module = Locate_Prefered_Module( m_Pcb, CURSEUR_ON_GRILLE );
            if( module == NULL )
                return FALSE;
            if( !IsOK( this, _( "Delete module?" ) ) )
                return FALSE;
            RemoveStruct( module, DC );
        }
        else
            return FALSE;
        break;

    default:
        return FALSE;
    }

    GetScreen()->SetModify();
    GetScreen()->m_CurrentItem = NULL;
    return TRUE;
}
