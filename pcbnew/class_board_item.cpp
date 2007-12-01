/*****************************************/
/* class BOARD_ITEM: som basic functions */
/*****************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "id.h"
#include "collectors.h"

#include "bitmaps.h"
#include "add_cotation.xpm"
#include "Add_Mires.xpm"
#include "Add_Zone.xpm"


/********************************************************/
wxString BOARD_ITEM::MenuText( const BOARD* aPcb ) const
/********************************************************/

/** return a specific comment for "this". Used in pop up menus
 * @param aPcb = the parent board
 */
{
    wxString          text, msg;
    const BOARD_ITEM* item = this;
    EQUIPOT*          net;
	D_PAD * pad;

    switch( item->Type() )
    {
    case PCB_EQUIPOT_STRUCT_TYPE:
        text << _( "Net" ) << ( (EQUIPOT*) item )->m_Netname << wxT( " " ) <<
        ( (EQUIPOT*) item )->GetNet();
        break;

    case TYPEMODULE:
        text << _( "Footprint" ) << wxT( " " ) << ( (MODULE*) item )->GetReference();
        text << wxT( " (" ) << ReturnPcbLayerName( item->m_Layer ).Trim() << wxT( ")" );
        break;

    case TYPEPAD:
		pad = (D_PAD *) this;
        text << _( "Pad" ) << wxT( " \"" ) << pad->ReturnStringPadName()
				<< wxT( "\" (" );
		if ( (pad->m_Masque_Layer & ALL_CU_LAYERS) == ALL_CU_LAYERS )
			text << _("all copper layers");
		else if( (pad->m_Masque_Layer & CUIVRE_LAYER) == CUIVRE_LAYER )
			text << _("copper layer");
		else if( (pad->m_Masque_Layer & CMP_LAYER) == CMP_LAYER )
			text << _("cmp layer");
		else text << _("???");
		text << _( ") of " ) << ( (MODULE*) GetParent() )->GetReference();
        break;

    case TYPEDRAWSEGMENT:
        text << _( "Pcb Graphic" ) << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim();  // @todo: extend text
        break;

    case TYPETEXTE:
        text << _( "Pcb Text" ) << wxT( " " );;
        if( ( (TEXTE_PCB*) item )->m_Text.Len() < 12 )
            text << ( (TEXTE_PCB*) item )->m_Text;
        else
            text += ( (TEXTE_PCB*) item )->m_Text.Left( 10 ) + wxT( ".." );
        text << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim();
        break;

    case TYPETEXTEMODULE:
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

    case TYPEEDGEMODULE:
        text << _( "Graphic" ) << wxT( " " );
        const wxChar* cp;

        switch( ( (EDGE_MODULE*) item )->m_Shape )
        {
        case S_SEGMENT:
            cp = _( "Line" );             break;

        case S_RECT:
            cp = _( "Rect" );             break;

        case S_ARC:
            cp = _( "Arc" );              break;

        case S_CIRCLE:
            cp = _( "Circle" );           break;

            /* used in Gerbview: */
        case S_ARC_RECT:
            cp = wxT( "arc_rect" );       break;

        case S_SPOT_OVALE:
            cp = wxT( "spot_oval" );      break;

        case S_SPOT_CIRCLE:
            cp = wxT( "spot_circle" );    break;

        case S_SPOT_RECT:
            cp = wxT( "spot_rect" );      break;

        case S_POLYGON:
            cp = wxT( "polygon" );        break;

        default:
            cp = wxT( "??EDGE??" );       break;
        }

        text << *cp << _( " of " )
             << ( (MODULE*) GetParent() )->GetReference();
        break;

    case TYPETRACK:
        // deleting tracks requires all the information we can get to 
        // disambiguate all the crap under the cursor!
        {
            wxString txt;
            
            text << _( "Track" ) << wxT( " " ) << ((TRACK*)item)->ShowWidth();
            net = aPcb->FindNet( ((TRACK*)item)->GetNet() );
            if( net )
            {
                text << wxT( " [" ) << net->m_Netname << wxT( "]" );
            }
            text << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim() 
                 << wxT("  ") << _("Net:") << ((TRACK*)item)->GetNet()
                 << wxT("  ") << _("Length:") << valeur_param( (int) ((TRACK*)item)->GetLength(), txt );
        }
        break;

    case TYPEZONE:
        text << _( "Zone" ) << wxT( " " );
        {
            wxString TimeStampText;
            TimeStampText.Printf( wxT( "(%8.8X)" ), item->m_TimeStamp );
            text << TimeStampText;
        }
        net = aPcb->FindNet( ( (SEGZONE*) item )->GetNet() );
        if( net )
        {
            text << wxT( " [" ) << net->m_Netname << wxT( "]" );
        }
        text << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim();
        break;

    case TYPEVIA:
        {
            SEGVIA* via = (SEGVIA*) item;
            text << _( "Via" ) << wxT( " " ) << via->ShowWidth();
            
            int shape = via->Shape(); 
            if( shape == VIA_BURIED )
                text << wxT(" ") << _( "Blind" );
            else if( shape == VIA_BLIND )
                text << wxT(" ") << _("Buried");
            // else say nothing about normal (through) vias
            
            net = aPcb->FindNet( via->GetNet() );
            if( net )
            {
                text << wxT( " [" ) << net->m_Netname << wxT( "]" );
            }
            
            if( shape != VIA_THROUGH )
            {
                // say which layers, only two for now
                int topLayer;
                int botLayer;
                via->ReturnLayerPair( &topLayer, &botLayer );
                text << _( " on " ) << ReturnPcbLayerName( topLayer).Trim() << wxT(" <-> ") 
                    << ReturnPcbLayerName( botLayer ).Trim();
            }
        }
        break;

    case TYPEMARKER:
        text << _( "Marker" ) << wxT( " @(" ) << ((MARKER*)item)->GetPos().x 
             << wxT(",") << ((MARKER*)item)->GetPos().y << wxT(")");
        break;

    case TYPECOTATION:
        text << _( "Dimension" ) << wxT( " \"" ) << ( (COTATION*) item )->GetText() << wxT( "\"" );
        break;

    case TYPEMIRE:
		valeur_param( ((MIREPCB*)item)->m_Size, msg );
        text << _( "Target" ) << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim()
            << wxT( " " ) << _( "size" ) << wxT( " " ) << msg
            ;
        break;

    case TYPEEDGEZONE:
        text << _( "Edge Zone" ) << _( " on " ) << ReturnPcbLayerName( item->GetLayer() ).Trim();  // @todo: extend text
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
    case PCB_EQUIPOT_STRUCT_TYPE:
        xpm = general_ratsnet_xpm;
        break;

    case TYPEMODULE:
        xpm = module_xpm;
        break;

    case TYPEPAD:
        xpm = pad_xpm;
        break;

    case TYPEDRAWSEGMENT:
        xpm = add_dashed_line_xpm;
        break;

    case TYPETEXTE:
        xpm = add_text_xpm;
        break;

    case TYPETEXTEMODULE:
        xpm = footprint_text_xpm;
        break;

    case TYPEEDGEMODULE:
        xpm = show_mod_edge_xpm;
        break;

    case TYPETRACK:
        xpm = showtrack_xpm;
        break;

    case TYPEZONE:
        xpm = add_zone_xpm;
        break;

    case TYPEVIA:
        xpm = pad_sketch_xpm;
        break;

    case TYPEMARKER:
        xpm = pad_xpm;              // @todo: create and use marker xpm
        break;

    case TYPECOTATION:
        xpm = add_cotation_xpm;
        break;

    case TYPEMIRE:
        xpm = add_mires_xpm;
        break;

    case TYPEEDGEZONE:
        xpm = show_mod_edge_xpm;    // @todo: pcb edge xpm
        break;

    default:
        xpm = 0;
        break;
    }

    return (const char**) xpm;
}

