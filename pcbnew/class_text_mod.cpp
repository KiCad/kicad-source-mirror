/********************************************************/
/* class_module.cpp : TEXT_MODULE class implementation. */
/********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "kicad_string.h"
#include "pcbcommon.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

/*******************************************************************/
/* Class TEXTE_MODULE base class type of text elements in a module */
/*******************************************************************/

TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, int text_type ) :
    BOARD_ITEM( parent, TYPE_TEXTE_MODULE ), EDA_TextStruct()
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Type = text_type;         /* Reference */
    if( (m_Type != TEXT_is_REFERENCE) && (m_Type != TEXT_is_VALUE) )
        m_Type = TEXT_is_DIVERS;

    m_NoShow = false;
    m_Size.x = m_Size.y = 400;
    m_Thickness  = 120;   /* Set default dimension to a reasonable value. */

    SetLayer( SILKSCREEN_N_FRONT );
    if( Module && ( Module->Type() == TYPE_MODULE ) )
    {
        m_Pos = Module->m_Pos;

        int moduleLayer = Module->GetLayer();

        if( moduleLayer == LAYER_N_BACK )
            SetLayer( SILKSCREEN_N_BACK );
        else if( moduleLayer == LAYER_N_FRONT )
            SetLayer( SILKSCREEN_N_FRONT );
        else
            SetLayer( moduleLayer );

        if(  moduleLayer == SILKSCREEN_N_BACK
             || moduleLayer == ADHESIVE_N_BACK
             || moduleLayer == LAYER_N_BACK )
        {
            m_Mirror = true;
        }
    }
}


