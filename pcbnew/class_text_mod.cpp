/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_module.cpp
 * @brief TEXT_MODULE class implementation.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wxstruct.h>
#include <trigo.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <kicad_string.h>
#include <pcbcommon.h>
#include <colors_selection.h>
#include <richio.h>
#include <macros.h>
#include <wxBasePcbFrame.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>


TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, int text_type ) :
    BOARD_ITEM( parent, PCB_MODULE_TEXT_T ),
    EDA_TEXT()
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Type = text_type;         /* Reference */

    if( (m_Type != TEXT_is_REFERENCE) && (m_Type != TEXT_is_VALUE) )
        m_Type = TEXT_is_DIVERS;

    m_NoShow = false;
    m_Size.x = m_Size.y = 400;
    m_Thickness  = 120;   /* Set default dimension to a reasonable value. */

    SetLayer( SILKSCREEN_N_FRONT );

    if( Module && ( Module->Type() == PCB_MODULE_T ) )
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


int TEXTE_MODULE::GetLength() const
{
    return m_Text.Len();
}

// Update draw coordinates
void TEXTE_MODULE::SetDrawCoord()
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
void TEXTE_MODULE::SetLocalCoord()
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
 * @return an EDA_RECT which gives the position and size of the text area
 *         (for the footprint orientation)
 */
EDA_RECT TEXTE_MODULE::GetTextRect( void ) const
{
    EDA_RECT area;

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


bool TEXTE_MODULE::HitTest( const wxPoint& aPosition )
{
    wxPoint  rel_pos;
    EDA_RECT area = GetTextRect();

    /* Rotate refPos to - angle
     * to test if refPos is within area (which is relative to an horizontal
     * text)
     */
    rel_pos = aPosition;
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
EDA_RECT TEXTE_MODULE::GetBoundingBox() const
{
    // Calculate area without text fields:
    EDA_RECT text_area;
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
void TEXTE_MODULE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode, const wxPoint& offset )
{
    int             width, color, orient;
    wxSize          size;
    wxPoint         pos;      // Center of text
    PCB_BASE_FRAME* frame;
    MODULE*         Module = (MODULE*) m_Parent;   /* parent must *not* be null
                                                    *  (a module text without a footprint
                                                    * parent has no sense) */


    if( panel == NULL )
        return;

    frame  = (PCB_BASE_FRAME*) panel->GetParent();

    pos.x = m_Pos.x - offset.x;
    pos.y = m_Pos.y - offset.y;

    size   = m_Size;
    orient = GetDrawRotation();
    width  = m_Thickness;

    if( ( frame->m_DisplayModText == LINE )
        || ( DC->LogicalToDeviceXRel( width ) < MIN_DRAW_WIDTH ) )
        width = 0;
    else if( frame->m_DisplayModText == SKETCH )
        width = -width;

    GRSetDrawMode( DC, draw_mode );

    BOARD * brd =  GetBoard( );
    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        color = brd->GetVisibleElementColor(ANCHOR_VISIBLE);

        int anchor_size = DC->DeviceToLogicalXRel( 2 );

        GRLine( panel->GetClipBox(), DC,
                pos.x - anchor_size, pos.y,
                pos.x + anchor_size, pos.y, 0, color );
        GRLine( panel->GetClipBox(), DC,
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

    DrawGraphicText( panel, DC, pos, (enum EDA_COLOR_T) color, m_Text, orient,
                     size, m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}

/* Draws a line from the TEXTE_MODULE origin to parent MODULE origin.
*/
void TEXTE_MODULE::DrawUmbilical( EDA_DRAW_PANEL* aPanel,
                                  wxDC*           aDC,
                                  int             aDrawMode,
                                  const wxPoint&  aOffset )
{
    MODULE* parent = (MODULE*) GetParent();

    if( !parent )
        return;

    GRSetDrawMode( aDC, GR_XOR );
    GRLine( aPanel->GetClipBox(), aDC,
            parent->GetPosition(), GetPosition() + aOffset,
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
void TEXTE_MODULE::DisplayInfo( EDA_DRAW_FRAME* frame )
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


wxString TEXTE_MODULE::GetSelectMenuText() const
{
    wxString text;

    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        text << _( "Reference" ) << wxT( " " ) << m_Text;
        break;

    case TEXT_is_VALUE:
        text << _( "Value" ) << wxT( " " ) << m_Text << _( " of " )
             << ( (MODULE*) GetParent() )->GetReference();
        break;

    default:    // wrap this one in quotes:
        text << _( "Text" ) << wxT( " \"" ) << m_Text << wxT( "\"" ) << _( " of " )
             << ( (MODULE*) GetParent() )->GetReference();
        break;
    }

    return text;
}


EDA_ITEM* TEXTE_MODULE::Clone() const
{
    return new TEXTE_MODULE( *this );
}


void TEXTE_MODULE::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    MODULE* parent = (MODULE*) GetParent();
    double  orient = GetOrientation();

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->GetOrientation();

    aFormatter->Print( aNestLevel, "(module_text %d (at %s %0.1f)%s\n", m_Type,
                       FMT_IU( m_Pos0 ).c_str(), orient, (m_NoShow) ? "hide" : "" );

    EDA_TEXT::Format( aFormatter, aNestLevel+1, aControlBits );

    aFormatter->Print( aNestLevel, ")\n" );
}


#if defined(DEBUG)

void TEXTE_MODULE::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " string=\"" << m_Text.mb_str() << "\"/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
//                                 << ">\n";
}

#endif
