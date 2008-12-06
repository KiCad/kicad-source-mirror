/********************************************************/
/* Effacements : Routines de sauvegarde et d'effacement */
/********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"


/* Routines externes : */

/* Routines Locales */

/********************************************/
void WinEDA_BasePcbFrame::UnDeleteItem( wxDC* DC )
/********************************************/

/* Restitution d'un element (MODULE ou TRACK ) Efface
 */
{
    BOARD_ITEM*     item;
    int             net_code = 0;

    if( !g_UnDeleteStackPtr )
        return;

    g_UnDeleteStackPtr--;
    item = g_UnDeleteStack[g_UnDeleteStackPtr];
    if( item == NULL )
        return;                     // Ne devrait pas se produire

    // we decremented the stack pointer, so the stack no longer
    // owns "item".  We do here, so we have to delete item if its
    // not going back into the board, see default case below.
    g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;


    switch( item->Type() )
    {
    case TYPE_VIA:
    case TYPE_TRACK:
        TRACK* track;
        track = (TRACK*) item;

        D(printf("%s: track %p status=\"%s\"\n", __func__, track,
                 CONV_TO_UTF8( TRACK::ShowState( track->GetState(-1)) )
                 );)

        track->SetState( DELETED, OFF );
        DrawPanel->PostDirtyRect( track->GetBoundingBox() );
        m_Pcb->Add( track );

        net_code = track->GetNet();

#if !defined(GERBVIEW)
        test_1_net_connexion( DC, net_code );
#endif

        m_Pcb->Display_Infos( this );
        break;

    case TYPE_BOARD_ITEM_LIST:
        BOARD_ITEM_LIST*  list;
        list = (BOARD_ITEM_LIST*) item;
        while( list->GetCount() )
        {
            TRACK* t = (TRACK*) list->Remove( 0 );
            wxASSERT( t->Type()==TYPE_TRACK || t->Type()==TYPE_VIA );
            t->SetState( DELETED, OFF );
            DrawPanel->PostDirtyRect( t->GetBoundingBox() );
            m_Pcb->Add( t );
            net_code = t->GetNet();
        }
        delete list;

#if !defined(GERBVIEW)
        test_1_net_connexion( DC, net_code );
#endif
        m_Pcb->Display_Infos( this );
        break;

#if !defined(GERBVIEW)
    case TYPE_MODULE:
        // Erase general rastnest if needed
        if( g_Show_Ratsnest )
            DrawGeneralRatsnest( DC );

        m_Pcb->Add( item );

        item->Draw( DrawPanel, DC, GR_OR );

        item->SetState( DELETED, OFF );     /* Creal DELETED flag */
        item->m_Flags   = 0;
        m_Pcb->m_Status_Pcb = 0;

        build_liste_pads();
        ReCompile_Ratsnest_After_Changes( DC );
        break;
#endif

    default:
        DisplayError( this, wxT( "WinEDA_PcbFrame::UnDeleteItem(): unexpected Struct type" ) );
        delete item;
        break;
    }
}


/* Sauvegarde d'un element aux fins de restitution par Undelete
 *  Supporte actuellement : Module et segments de piste
 */
BOARD_ITEM* WinEDA_BasePcbFrame::SaveItemEfface( BOARD_ITEM* aItem, int nbitems )
{
    if( aItem == NULL || nbitems == 0 )
        return NULL;

    if( g_UnDeleteStackPtr >= UNDELETE_STACK_SIZE )
    {
        // Delete last deleted item, and shift stack.
        delete g_UnDeleteStack[0];

        for( int ii = 0; ii < (g_UnDeleteStackPtr - 1); ii++ )
        {
            g_UnDeleteStack[ii] = g_UnDeleteStack[ii + 1];
        }

        g_UnDeleteStackPtr--;;
    }

    switch( aItem->Type() )
    {
    case TYPE_VIA:
    case TYPE_TRACK:
        {
            DLIST<TRACK>* container = (DLIST<TRACK>*) aItem->GetList();
            wxASSERT( container );

            if( nbitems == 1 )
            {
                container->Remove( (TRACK*) aItem );
                g_UnDeleteStack[g_UnDeleteStackPtr++] = aItem;
            }
            else
            {
                BOARD_ITEM_LIST* list = new BOARD_ITEM_LIST();
                g_UnDeleteStack[g_UnDeleteStackPtr++] = list;

                // copy the numerous tracks into the list, which is already on stack
                int i = 0;
                TRACK* next;
                for( TRACK* track = (TRACK*) aItem;  track && i<nbitems;  track = next, ++i )
                {
                    next = track->Next();
                    list->PushBack( container->Remove( track ) );
                }
            }
        }
        break;

#if !defined(GERBVIEW)
    case TYPE_MODULE:
        {
            MODULE* module = (MODULE*) aItem;
            m_Pcb->m_Modules.Remove( module );
            module->SetState( DELETED, ON );
            g_UnDeleteStack[g_UnDeleteStackPtr++] = module;
            build_liste_pads();
        }
        break;
#endif

    default:
        break;
    }

    // don't know why this is not simply return aItem?
    return g_UnDeleteStack[g_UnDeleteStackPtr - 1];
}