TEXTE_MODULE::~TEXTE_MODULE()
{
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool TEXTE_MODULE::Save( FILE* aFile ) const
{
    MODULE* parent = (MODULE*) GetParent();
    int     orient = m_Orient;

    // Due to the pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->m_Orient;

    int ret = fprintf( aFile, "T%d %d %d %d %d %d %d %c %c %d %c\"%s\"\n",
                      m_Type,
                      m_Pos0.x, m_Pos0.y,
                      m_Size.y, m_Size.x,
                      orient,
                      m_Thickness,
                      m_Mirror ? 'M' : 'N', m_NoShow ? 'I' : 'V',
                      GetLayer(),
                      m_Italic ? 'I' : 'N',
                      CONV_TO_UTF8( m_Text ) );

    return ret > 20;
}


/**
 * Function ReadLineDescr
 * Read description from a given line in "*.brd" format.
 * @param aLine The current line which contains the first line of description.
 * @param aLine The FILE to read next lines (currently not used).
 * @param LineNum a point to the line count (currently not used).
 * @return int - > 0 if success reading else 0.
 */
int TEXTE_MODULE::ReadDescr( char* aLine, FILE* aFile, int* aLineNum )
{
    int  success = true;
    int  type;
    int  layer;
    char BufCar1[128], BufCar2[128], BufCar3[128], BufLine[256];

    layer = SILKSCREEN_N_FRONT;
    BufCar1[0] = 0;
    BufCar2[0] = 0;
    BufCar3[0] = 0;
    if( sscanf( aLine + 1, "%d %d %d %d %d %d %d %s %s %d %s",
                &type,
                &m_Pos0.x, &m_Pos0.y,
                &m_Size.y, &m_Size.x,
                &m_Orient, &m_Thickness,
                BufCar1, BufCar2, &layer, BufCar3 ) >= 10 )
        success = true;

    if( (type != TEXT_is_REFERENCE) && (type != TEXT_is_VALUE) )
        type = TEXT_is_DIVERS;
    m_Type = type;

    // Due to the pcbnew history, .m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    m_Orient -= ( (MODULE*) m_Parent )->m_Orient;
    if( BufCar1[0] == 'M' )
        m_Mirror = true;
    else
        m_Mirror = false;
    if( BufCar2[0]  == 'I' )
        m_NoShow = true;
    else
        m_NoShow = false;

    if( BufCar3[0]  == 'I' )
        m_Italic = true;
    else
        m_Italic = false;

    // Test for a reasonable layer:
    if( layer < 0 )
        layer = 0;
    if( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    if( layer == LAYER_N_BACK )
        layer = SILKSCREEN_N_BACK;
    else if( layer == LAYER_N_FRONT )
        layer = SILKSCREEN_N_FRONT;

    SetLayer( layer );

    /* Calculate the true position. */
    SetDrawCoord();
    /* Read the "text" string. */
    ReadDelimitedText( BufLine, aLine, sizeof(BufLine) );
    m_Text = CONV_FROM_UTF8( BufLine );

    // Test for a reasonable size:
    if( m_Size.x < TEXTS_MIN_SIZE )
        m_Size.x = TEXTS_MIN_SIZE;
    if( m_Size.y < TEXTS_MIN_SIZE )
        m_Size.y = TEXTS_MIN_SIZE;

    // Set a reasonable width:
    if( m_Thickness < 1 )
        m_Thickness = 1;
    m_Thickness = Clamp_Text_PenSize( m_Thickness, m_Size );

    return success;
}


void TEXTE_MODULE::Copy( TEXTE_MODULE* source )
{
    if( source == NULL )
        return;

    m_Pos = source->m_Pos;
    SetLayer( source->GetLayer() );

    m_Mirror = source->m_Mirror;
    m_NoShow = source->m_NoShow;
    m_Type   = source->m_Type;
    m_Orient = source->m_Orient;
    m_Pos0   = source->m_Pos0;
    m_Size   = source->m_Size;
    m_Thickness  = source->m_Thickness;
    m_Italic = source->m_Italic;
    m_Bold   = source->m_Bold;
    m_Text   = source->m_Text;
}


int TEXTE_MODULE:: GetLength() const
{
    return m_Text.Len();
}


void TEXTE_MODULE:: SetWidth( int new_width )
{
    m_Thickness = new_width;
}


// Update draw coordinates
void TEXTE_MODULE:: SetDrawCoord()
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Pos = m_Pos0;

    if( Module == NULL )
        return;

    int angle = Module->m_Orient;
    NORMALIZE_ANGLE_POS( angle );

    RotatePoint( &m_Pos.x, &m_Pos.y, angle );
    m_Pos += Module->m_Pos;
}


// Update "local" coordinates (coordinates relatives to the footprint
//  anchor point)
void TEXTE_MODULE:: SetLocalCoord()
{
    MODULE* Module = (MODULE*) m_Parent;

    if( Module == NULL )
    {
        m_Pos0 = m_Pos;
        return;
    }

    m_Pos0 = m_Pos - Module->m_Pos;

    int angle = Module->m_Orient;
    NORMALIZE_ANGLE_POS( angle );

    RotatePoint( &m_Pos0.x, &m_Pos0.y, -angle );
}


/**
 * Function GetTextRect
 * @return an EDA_Rect which gives the position and size of the text area
 *         (for the footprint orientation)
 */
EDA_Rect TEXTE_MODULE::GetTextRect( void ) const
{
    EDA_Rect area;

    int      dx, dy;

    dx  = ( m_Size.x * GetLength() ) / 2;
    dx  = (dx * 10) / 9; /* letter size = 10/9 */
    dx += m_Thickness / 2;
    dy  = ( m_Size.y + m_Thickness ) / 2;

    wxPoint Org = m_Pos;    // This is the position of the center of the area
    Org.x -= dx;
    Org.y -= dy;
    area.SetOrigin( Org );
    area.SetHeight( 2 * dy );
    area.SetWidth( 2 * dx );
    area.Normalize();

    return area;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool TEXTE_MODULE::HitTest( const wxPoint& refPos )
{
    wxPoint  rel_pos;
    EDA_Rect area = GetTextRect();

    /* Rotate refPos to - angle
     * to test if refPos is within area (which is relative to an horizontal
     * text)
     */
    rel_pos = refPos;
    RotatePoint( &rel_pos, m_Pos, -GetDrawRotation() );

    if( area.Contains( rel_pos ) )
        return true;

    return false;
}


/**
 * Function GetBoundingBox
 * returns the bounding box of this Text (according to text and footprint
 * orientation)
 */
EDA_Rect TEXTE_MODULE::GetBoundingBox() const
{
    // Calculate area without text fields:
    EDA_Rect text_area;
    int      angle = GetDrawRotation();
    wxPoint  textstart, textend;

    text_area = GetTextRect();
    textstart = text_area.GetOrigin();
    textend   = text_area.GetEnd();
    RotatePoint( &textstart, m_Pos, angle );
    RotatePoint( &textend, m_Pos, angle );

    text_area.SetOrigin( textstart );
    text_area.SetEnd( textend );
    text_area.Normalize();
    return text_area;
}


/**
 * Function Draw
 * Draw the text according to the footprint pos and orient
 * @param panel = draw panel, Used to know the clip box
 * @param DC = Current Device Context
 * @param offset = draw offset (usually wxPoint(0,0)
 * @param draw_mode = GR_OR, GR_XOR..
 */
void TEXTE_MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode,
                         const wxPoint& offset )
{
    int                  width, color, orient;
    wxSize               size;
    wxPoint              pos;      // Center of text
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;
    MODULE*              Module = (MODULE*) m_Parent;   /* parent must *not* be null
                                                         *  (a module text without a footprint parent has no sense)
                                                         */


    if( panel == NULL )
        return;

    screen = (PCB_SCREEN*) panel->GetScreen();
    frame  = (WinEDA_BasePcbFrame*) panel->GetParent();

    pos.x = m_Pos.x - offset.x;
    pos.y = m_Pos.y - offset.y;

    size   = m_Size;
    orient = GetDrawRotation();
    width  = m_Thickness;

    if( ( frame->m_DisplayModText == FILAIRE )

#ifdef USE_WX_ZOOM
       || ( DC->LogicalToDeviceXRel( width ) < L_MIN_DESSIN ) )
#else
       || ( screen->Scale( width ) < L_MIN_DESSIN ) )
#endif
        width = 0;
    else if( frame->m_DisplayModText == SKETCH )
        width = -width;

    GRSetDrawMode( DC, draw_mode );

    BOARD * brd =  GetBoard( );
    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        color = brd->GetVisibleElementColor(ANCHOR_VISIBLE);

#ifdef USE_WX_ZOOM
        int anchor_size = DC->DeviceToLogicalXRel( 2 );
#else
        int anchor_size = screen->Unscale( 2 );
#endif
        GRLine( &panel->m_ClipBox, DC,
                pos.x - anchor_size, pos.y,
                pos.x + anchor_size, pos.y, 0, color );
        GRLine( &panel->m_ClipBox, DC,
                pos.x, pos.y - anchor_size,
                pos.x, pos.y + anchor_size, 0, color );
    }

    color = brd->GetLayerColor(Module->GetLayer());


    if( Module->GetLayer() == LAYER_N_BACK )
    {
        if( brd->IsElementVisible( MOD_TEXT_BK_VISIBLE ) == false )
            return;
        color = brd->GetVisibleElementColor(MOD_TEXT_BK_VISIBLE);
    }
    else if( Module->GetLayer() == LAYER_N_FRONT )
    {
        if( brd->IsElementVisible( MOD_TEXT_FR_VISIBLE ) == false )
            return;
        color = brd->GetVisibleElementColor(MOD_TEXT_FR_VISIBLE);
    }

    if( m_NoShow )
    {
        if( brd->IsElementVisible( MOD_TEXT_INVISIBLE ) == false )
            return;
        color = brd->GetVisibleElementColor(MOD_TEXT_INVISIBLE);
    }

    /* If the text is mirrored : negate size.x (mirror / Y axis) */
    if( m_Mirror )
        size.x = -size.x;

    DrawGraphicText( panel, DC, pos, (enum EDA_Colors) color, m_Text, orient,
                     size, m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}

