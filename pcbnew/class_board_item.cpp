/******************************************/
/* class BOARD_ITEM: some basic functions */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "pcbnew_id.h"
#include "collectors.h"

#include "bitmaps.h"


wxString BOARD_ITEM::ShowShape( Track_Shapes aShape )
{
    switch( aShape )
    {
    case S_SEGMENT:         return _( "Line" );
    case S_RECT:            return _( "Rect" );
    case S_ARC:             return _( "Arc" );
    case S_CIRCLE:          return _( "Circle" );
    case S_CURVE:           return _( "Bezier Curve" );
    case S_POLYGON:         return wxT( "polygon" );
    default:                return wxT( "??" );
    }
}



/** return a specific icon pointer (an xpm icon)  for "this". Used in pop up menus
 * @return an icon pointer (can be NULL)
 */
const char** BOARD_ITEM::MenuIcon() const
{
    const char**            xpm;
    const BOARD_ITEM* item = this;

    switch( item->Type() )
    {
    case TYPE_MODULE:
        xpm = module_xpm;
        break;

    case TYPE_PAD:
        xpm = pad_xpm;
        break;

    case TYPE_DRAWSEGMENT:
        xpm = add_dashed_line_xpm;
        break;

    case TYPE_TEXTE:
        xpm = add_text_xpm;
        break;

    case TYPE_TEXTE_MODULE:
        xpm = footprint_text_xpm;
        break;

    case TYPE_EDGE_MODULE:
        xpm = show_mod_edge_xpm;
        break;

    case TYPE_TRACK:
        xpm = showtrack_xpm;
        break;

    case TYPE_ZONE_CONTAINER:
    case TYPE_ZONE:
        xpm = add_zone_xpm;
        break;

    case TYPE_VIA:
        xpm = via_sketch_xpm;
        break;

    case TYPE_MARKER_PCB:
        xpm = pad_xpm;              // @todo: create and use marker xpm
        break;

    case TYPE_DIMENSION:
        xpm = add_dimension_xpm;
        break;

    case TYPE_MIRE:
        xpm = add_mires_xpm;
        break;

    default:
        xpm = 0;
        break;
    }

    return (const char**) xpm;
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
