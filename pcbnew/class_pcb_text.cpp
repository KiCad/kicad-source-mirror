/**
 * @file class_pcb_text.cpp
 * @brief Class TEXTE_PCB texts on copper or technical layers implementation
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "gr_basic.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "kicad_string.h"
#include "trigo.h"
#include "pcbcommon.h"
#include "colors_selection.h"
#include "richio.h"
#include "class_drawpanel.h"
#include "macros.h"

#include "class_board.h"
#include "class_pcb_text.h"

#include "protos.h"


TEXTE_PCB::TEXTE_PCB( BOARD_ITEM* parent ) :
    BOARD_ITEM( parent, PCB_TEXT_T ),
    EDA_TEXT()
{
    m_MultilineAllowed = true;
}


TEXTE_PCB:: ~TEXTE_PCB()
{
}


void TEXTE_PCB::Copy( TEXTE_PCB* source )
{
    m_Parent    = source->m_Parent;
    Pback       = Pnext = NULL;
    m_Mirror    = source->m_Mirror;
    m_Size      = source->m_Size;
    m_Orient    = source->m_Orient;
    m_Pos       = source->m_Pos;
    m_Layer     = source->m_Layer;
    m_Thickness     = source->m_Thickness;
    m_Attributs = source->m_Attributs;
    m_Italic    = source->m_Italic;
    m_Bold      = source->m_Bold;
    m_HJustify  = source->m_HJustify;
    m_VJustify  = source->m_VJustify;
    m_MultilineAllowed = m_MultilineAllowed;

    m_Text = source->m_Text;
}


/*
 * Function Draw
 * Like tracks, texts are drawn in filled or sketch mode, never in line mode
 * because the line mode does not keep the actual size of the text
 * and the actual size is very important, especially for copper texts
 */
void TEXTE_PCB::Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                      int DrawMode, const wxPoint& offset )
{
    BOARD* brd = GetBoard();

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    int color = brd->GetLayerColor( m_Layer );

    GRTraceMode fillmode = FILLED;
    if( DisplayOpt.DisplayDrawItems == SKETCH )
        fillmode = SKETCH;

    int anchor_color = UNSPECIFIED_COLOR;
    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
        anchor_color = brd->GetVisibleElementColor( ANCHOR_VISIBLE );

    EDA_TEXT::Draw( panel, DC, offset, (EDA_Colors) color,
                    DrawMode, fillmode, (EDA_Colors) anchor_color );
}


// see class_pcb_text.h
void TEXTE_PCB::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString    msg;

    BOARD*      board;
    BOARD_ITEM* parent = (BOARD_ITEM*) m_Parent;

    wxASSERT( parent );

    if( parent->Type() == PCB_DIMENSION_T )
        board = (BOARD*) parent->GetParent();
    else
        board = (BOARD*) parent;
    wxASSERT( board );

    frame->ClearMsgPanel();

    if( m_Parent && m_Parent->Type() == PCB_DIMENSION_T )
        frame->AppendMsgPanel( _( "DIMENSION" ), m_Text, DARKGREEN );
    else
        frame->AppendMsgPanel( _( "PCB Text" ), m_Text, DARKGREEN );

    frame->AppendMsgPanel( _( "Layer" ),
                         board->GetLayerName( m_Layer ), BLUE );

    if( !m_Mirror )
        frame->AppendMsgPanel( _( "Mirror" ), _( "No" ), DARKGREEN );
    else
        frame->AppendMsgPanel( _( "Mirror" ), _( "Yes" ), DARKGREEN );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    frame->AppendMsgPanel( _( "Orientation" ), msg, DARKGREEN );

    valeur_param( m_Thickness, msg );
    frame->AppendMsgPanel( _( "Thickness" ), msg, MAGENTA );

    valeur_param( m_Size.x, msg );
    frame->AppendMsgPanel( _( "Size X" ), msg, RED );

    valeur_param( m_Size.y, msg );
    frame->AppendMsgPanel( _( "Size Y" ), msg, RED );
}


/**
 * Function Rotate
 * Rotate this object.
 * @param aRotCentre - the rotation point.
 * @param aAngle - the rotation angle in 0.1 degree.
 */
void TEXTE_PCB::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
    m_Orient += aAngle;
    NORMALIZE_ANGLE_360( m_Orient );
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param aCentre - the rotation point.
 */
void TEXTE_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
//    NEGATE( m_Orient );   not needed: m_Mirror handles this
    if( ( GetLayer() == LAYER_N_BACK ) || ( GetLayer() == LAYER_N_FRONT ) )
    {
        m_Mirror = not m_Mirror;      /* inverse mirror */
    }
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


wxString TEXTE_PCB::GetSelectMenuText() const
{
    wxString text, shorttxt;

    if( m_Text.Len() < 12 )
        shorttxt << m_Text;
    else
        shorttxt += m_Text.Left( 10 ) + wxT( ".." );

    text.Printf( _( "Pcb Text %s on %s"),
                 GetChars ( shorttxt ), GetChars( GetLayerName() ) );

    return text;
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void TEXTE_PCB::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " string=\"" << m_Text.mb_str() << "\"/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
//                                 << ">\n";
}


#endif