void TEXTE_MODULE::DrawUmbilical( WinEDA_DrawPanel* aPanel,
                                  wxDC*             aDC,
                                  int               aDrawMode,
                                  const wxPoint&    aOffset )
{
    MODULE* parent = (MODULE*) GetParent();
    if( !parent )
        return;

    GRSetDrawMode( aDC, GR_XOR );
    GRLine( &aPanel->m_ClipBox, aDC,
            parent->GetPosition().x, parent->GetPosition().y,
            GetPosition().x + aOffset.x,
            GetPosition().y + aOffset.y,
            0, UMBILICAL_COLOR);
}

/* Return text rotation for drawings and plotting
 */
int TEXTE_MODULE::GetDrawRotation() const
{
    int     rotation;
    MODULE* Module = (MODULE*) m_Parent;

    rotation = m_Orient;

    if( Module )
        rotation += Module->m_Orient;

    NORMALIZE_ANGLE_POS( rotation );

    // For angle = 0 .. 180 deg
    while( rotation > 900 )
        rotation -= 1800;

    return rotation;
}


// see class_text_mod.h
void TEXTE_MODULE::DisplayInfo( WinEDA_DrawFrame* frame )
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )        // Happens in modedit, and for new texts
        return;

    wxString msg, Line;
    int      ii;

    static const wxString text_type_msg[3] =
    {
        _( "Ref." ), _( "Value" ), _( "Text" )
    };

    frame->ClearMsgPanel();

    Line = module->m_Reference->m_Text;
    frame->AppendMsgPanel( _( "Module" ), Line, DARKCYAN );

    Line = m_Text;
    frame->AppendMsgPanel( _( "Text" ), Line, BROWN );

    ii = m_Type;
    if( ii > 2 )
        ii = 2;
    frame->AppendMsgPanel( _( "Type" ), text_type_msg[ii], DARKGREEN );

    if( m_NoShow )
        msg = _( "No" );
    else
        msg = _( "Yes" );
    frame->AppendMsgPanel( _( "Display" ), msg, DARKGREEN );

    // Display text layer (use layer name if possible)
    BOARD* board = NULL;
    board = (BOARD*) module->GetParent();
    if( m_Layer < NB_LAYERS && board )
        msg = board->GetLayerName( m_Layer );
    else
        msg.Printf( wxT( "%d" ), m_Layer );
    frame->AppendMsgPanel( _( "Layer" ), msg, DARKGREEN );

    msg = _( " No" );
    if( m_Mirror )
        msg = _( " Yes" );
    frame->AppendMsgPanel( _( "Mirror" ), msg, DARKGREEN );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    frame->AppendMsgPanel( _( "Orient" ), msg, DARKGREEN );

    valeur_param( m_Thickness, msg );
    frame->AppendMsgPanel( _( "Thickness" ), msg, DARKGREEN );

    valeur_param( m_Size.x, msg );
    frame->AppendMsgPanel( _( "H Size" ), msg, RED );

    valeur_param( m_Size.y, msg );
    frame->AppendMsgPanel( _( "V Size" ), msg, RED );
}


// see class_text_mod.h
bool TEXTE_MODULE::IsOnLayer( int aLayer ) const
{
    if( m_Layer == aLayer )
        return true;

    /* test the parent, which is a MODULE */
    if( aLayer == GetParent()->GetLayer() )
        return true;

    if( aLayer == LAYER_N_BACK )
    {
        if( m_Layer==ADHESIVE_N_BACK || m_Layer==SILKSCREEN_N_BACK )
            return true;
    }
    else if( aLayer == LAYER_N_FRONT )
    {
        if( m_Layer==ADHESIVE_N_FRONT || m_Layer==SILKSCREEN_N_FRONT )
            return true;
    }

    return false;
}


/* see class_text_mod.h
 * bool TEXTE_MODULE::IsOnOneOfTheseLayers( int aLayerMask ) const
 * {
 *
 * }
 */


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void TEXTE_MODULE::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " string=\"" << m_Text.mb_str() << "\"/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
//                                 << ">\n";
}


#endif
