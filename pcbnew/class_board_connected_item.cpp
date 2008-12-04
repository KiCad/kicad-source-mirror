/*************************************************************************/
/* class_board_connected_item.cpp : BOARD_CONNECTED_ITEM class functions */
/*************************************************************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif

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
