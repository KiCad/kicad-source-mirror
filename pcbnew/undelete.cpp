/********************************************************/
/* Effacements : Routines de sauvegarde et d'effacement */
/********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Routines externes : */

/* Routines Locales */

/********************************************/
void WinEDA_PcbFrame::UnDeleteItem( wxDC* DC )
/********************************************/

/* Restitution d'un element (MODULE ou TRACK ) Efface
 */
{
    BOARD_ITEM*     item;
    BOARD_ITEM*     next;
    int             net_code;

    if( !g_UnDeleteStackPtr )
        return;

    g_UnDeleteStackPtr--;
    item = g_UnDeleteStack[g_UnDeleteStackPtr];
    if( item == NULL )
        return;                     // Ne devrait pas se produire

    switch( item->Type() )
    {
    case TYPEVIA:
    case TYPETRACK:
        for( ; item; item = next )
        {
            next = item->Next();
            item->SetState( DELETED, OFF );     /* Effacement du bit DELETED */
            ((TRACK*) item)->Draw( DrawPanel, DC, GR_OR );
        }

        item = g_UnDeleteStack[g_UnDeleteStackPtr];
        net_code = ((TRACK*) item)->GetNet();

        m_Pcb->Add( item );
        g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;

        test_1_net_connexion( DC, net_code );
        m_Pcb->Display_Infos( this );
        break;

    case TYPEMODULE:
        /* Erase general rastnest if needed */
        if( g_Show_Ratsnest )
            DrawGeneralRatsnest( DC );

        /* Reinsertion du module dans la liste chainee des modules,
         *  en debut de chaine */
        m_Pcb->Add( item );

        g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;

        ((MODULE*) item)->Draw( DrawPanel, DC, GR_OR );

        item->SetState( DELETED, OFF );     /* Creal DELETED flag */
        item->m_Flags   = 0;
        m_Pcb->m_Status_Pcb = 0;
        build_liste_pads();
        ReCompile_Ratsnest_After_Changes( DC );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_PcbFrame::UnDeleteItem(): unexpected Struct type" ) );
        break;
    }
}


/**************************************************************/
/* void * SaveItemEfface(int type, void * PtItem, int nbitems) */
/**************************************************************/

/* Sauvegarde d'un element aux fins de restitution par Undelete
 *  Supporte actuellement : Module et segments de piste
 */
BOARD_ITEM* WinEDA_PcbFrame::SaveItemEfface( BOARD_ITEM* PtItem, int nbitems )
{
    BOARD_ITEM* NextS, * PtStruct = PtItem;
    int             ii;

    if( (PtItem == NULL) || (nbitems == 0) )
        return NULL;

    if( g_UnDeleteStackPtr >= UNDELETE_STACK_SIZE )
    {
        /* Delete last deleted item, and shift stack. */
        g_UnDeleteStack[0]->DeleteStructList();
        for( ii = 0; ii < (g_UnDeleteStackPtr - 1); ii++ )
        {
            g_UnDeleteStack[ii] = g_UnDeleteStack[ii + 1];
        }

        g_UnDeleteStackPtr--;;
    }

    switch( PtStruct->Type() )
    {
    case TYPEVIA:
    case TYPETRACK:
    {
        BOARD_ITEM* Back = NULL;
        g_UnDeleteStack[g_UnDeleteStackPtr++] = PtStruct;

        for( ; nbitems > 0; nbitems--, PtStruct = NextS )
        {
            NextS = PtStruct->Next();
            ( (TRACK*) PtStruct )->UnLink();

            PtStruct->SetState( DELETED, ON );
            if( nbitems <= 1 )
                NextS = NULL;                       /* fin de chaine */

            PtStruct->SetNext( NextS );
            PtStruct->SetBack( Back );
            Back = PtStruct;
            if( NextS == NULL )
                break;
        }
    }
        break;

    case TYPEMODULE:
    {
        MODULE* Module = (MODULE*) PtItem;
        Module->UnLink();
        Module->SetState( DELETED, ON );
        g_UnDeleteStack[g_UnDeleteStackPtr++] = Module;
        build_liste_pads();
    }
        break;

    default:
        break;
    }

    return g_UnDeleteStack[g_UnDeleteStackPtr - 1];
}
