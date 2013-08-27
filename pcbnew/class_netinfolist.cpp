/**
 * @file class_netinfolist.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_netinfo.h>


// Constructor and destructor
NETINFO_LIST::NETINFO_LIST( BOARD* aParent )
{
    m_Parent = aParent;
}


NETINFO_LIST::~NETINFO_LIST()
{
    clear();
}


void NETINFO_LIST::clear()
{
    for( unsigned ii = 0; ii < GetNetCount(); ii++ )
        delete m_NetBuffer[ii];

    m_NetBuffer.clear();
    m_PadsFullList.clear();
}


/**
 * Function Append
 * adds \a aNewElement to the end of the list.
 */
void NETINFO_LIST::AppendNet( NETINFO_ITEM* aNewElement )
{
    m_NetBuffer.push_back( aNewElement );

    // D(Show();)
}


/* sort function, to sort pad list by netnames
 * this is a case sensitive sort.
 * DO NOT change it because NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
 * when search a net by its net name does a binary search
 * and expects to have a nets list sorted by an alphabetic case sensitive sort
 */

static bool padlistSortByNetnames( const D_PAD* a, const D_PAD* b )
{
    return ( a->GetNetname().Cmp( b->GetNetname() ) ) < 0;
}


/**
 *  Compute and update the net_codes for PADS et and equipots (.m_NetCode member)
 *  net_codes are >= 1 (net_code = 0 means not connected)
 *  Update the net buffer
 *  Must be called after editing pads (netname, or deleting) or after read a netlist
 *  set to 1 flag NET_CODE_OK  of m_Pcb->m_Status_Pcb;
 *  m_Pcb->m_NbNodes and m_Pcb->m_NbNets are updated
 * Be aware NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
 * when search a net by its net name does a binary search
 * and expects to have a nets list sorted by an alphabetic case sensitive sort
 * So do not change Build_Pads_Full_List() which build a sorted list of pads
 */
void NETINFO_LIST::buildListOfNets()
{
    D_PAD*          pad;
    int             nodes_count = 0;
    NETINFO_ITEM*   net_item;

    clear();        // Remove all nets info and free memory

    // Create and add the "unconnected net", always existing,
    // used to handle pads and tracks that are not member of a "real" net
    net_item = new NETINFO_ITEM( m_Parent );
    AppendNet( net_item );

    // Build the PAD list, sorted by net
    buildPadsFullList();

    // Build netnames list, and create a netcode for each netname
    D_PAD* last_pad = NULL;
    int    netcode = 0;

    for( unsigned ii = 0; ii < m_PadsFullList.size(); ii++ )
    {
        pad = m_PadsFullList[ii];

        if( pad->GetNetname().IsEmpty() ) // pad not connected
        {
            pad->SetNet( 0 );
            continue;
        }

        /* if the current netname was already found: add pad to the current net_item ,
         *  else create a new net_code and a new net_item
         */
        if( last_pad == NULL || ( pad->GetNetname() != last_pad->GetNetname() ) )
        {
            netcode++;
            net_item = new NETINFO_ITEM( m_Parent, pad->GetNetname(), netcode );
            AppendNet( net_item );
        }

        pad->SetNet( netcode );
        net_item->m_PadInNetList.push_back( pad );

        nodes_count++;

        last_pad = pad;
    }

    m_Parent->SetNodeCount( nodes_count );

    m_Parent->SynchronizeNetsAndNetClasses( );

    m_Parent->m_Status_Pcb |= NET_CODES_OK;

    m_Parent->SetAreasNetCodesFromNetNames();

    // D( Show(); )
}

#if defined(DEBUG)
void NETINFO_LIST::Show() const
{
    for( unsigned i=0; i < m_NetBuffer.size();  ++i )
    {
        printf( "[%d]: netcode:%d  netname:<%s>\n",
            i, m_NetBuffer[i]->GetNet(),
            TO_UTF8( m_NetBuffer[i]->GetNetname() ) );
    }
}
#endif

void NETINFO_LIST::buildPadsFullList()
{
    /*
     * initialize:
     *   m_Pads (list of pads)
     * set m_Status_Pcb = LISTE_PAD_OK;
     * also clear m_Pcb->m_FullRatsnest that could have bad data
     *   (m_Pcb->m_FullRatsnest uses pointer to pads)
     * Be aware NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
     * when search a net by its net name does a binary search
     * and expects to have a nets list sorted by an alphabetic case sensitive sort
     * So do not change the sort function used here
     */

    if( m_Parent->m_Status_Pcb & LISTE_PAD_OK )
        return;

    // empty the old list
    m_PadsFullList.clear();
    m_Parent->m_FullRatsnest.clear();

    // Clear variables used in ratsnest computation
    for( MODULE* module = m_Parent->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
        {
            m_PadsFullList.push_back( pad );

            pad->SetSubRatsnest( 0 );
            pad->SetParent( module );
        }
    }

    // Sort pad list per net
    sort( m_PadsFullList.begin(), m_PadsFullList.end(), padlistSortByNetnames );

    m_Parent->m_Status_Pcb = LISTE_PAD_OK;
}
