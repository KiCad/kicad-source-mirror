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

    // DO NOT use wxASSERT, because GetClearance is called inside an OnPaint event
    // and a call to wxASSERT can crash the application.
    if( myclass )
    {
        // @todo : after GetNetClass() is reliably not returning NULL, remove the
        // tests for if( myclass ) and if( hisclass )

        if( aItem )
        {
            NETCLASS*   hisclass = aItem->GetNetClass();
            if( hisclass )
            {
                int hisClearance = hisclass->GetClearance();
                int myClearance  = myclass->GetClearance();
                return max( hisClearance, myClearance );
            }
            else
            {
#ifdef __WXDEBUG__
                wxLogWarning(wxT("BOARD_CONNECTED_ITEM::GetClearance(): NULL hisclass") );
#endif
            }
        }

        return myclass->GetClearance();
    }
    else
    {
#ifdef __WXDEBUG__
        wxLogWarning(wxT("BOARD_CONNECTED_ITEM::GetClearance(): NULL netclass") );
#endif
    }

    return 0;
}


/** return a pointer to the netclass of the zone
 * if the net is not found (can happen when a netlist is reread,
 * and the net name is not existant, return the default net class
 * So should not return a null pointer
 */
NETCLASS* BOARD_CONNECTED_ITEM::GetNetClass() const
{
    // It is important that this be implemented without any sequential searching.
    // Simple array lookups should be fine, performance-wise.
    BOARD*  board = GetBoard();
    // DO NOT use wxASSERT, because GetNetClass is called inside an OnPaint event
    // and a call to wxASSERT can crash the application.
    if( board == NULL )     // Should not occurs
    {
#ifdef __WXDEBUG__
        wxLogWarning(wxT("BOARD_CONNECTED_ITEM::GetNetClass(): NULL board, type %d"), Type() );
#endif
        return NULL;
    }

    NETCLASS* netclass = NULL;
    NETINFO_ITEM* net = board->FindNet( GetNet() );
    if( net )
    {
        netclass = net->GetNetClass();
#ifdef __WXDEBUG__
        if( netclass == NULL )
            wxLogWarning(wxT("BOARD_CONNECTED_ITEM::GetNetClass(): NULL netclass") );
#endif
    }
    else
    {
#ifdef __WXDEBUG__
        wxLogWarning(wxT("BOARD_CONNECTED_ITEM::GetNetClass(): NULL net") );
#endif
    }

    if( netclass )
        return netclass;
    else
        return board->m_NetClasses.GetDefault();
}

/** function GetNetClassName
 * @return the Net Class name of this item
 */
wxString BOARD_CONNECTED_ITEM::GetNetClassName( ) const
{
    wxString name;
    NETCLASS*   myclass  = GetNetClass();

    if( myclass )
        name = myclass->GetName();
    else
    {
        BOARD*  board = GetBoard();
        name = board->m_NetClasses.GetDefault()->GetName();
    }

    return name;
}
