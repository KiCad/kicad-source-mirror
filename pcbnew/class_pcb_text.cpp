/*********************************************************/
/* class TEXTE_PCB : texts on copper or technical layers */
/*********************************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "gr_basic.h"
#include "base_struct.h"
#include "common.h"
#include "drawtxt.h"
#include "kicad_string.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"
#include "trigo.h"
#include "protos.h"
#include "richio.h"


/*******************/
/* class TEXTE_PCB */
/*******************/

TEXTE_PCB::TEXTE_PCB( BOARD_ITEM* parent ) :
    BOARD_ITEM( parent, TYPE_TEXTE ),
    EDA_TextStruct()
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


/**
 * Function ReadTextePcbDescr
 * Read a text description from pcb file.
 *
 * For a single line text:
 *
 * $TEXTPCB
 * Te "Text example"
 * Po 66750 53450 600 800 150 0
 * From 24 1 0 Italic
 * $EndTEXTPCB
 *
 * For a multi line text
 *
 * $TEXTPCB
 * Te "Text example"
 * Nl "Line 2"
 * Po 66750 53450 600 800 150 0
 * From 24 1 0 Italic
 * $EndTEXTPCB
 * Nl "line nn" is a line added to the current text
 */
int TEXTE_PCB::ReadTextePcbDescr( LINE_READER* aReader )
{
    char* line;
    char  text[1024];
    char  style[256];

    while( aReader->ReadLine() )
    {
        line = aReader->Line();
        if( strnicmp( line, "$EndTEXTPCB", 11 ) == 0 )
            return 0;
        if( strncmp( line, "Te", 2 ) == 0 ) /* Text line (first line for multi line texts */
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            m_Text = CONV_FROM_UTF8( text );
            continue;
        }
        if( strncmp( line, "nl", 2 ) == 0 ) /* next line of the current text */
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            m_Text.Append( '\n' );
            m_Text += CONV_FROM_UTF8( text );
            continue;
        }
        if( strncmp( line, "Po", 2 ) == 0 )
        {
            sscanf( line + 2, " %d %d %d %d %d %d",
                    &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y,
                    &m_Thickness, &m_Orient );

            // Ensure the text has minimal size to see this text on screen:
            if( m_Size.x < 5 )
                m_Size.x = 5;
            if( m_Size.y < 5 )
                m_Size.y = 5;
            continue;
        }
        if( strncmp( line, "De", 2 ) == 0 )
        {
            style[0] = 0;
            int normal_display = 1;
            sscanf( line + 2, " %d %d %lX %s\n", &m_Layer, &normal_display,
                    &m_TimeStamp, style );

            m_Mirror = normal_display ? false : true;

            if( m_Layer < FIRST_COPPER_LAYER )
                m_Layer = FIRST_COPPER_LAYER;
            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;

            if( strnicmp( style, "Italic", 6 ) == 0 )
                m_Italic = 1;
            else
                m_Italic = 0;
            continue;
        }
    }

     // Set a reasonable width:
    if( m_Thickness < 1 )
        m_Thickness = 1;
    m_Thickness = Clamp_Text_PenSize( m_Thickness, m_Size );

    return 1;
}


bool TEXTE_PCB::Save( FILE* aFile ) const
{
    if( m_Text.IsEmpty() )
        return true;

    if( fprintf( aFile, "$TEXTPCB\n" ) != sizeof("$TEXTPCB\n") - 1 )
        return false;

    const char* style = m_Italic ? "Italic" : "Normal";

    wxArrayString* list = wxStringSplit( m_Text, '\n' );

    for( unsigned ii = 0; ii < list->Count(); ii++ )
    {
        wxString txt  = list->Item( ii );

        if ( ii == 0 )
            fprintf( aFile, "Te \"%s\"\n", EscapedUTF8( txt ).c_str() );
        else
            fprintf( aFile, "nl \"%s\"\n", CONV_TO_UTF8( txt ) );
    }

    delete list;

    fprintf( aFile, "Po %d %d %d %d %d %d\n",
             m_Pos.x, m_Pos.y, m_Size.x, m_Size.y, m_Thickness, m_Orient );

    fprintf( aFile, "De %d %d %lX %s\n", m_Layer,
             m_Mirror ? 0 : 1,
             m_TimeStamp, style );

    if( fprintf( aFile, "$EndTEXTPCB\n" ) != sizeof("$EndTEXTPCB\n") - 1 )
        return false;

    return true;
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
    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    int color = brd->GetLayerColor(m_Layer);

    GRTraceMode fillmode = FILLED;
    if ( DisplayOpt.DisplayDrawItems == SKETCH)
        fillmode = SKETCH;

    int anchor_color = UNSPECIFIED_COLOR;
    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
        anchor_color = brd->GetVisibleElementColor(ANCHOR_VISIBLE);

    EDA_TextStruct::Draw( panel, DC, offset, (EDA_Colors) color,
                          DrawMode, fillmode, (EDA_Colors) anchor_color );
}


// see class_pcb_text.h
void TEXTE_PCB::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString    msg;

    BOARD*      board;
    BOARD_ITEM* parent = (BOARD_ITEM*) m_Parent;

    wxASSERT( parent );

    if( parent->Type() == TYPE_DIMENSION )
        board = (BOARD*) parent->GetParent();
    else
        board = (BOARD*) parent;
    wxASSERT( board );

    frame->ClearMsgPanel();

    if( m_Parent && m_Parent->Type() == TYPE_DIMENSION )
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
    frame->AppendMsgPanel( _( "Orient" ), msg, DARKGREEN );

    valeur_param( m_Thickness, msg );
    frame->AppendMsgPanel( _( "Thickness" ), msg, MAGENTA );

    valeur_param( m_Size.x, msg );
    frame->AppendMsgPanel( _( "H Size" ), msg, RED );

    valeur_param( m_Size.y, msg );
    frame->AppendMsgPanel( _( "V Size" ), msg, RED );
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
    while( m_Orient >= 3600 )
        m_Orient -= 3600;
    while( m_Orient < -3600 )
        m_Orient += 3600;
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param aCentre - the rotation point.
 */
void TEXTE_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    NEGATE( m_Orient );
    if( ( GetLayer() == LAYER_N_BACK ) || ( GetLayer() == LAYER_N_FRONT ) )
    {
        m_Mirror = not m_Mirror;      /* inverse mirror */
    }
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
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
