/******************************************/
/* class BOARD_ITEM: some basic functions */
/******************************************/

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"


wxString BOARD_ITEM::ShowShape( Track_Shapes aShape )
{
    switch( aShape )
    {
    case S_SEGMENT:         return _( "Line" );
    case S_RECT:            return _( "Rect" );
    case S_ARC:             return _( "Arc" );
    case S_CIRCLE:          return _( "Circle" );
    case S_CURVE:           return _( "Bezier Curve" );
    case S_POLYGON:         return _( "Polygon" );
    default:                return wxT( "??" );
    }
}


void BOARD_ITEM::UnLink()
{
    DLIST<BOARD_ITEM>* list = (DLIST<BOARD_ITEM>*) GetList();
    wxASSERT( list );

    if( list )
        list->Remove( this );
}


BOARD* BOARD_ITEM::GetBoard() const
{
    if( Type() == TYPE_PCB )
        return (BOARD*) this;

    BOARD_ITEM* parent = GetParent();

    if( parent )
        return parent->GetBoard();

    return NULL;
}


wxString BOARD_ITEM::GetLayerName() const
{
    wxString layerName;
    BOARD* board = GetBoard();

    if( board != NULL )
        return board->GetLayerName( m_Layer ).Trim();

    wxFAIL_MSG( wxT( "No board found for board item type " ) + GetClass() );
    layerName = _( "** undefined layer **" );

    return layerName;
}
