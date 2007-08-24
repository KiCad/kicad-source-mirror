/***********************************************************************/
/* Methodes de base de gestion des classes des elements de schematique */
/***********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"


/*******************************************************************/
DrawBusEntryStruct::DrawBusEntryStruct( const wxPoint& pos, int shape, int id ) :
    EDA_BaseStruct( DRAW_BUSENTRY_STRUCT_TYPE )
/*******************************************************************/
{
    m_Pos    = pos;
    m_Size.x = 100;
    m_Size.y = 100;
    m_Layer  = LAYER_WIRE;
    m_Width  = 0;

    if( id == BUS_TO_BUS )
    {
        m_Layer = LAYER_BUS;
        m_Width = 1;
    }

    if( shape == '/' )
        m_Size.y = -100;
}


/*************************************/
wxPoint DrawBusEntryStruct::m_End( void )
/*************************************/

// retourne la coord de fin du raccord
{
    return wxPoint( m_Pos.x + m_Size.x, m_Pos.y + m_Size.y );
}


/***************************************************/
DrawBusEntryStruct* DrawBusEntryStruct::GenCopy( void )
/***************************************************/
{
    DrawBusEntryStruct* newitem = new DrawBusEntryStruct( m_Pos, 0, 0 );

    newitem->m_Layer = m_Layer;
    newitem->m_Width = m_Width;
    newitem->m_Size  = m_Size;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/****************************/
/* class DrawJunctionStruct */
/***************************/

/************************************************************/
DrawJunctionStruct::DrawJunctionStruct( const wxPoint& pos ) :
    EDA_BaseStruct( DRAW_JUNCTION_STRUCT_TYPE )
/************************************************************/
{
    m_Pos   = pos;
    m_Layer = LAYER_JUNCTION;
}


DrawJunctionStruct* DrawJunctionStruct::GenCopy( void )
{
    DrawJunctionStruct* newitem = new DrawJunctionStruct( m_Pos );

    newitem->m_Layer = m_Layer;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/*****************************/
/* class DrawNoConnectStruct */
/*****************************/

DrawNoConnectStruct::DrawNoConnectStruct( const wxPoint& pos ) :
    EDA_BaseStruct( DRAW_NOCONNECT_STRUCT_TYPE )
{
    m_Pos = pos;
}


DrawNoConnectStruct* DrawNoConnectStruct::GenCopy( void )
{
    DrawNoConnectStruct* newitem = new DrawNoConnectStruct( m_Pos );

    newitem->m_Flags = m_Flags;

    return newitem;
}


/**************************/
/* class DrawMarkerStruct */
/**************************/

DrawMarkerStruct::DrawMarkerStruct( const wxPoint& pos, const wxString& text ) :
    EDA_BaseStruct( DRAW_MARKER_STRUCT_TYPE )
{
    m_Pos       = pos;              /* XY coordinates of marker. */
    m_Type      = MARQ_UNSPEC;
    m_MarkFlags = 0;                // complements d'information
    m_Comment   = text;
}


DrawMarkerStruct::~DrawMarkerStruct( void )
{
}


DrawMarkerStruct* DrawMarkerStruct::GenCopy( void )
{
    DrawMarkerStruct* newitem = new DrawMarkerStruct( m_Pos, m_Comment );

    newitem->m_Type      = m_Type;
    newitem->m_MarkFlags = m_MarkFlags;

    return newitem;
}


wxString DrawMarkerStruct::GetComment( void )
{
    return m_Comment;
}


/***************************/
/* Class EDA_DrawLineStruct */
/***************************/

EDA_DrawLineStruct::EDA_DrawLineStruct( const wxPoint& pos, int layer ) :
    EDA_BaseStruct( NULL, DRAW_SEGMENT_STRUCT_TYPE )
{
    m_Start = pos;
    m_End   = pos;
    m_StartIsDangling = m_EndIsDangling = FALSE;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;     /* Mettre ds Notes */
        m_Width = GR_NORM_WIDTH;
        break;

    case LAYER_WIRE:
        m_Layer = LAYER_WIRE;
        m_Width = GR_NORM_WIDTH;
        break;

    case LAYER_BUS:
        m_Layer = LAYER_BUS;
        m_Width = GR_THICK_WIDTH;
        break;
    }
}


/***************************************************/
EDA_DrawLineStruct* EDA_DrawLineStruct::GenCopy( void )
/***************************************************/
{
    EDA_DrawLineStruct* newitem = new EDA_DrawLineStruct( m_Start, m_Layer );

    newitem->m_End = m_End;

    return newitem;
}


/************************************************************/
bool EDA_DrawLineStruct::IsOneEndPointAt( const wxPoint& pos )
/************************************************************/

/* Return TRUE if the start or the end point is in position pos
 */
{
    if( (pos.x == m_Start.x) && (pos.y == m_Start.y) )
        return TRUE;
    if( (pos.x == m_End.x) && (pos.y == m_End.y) )
        return TRUE;
    return FALSE;
}


/****************************/
/* Class DrawPolylineStruct */
/****************************/

/***********************************************************/
DrawPolylineStruct::DrawPolylineStruct( int layer ) :
    EDA_BaseStruct( DRAW_POLYLINE_STRUCT_TYPE )
/***********************************************************/
{
    m_NumOfPoints = 0;          /* Number of XY pairs in Points array. */
    m_Points = NULL;            /* XY pairs that forms the polyline. */
    m_Width  = GR_NORM_WIDTH;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
    case LAYER_NOTES:
        m_Layer = layer;
        break;

    case LAYER_BUS:
        m_Layer = layer;
        m_Width = GR_THICK_WIDTH;
        break;
    }
}


/********************************************/
DrawPolylineStruct::~DrawPolylineStruct( void )
/*********************************************/
{
    if( m_Points )
        free( m_Points );
}


/*****************************************************/
DrawPolylineStruct* DrawPolylineStruct::GenCopy( void )
/*****************************************************/
{
    int memsize;

    DrawPolylineStruct* newitem =
        new DrawPolylineStruct( m_Layer );

    memsize = sizeof(int) * 2 * m_NumOfPoints;
    newitem->m_NumOfPoints = m_NumOfPoints;
    newitem->m_Points = (int*) MyZMalloc( memsize );
    memcpy( newitem->m_Points, m_Points, memsize );

    return newitem;
}
