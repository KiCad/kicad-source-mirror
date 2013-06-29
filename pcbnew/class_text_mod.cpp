/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <msgpanel.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>


TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, TEXT_TYPE text_type ) :
    BOARD_ITEM( parent, PCB_MODULE_TEXT_T ),
    EDA_TEXT()
{
    MODULE* module = (MODULE*) m_Parent;

    m_Type = text_type;

    m_NoShow = false;

    // Set text tickness to a default value
    m_Thickness = Millimeter2iu( 0.15 );

    SetLayer( SILKSCREEN_N_FRONT );

    if( module && ( module->Type() == PCB_MODULE_T ) )
    {
        m_Pos = module->GetPosition();

        if( IsBackLayer( module->GetLayer() ) )
        {
            SetLayer( SILKSCREEN_N_BACK );
            m_Mirror = true;
        }
        else
        {
            SetLayer( SILKSCREEN_N_FRONT );
            m_Mirror = false;
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
    MODULE* module = (MODULE*) m_Parent;

    m_Pos = m_Pos0;

    if( module == NULL )
        return;

    double angle = module->GetOrientation();

    RotatePoint( &m_Pos.x, &m_Pos.y, angle );
    m_Pos += module->GetPosition();
}


// Update "local" coordinates (coordinates relatives to the footprint
//  anchor point)
void TEXTE_MODULE::SetLocalCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
    {
        m_Pos0 = m_Pos;
        return;
    }

    m_Pos0 = m_Pos - module->GetPosition();

    double angle = module->GetOrientation();

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
    double   angle = GetDrawRotation();
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
void TEXTE_MODULE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE draw_mode,
                         const wxPoint& offset )
{
    MODULE* module = (MODULE*) m_Parent;

    /* parent must *not* be NULL (a module text without a footprint
       parent has no sense) */
    wxASSERT( module );

    if( panel == NULL )
        return;

    BOARD* brd = GetBoard( );
    EDA_COLOR_T color;
    // Determine the element color or suppress it element if hidden
    switch( module->GetLayer() )
    {
    case LAYER_N_BACK:
        if( !brd->IsElementVisible( MOD_TEXT_BK_VISIBLE ) )
            return;
        color = brd->GetVisibleElementColor( MOD_TEXT_BK_VISIBLE );
        break;

    case LAYER_N_FRONT:
        if( !brd->IsElementVisible( MOD_TEXT_FR_VISIBLE ) )
            return;
        color = brd->GetVisibleElementColor( MOD_TEXT_FR_VISIBLE );
        break;

    default:
        color = brd->GetLayerColor( module->GetLayer() );
    }

    // 'Ghost' the element if forced show
    if( m_NoShow )
    {
        if( !brd->IsElementVisible( MOD_TEXT_INVISIBLE ) )
            return;
        color = brd->GetVisibleElementColor( MOD_TEXT_INVISIBLE );
    }

    // Draw mode compensation for the width
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) panel->GetParent();
    int width = m_Thickness;
    if( ( frame->m_DisplayModText == LINE )
        || ( DC->LogicalToDeviceXRel( width ) <= MIN_DRAW_WIDTH ) )
        width = 0;
    else if( frame->m_DisplayModText == SKETCH )
        width = -width;

    GRSetDrawMode( DC, draw_mode );
    wxPoint pos( m_Pos.x - offset.x,
                 m_Pos.y - offset.y);

    // Draw the text anchor point
    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        EDA_COLOR_T anchor_color = brd->GetVisibleElementColor(ANCHOR_VISIBLE);
        GRDrawAnchor( panel->GetClipBox(), DC, pos.x, pos.y,
                      DIM_ANCRE_TEXTE, anchor_color );
    }

    // Draw the text proper, with the right attributes
    wxSize size   = m_Size;
    double orient = GetDrawRotation();

    // If the text is mirrored : negate size.x (mirror / Y axis)
    if( m_Mirror )
        size.x = -size.x;

    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    DrawGraphicText( clipbox, DC, pos, color, m_Text, orient,
                     size, m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}

