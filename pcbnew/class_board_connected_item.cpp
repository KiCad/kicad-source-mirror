/*************************************************************************/
/* class_board_connected_item.cpp : BOARD_CONNECTED_ITEM class functions */
/*************************************************************************/

#include "fctsys.h"
#include "pcbnew.h"


BOARD_CONNECTED_ITEM::BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype )
{
    m_NetCode    = 0;
    m_Subnet     = 0;
    m_ZoneSubnet = 0;
}


BOARD_CONNECTED_ITEM::BOARD_CONNECTED_ITEM( const BOARD_CONNECTED_ITEM& src ) :
    BOARD_ITEM( src )
{
    m_Layer = src.m_Layer;
}


/**
 * Function GetNet
 * @return int - the net code.
 */
int BOARD_CONNECTED_ITEM::GetNet() const
{
    return m_NetCode;
}


void BOARD_CONNECTED_ITEM::SetNet( int aNetCode )
{
    m_NetCode = aNetCode;
}


/**
 * Function GetSubNet
 * @return int - the sub net code.
 */
int BOARD_CONNECTED_ITEM::GetSubNet() const
{
    return m_Subnet;
}


void BOARD_CONNECTED_ITEM::SetSubNet( int aSubNetCode )
{
    m_Subnet = aSubNetCode;
}


/**
 * Function GetZoneSubNet
 * @return int - the sub net code in zone connections.
 */
int BOARD_CONNECTED_ITEM::GetZoneSubNet() const
{
    return m_ZoneSubnet;
}


void BOARD_CONNECTED_ITEM::SetZoneSubNet( int aSubNetCode )
{
    m_ZoneSubnet = aSubNetCode;
}


int BOARD_CONNECTED_ITEM::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    NETCLASS*   myclass  = GetNetClass();

    wxASSERT( myclass );

    if( myclass )
    {
        // @todo : after GetNetClass() is reliably not returning NULL, remove the
        // tests for if( myclass ) and if( hisclass )

        if( aItem )
        {
            NETCLASS*   hisclass = aItem->GetNetClass();
            wxASSERT( hisclass );

            if( hisclass )
            {
                int hisClearance = hisclass->GetClearance();
                int myClearance  = myclass->GetClearance();
                return max( hisClearance, myClearance );
            }
        }

        return myclass->GetClearance();
    }

    return 0;
}


NETCLASS* BOARD_CONNECTED_ITEM::GetNetClass() const
{
    // It is important that this be implemented without any sequential searching.
    // Simple array lookups should be fine, performance-wise.

    BOARD*  board = GetBoard();
    wxASSERT( board );
    if( board )
    {
        NETINFO_ITEM* net = board->FindNet( GetNet() );
        wxASSERT( net );
        if( net )
        {
            NETCLASS* netclass = net->GetNetClass();
            wxASSERT( netclass );
            return netclass;
        }
    }

    return NULL;
}
