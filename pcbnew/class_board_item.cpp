/*****************************************/
/* class BOARD_ITEM: som basic functions */
/*****************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "pcbnew_id.h"
#include "collectors.h"

#include "bitmaps.h"


/********************************************************/
wxString BOARD_ITEM::ShowShape( Track_Shapes aShape )
/********************************************************/
{
    switch( aShape )
    {
    case S_SEGMENT:         return _( "Line" );
    case S_RECT:            return _( "Rect" );
    case S_ARC:             return _( "Arc" );
    case S_CIRCLE:          return _( "Circle" );
    case S_CURVE:           return _( "Bezier Curve" );

    // used in Gerbview:
    case S_ARC_RECT:        return wxT( "arc_rect" );
    case S_SPOT_OVALE:      return wxT( "spot_oval" );
    case S_SPOT_CIRCLE:     return wxT( "spot_circle" );
    case S_SPOT_RECT:       return wxT( "spot_rect" );
    case S_POLYGON:         return wxT( "polygon" );
    default:                return wxT( "??" );
    }
}


/********************************************************/
wxString BOARD_ITEM::MenuText( const BOARD* aPcb ) const
/********************************************************/

/** return a specific comment for "this". Used in pop up menus
 * @param aPcb = the parent board
 */
{
    wxString            text;
    wxString            msg;
    wxString            temp;
    NETINFO_ITEM* net;
    const BOARD_ITEM*   item = this;
    D_PAD *             pad;

    switch( item->Type() )
    {
    case TYPE_MODULE:
        text << _( "Footprint" ) << wxT( " " ) << ( (MODULE*) item )->GetReference();
        text << wxT( " (" ) << aPcb->GetLayerName( item->m_Layer ).Trim() << wxT( ")" );
        break;

    case TYPE_PAD:
        pad = (D_PAD *) this;
        text << _( "Pad" ) << wxT( " \"" ) << pad->ReturnStringPadName()
                << wxT( "\" (" );
        if ( (pad->m_Masque_Layer & ALL_CU_LAYERS) == ALL_CU_LAYERS )
            text << _("all copper layers");
        else if( (pad->m_Masque_Layer & LAYER_BACK) == LAYER_BACK )
            text << aPcb->GetLayerName( LAYER_N_BACK ).Trim();
        else if( (pad->m_Masque_Layer & LAYER_FRONT) == LAYER_FRONT )
            text << aPcb->GetLayerName( LAYER_N_FRONT );
        else text << _("???");
        text << _( ") of " ) << ( (MODULE*) GetParent() )->GetReference();
        break;

    case TYPE_DRAWSEGMENT:
        text << _( "Pcb Graphic" ) << wxT(": ")
            << ShowShape( (Track_Shapes) ((DRAWSEGMENT*)item)->m_Shape )
            << wxChar(' ') << _("Length:") << valeur_param( (int) ((DRAWSEGMENT*)item)->GetLength(), temp )
            << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim();
        break;

    case TYPE_TEXTE:
        text << _( "Pcb Text" ) << wxT( " " );;
        if( ( (TEXTE_PCB*) item )->m_Text.Len() < 12 )
            text << ( (TEXTE_PCB*) item )->m_Text;
        else
            text += ( (TEXTE_PCB*) item )->m_Text.Left( 10 ) + wxT( ".." );
        text << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim();
        break;

    case TYPE_TEXTE_MODULE:
        switch( ( (TEXTE_MODULE*) item )->m_Type )
        {
        case TEXT_is_REFERENCE:
            text << _( "Reference" ) << wxT( " " ) << ( (TEXTE_MODULE*) item )->m_Text;
            break;

        case TEXT_is_VALUE:
            text << _( "Value" ) << wxT( " " ) << ( (TEXTE_MODULE*) item )->m_Text << _( " of " )
                 << ( (MODULE*) GetParent() )->GetReference();
            break;

        default:    // wrap this one in quotes:
            text << _( "Text" ) << wxT( " \"" ) << ( (TEXTE_MODULE*) item )->m_Text <<
            wxT( "\"" ) << _( " of " )
                 << ( (MODULE*) GetParent() )->GetReference();
            break;
        }
        break;

    case TYPE_EDGE_MODULE:
        text << _( "Graphic" ) << wxT( " " );
        text << ShowShape( (Track_Shapes) ( (EDGE_MODULE*) item )->m_Shape );
        text << wxT( " (" ) << aPcb->GetLayerName( ((EDGE_MODULE*) item )->m_Layer ).Trim() << wxT( ")" );
        text << _( " of " )
             << ( (MODULE*) GetParent() )->GetReference();
        break;

    case TYPE_TRACK:
        // deleting tracks requires all the information we can get to
        // disambiguate all the choices under the cursor!
        text << _( "Track" ) << wxT( " " ) << ((TRACK*)item)->ShowWidth();
        net = aPcb->FindNet( ((TRACK*)item)->GetNet() );
        if( net )
        {
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
        }
        text << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim()
             << wxT("  ") << _("Net:") << ((TRACK*)item)->GetNet()
             << wxT("  ") << _("Length:") << valeur_param( (int) ((TRACK*)item)->GetLength(), temp );
        break;

    case TYPE_ZONE_CONTAINER:
        text = _( "Zone Outline" );
        {
            ZONE_CONTAINER* area = (ZONE_CONTAINER*) this;
            int ncont = area->m_Poly->GetContour(area->m_CornerSelection);
            if( ncont )
                text << wxT(" ") << _("(Cutout)");
        }
        text << wxT( " " );
        {
            wxString TimeStampText;
            TimeStampText.Printf( wxT( "(%8.8X)" ), item->m_TimeStamp );
            text << TimeStampText;
        }
        if ( !((ZONE_CONTAINER*) item)->IsOnCopperLayer() )
        {
            text << wxT( " [" ) << _("Not on copper layer") << wxT( "]" );
        }
        else if( ((ZONE_CONTAINER*) item)->GetNet() >= 0 )
        {
            net = aPcb->FindNet( ( (ZONE_CONTAINER*) item )->GetNet() );
            if( net )
            {
                text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
            }
        }
        else    // A netcode < 0 is an error flag (Netname not found or area not initialised)
        {
            text << wxT( " [" ) << ( (ZONE_CONTAINER*) item )->m_Netname << wxT( "]" );
            text << wxT(" <") << _("Not Found") << wxT(">");
        }
        text << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim();
        break;

    case TYPE_ZONE:
        text = _( "Zone" );
        text << wxT( " " );
        {
            wxString TimeStampText;
            TimeStampText.Printf( wxT( "(%8.8X)" ), item->m_TimeStamp );
            text << TimeStampText;
        }
        net = aPcb->FindNet( ( (SEGZONE*) item )->GetNet() );
        if( net )
        {
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
        }
        text << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim();
        break;

    case TYPE_VIA:
        {
            SEGVIA* via = (SEGVIA*) item;
            text << _( "Via" ) << wxT( " " ) << via->ShowWidth();

            int shape = via->Shape();
            if( shape == VIA_BLIND_BURIED )
                text << wxT(" ") << _( "Blind/Buried" );
            else if( shape == VIA_MICROVIA )
                text << wxT(" ") << _("Micro Via");
            // else say nothing about normal (through) vias

            net = aPcb->FindNet( via->GetNet() );
            if( net )
            {
                text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
            }
            text << wxChar(' ') << _("Net:") << via->GetNet();

            if( shape != VIA_THROUGH )
            {
                // say which layers, only two for now
                int topLayer;
                int botLayer;
                via->ReturnLayerPair( &topLayer, &botLayer );
                text << _( " on " ) << aPcb->GetLayerName( topLayer).Trim() << wxT(" <-> ")
                    << aPcb->GetLayerName( botLayer ).Trim();
            }
        }
        break;

    case TYPE_MARKER_PCB:
        text << _( "Marker" ) << wxT( " @(" ) << ((MARKER_PCB*)item)->GetPos().x
             << wxT(",") << ((MARKER_PCB*)item)->GetPos().y << wxT(")");
        break;

    case TYPE_COTATION:
        text << _( "Dimension" ) << wxT( " \"" ) << ( (COTATION*) item )->GetText() << wxT( "\"" );
        break;

    case TYPE_MIRE:
        valeur_param( ((MIREPCB*)item)->m_Size, msg );
        text << _( "Target" ) << _( " on " ) << aPcb->GetLayerName( item->GetLayer() ).Trim()
            << wxT( " " ) << _( "size" ) << wxT( " " ) << msg
            ;
        break;

    default:
        text << item->GetClass() << wxT( " Unexpected item type: BUG!!" );
        break;
    }

    return text;
}


/*****************************************/
const char** BOARD_ITEM::MenuIcon() const
/*****************************************/

/** return a specific icon pointer (an xpm icon)  for "this". Used in pop up menus
 * @return an icon pointer (can be NULL)
 */
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

    case TYPE_COTATION:
        xpm = add_cotation_xpm;
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