/* Draws a line from the TEXTE_MODULE origin to parent MODULE origin.
*/
void TEXTE_MODULE::DrawUmbilical( EDA_DRAW_PANEL* aPanel,
                                  wxDC*           aDC,
                                  GR_DRAWMODE     aDrawMode,
                                  const wxPoint&  aOffset )
{
    MODULE* parent = (MODULE*) GetParent();

    if( !parent )
        return;

    GRSetDrawMode( aDC, GR_XOR );
    GRLine( aPanel->GetClipBox(), aDC,
            parent->GetPosition(), GetTextPosition() + aOffset,
            0, UMBILICAL_COLOR);
}

/* Return text rotation for drawings and plotting
 */
double TEXTE_MODULE::GetDrawRotation() const
{
    MODULE* module = (MODULE*) m_Parent;
    double  rotation = m_Orient;

    if( module )
        rotation += module->GetOrientation();

    NORMALIZE_ANGLE_POS( rotation );

    // For angle = 0 .. 180 deg
    while( rotation > 900 )
        rotation -= 1800;

    return rotation;
}


// see class_text_mod.h
void TEXTE_MODULE::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )        // Happens in modedit, and for new texts
        return;

    wxString msg, Line;

    static const wxString text_type_msg[3] =
    {
        _( "Ref." ), _( "Value" ), _( "Text" )
    };

    Line = module->GetReference();
    aList.push_back( MSG_PANEL_ITEM( _( "Module" ), Line, DARKCYAN ) );

    Line = m_Text;
    aList.push_back( MSG_PANEL_ITEM( _( "Text" ), Line, BROWN ) );

    wxASSERT( m_Type >= TEXT_is_REFERENCE && m_Type <= TEXT_is_DIVERS );
    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), text_type_msg[m_Type], DARKGREEN ) );

    if( m_NoShow )
        msg = _( "No" );
    else
        msg = _( "Yes" );

    aList.push_back( MSG_PANEL_ITEM( _( "Display" ), msg, DARKGREEN ) );

    // Display text layer
    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), DARKGREEN ) );

    if( m_Mirror )
        msg = _( " Yes" );
    else
        msg = _( " No" );

    aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), msg, DARKGREEN ) );

    msg.Printf( wxT( "%.1f" ), m_Orient / 10.0 );
    aList.push_back( MSG_PANEL_ITEM( _( "Orient" ), msg, DARKGREEN ) );

    msg = ::CoordinateToString( m_Thickness );
    aList.push_back( MSG_PANEL_ITEM( _( "Thickness" ), msg, DARKGREEN ) );

    msg = ::CoordinateToString( m_Size.x );
    aList.push_back( MSG_PANEL_ITEM( _( "H Size" ), msg, RED ) );

    msg = ::CoordinateToString( m_Size.y );
    aList.push_back( MSG_PANEL_ITEM( _( "V Size" ), msg, RED ) );
}


wxString TEXTE_MODULE::GetSelectMenuText() const
{
    wxString text;

    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        text.Printf( _( "Reference %s" ), GetChars( m_Text ) );
        break;

    case TEXT_is_VALUE:
        text.Printf( _( "Value %s of %s" ), GetChars( m_Text ),
                     GetChars( ( (MODULE*) GetParent() )->GetReference() ) );
        break;

    default:    // wrap this one in quotes:
        text.Printf( _( "Text \"%s\" on %s of %s" ), GetChars( m_Text ),
                     GetChars( GetLayerName() ),
                     GetChars( ( (MODULE*) GetParent() )->GetReference() ) );
        break;
    }

    return text;
}


EDA_ITEM* TEXTE_MODULE::Clone() const
{
    return new TEXTE_MODULE( *this );
}

