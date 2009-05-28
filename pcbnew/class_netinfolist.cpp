/***********************/
/**** class_netinfolist.cpp  ****/
/***********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "autorout.h"


// Constructor and destructor
NETINFO_LIST::NETINFO_LIST( BOARD* aParent )
{
    m_Parent = aParent;
}


NETINFO_LIST::~NETINFO_LIST()
{
    DeleteData();
}


/** Function GetItem
 * @param aNetcode = netcode to identify a given NETINFO_ITEM
 * @return a NETINFO_ITEM pointer to the selected NETINFO_ITEM by its netcode, or NULL if not found
 */
NETINFO_ITEM* NETINFO_LIST::GetItem( int aNetcode )
{
    if( aNetcode < 0 || ( aNetcode > (int) ( GetCount() - 1 ) ) )
        return NULL;
    return m_NetBuffer[aNetcode];
}


/** Function DeleteData
 * delete the list of nets (and free memory)
 */
void NETINFO_LIST::DeleteData()
{
    for( unsigned ii = 0; ii < GetCount(); ii++ )
        delete m_NetBuffer[ii];

    m_NetBuffer.clear();
}


/**
 * Function Append
 * adds \a aNewElement to the end of the list.
 */
void NETINFO_LIST::Append( NETINFO_ITEM* aNewElement )
{
    m_NetBuffer.push_back( aNewElement );
}


/* sort function, to sort pad list by netnames
 */

static bool PadlistSortByNetnames( const D_PAD* a, const D_PAD* b )
{
    return ( a->GetNetname().Cmp( b->GetNetname() ) ) < 0;
}


/*****************************************************/
void NETINFO_LIST::BuildListOfNets()
/*****************************************************/

/**
 *  Compute and update the net_codes for PADS et and equipots (.m_NetCode member)
 *  net_codes are >= 1 (net_code = 0 means not connected)
 *  Update the equipotents buffer
 *  Must be called after editing pads (netname, or deleting) or after read a netlist
 *  set to 1 flag NET_CODE_OK  of m_Pcb->m_Status_Pcb;
 *  m_Pcb->m_NbNodes and m_Pcb->m_NbNets are updated
 */
{
    D_PAD*        pad;
    int nodes_count = 0;
    NETINFO_ITEM* net_item;

    DeleteData();        // Remove all nets info and free memory

    // Create and add the "unconnected net"
    net_item = new NETINFO_ITEM( m_Parent );
    Append( net_item );

    /* Build the PAD list, sorted by net */
    m_Parent->Build_Pads_Full_List();

    int    netcode = 0;
    /* Build netnames list, and create a netcode for each netname */
    D_PAD* last_pad = NULL;
    for( unsigned ii = 0; ii < m_Parent->m_Pads.size(); ii++ )
    {
        pad = m_Parent->m_Pads[ii];
        if( pad->GetNetname().IsEmpty() ) // pad not connected
        {
            pad->SetNet( 0 );
            continue;
        }

        /* if the current netname was already found: add pad to the current net_item ,
         *  else create a new net_code and a new net_item
         */
        if( last_pad == NULL || ( pad->GetNetname() != last_pad->GetNetname() ) )   // create a new net_code
        {
            netcode++;
            net_item = new NETINFO_ITEM( m_Parent );
            net_item->SetNet( netcode );
            net_item->SetNetname( pad->GetNetname() );
            Append( net_item );
        }

        pad->SetNet( netcode );
        net_item->m_ListPad.push_back( pad );
        nodes_count ++;
        last_pad = pad;
    }

    m_Parent->m_NbNodes = nodes_count;

    m_Parent->m_Status_Pcb |= NET_CODES_OK;

    m_Parent->SetAreasNetCodesFromNetNames();
}


/**********************************/
void BOARD::Build_Pads_Full_List()
/**********************************/

/** Function Build_Pads_Full_List
 *  Create the pad list, sorted by net names
 * initialise:
 *   m_Pads (list of pads)
 * set m_Status_Pcb = LISTE_PAD_OK;
 * also clear m_Pcb->m_FullRatsnest that could have bad data
 *   (m_Pcb->m_FullRatsnest uses pointer to pads)
 */
{
    if( m_Status_Pcb & LISTE_PAD_OK )
        return;

    // empty the old list
    m_Pads.clear();
    m_FullRatsnest.clear();

    /* Clear variables used in rastnest computation */
    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
        {
            m_Pads.push_back( pad );

            pad->SetSubRatsnest( 0 );
            pad->SetParent( module );
        }
    }

    // Sort pad list per net
    sort( m_Pads.begin(), m_Pads.end(), PadlistSortByNetnames );

    m_Status_Pcb = LISTE_PAD_OK;
}
