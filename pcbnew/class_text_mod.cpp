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
    MODULE* module = static_cast<MODULE*>( m_Parent );

    m_Type = text_type;

    m_NoShow = false;

    // Set text tickness to a default value
    m_Thickness = Millimeter2iu( 0.15 );

    SetLayer( F_SilkS );

    // Set position and layer if there is already a parent module
    if( module && ( module->Type() == PCB_MODULE_T ) )
    {
        m_Pos = module->GetPosition();

        if( IsBackLayer( module->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            m_Mirror = true;
        }
        else
        {
            SetLayer( F_SilkS );
            m_Mirror = false;
        }
    }
}


TEXTE_MODULE::~TEXTE_MODULE()
{
}


void TEXTE_MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
    m_Orient += aAngle;
    NORMALIZE_ANGLE_360( m_Orient );
}


void TEXTE_MODULE::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    SetLayer( FlipLayer( GetLayer() ) );
    m_Mirror = !m_Mirror;
}


void TEXTE_MODULE::FlipWithModule( int aOffset )
{
    m_Pos.y = aOffset - (m_Pos.y - aOffset);
    NEGATE_AND_NORMALIZE_ANGLE_POS( m_Orient );
    SetLayer( FlipLayer( GetLayer() ) );
    m_Mirror = IsBackLayer( GetLayer() );
}


void TEXTE_MODULE::MirrorWithModule( int aOffset )
{
    wxPoint tmp = GetTextPosition();
    tmp.x = aOffset - (tmp.x - aOffset);
    SetTextPosition( tmp );
    tmp.y = GetPos0().y;
    SetPos0( tmp );
    NEGATE_AND_NORMALIZE_ANGLE_POS( m_Orient );
}


void TEXTE_MODULE::RotateWithModule( const wxPoint& aOffset, double aAngle )
{
    wxPoint pos = GetTextPosition();
    RotatePoint( &pos, aOffset, aAngle );
    SetTextPosition( pos );
    SetPos0( GetTextPosition() );
    SetOrientation( GetOrientation() + aAngle );
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


void TEXTE_MODULE::SetDrawCoord()
{
    const MODULE* module = static_cast<const MODULE*>( m_Parent );

    m_Pos = m_Pos0;

    if( module  )
    {
        double angle = module->GetOrientation();

        RotatePoint( &m_Pos.x, &m_Pos.y, angle );
        m_Pos += module->GetPosition();
    }
}


void TEXTE_MODULE::SetLocalCoord()
{
    const MODULE* module = static_cast<const MODULE*>( m_Parent );

    if( module )
    {
        m_Pos0 = m_Pos - module->GetPosition();
        double angle = module->GetOrientation();
        RotatePoint( &m_Pos0.x, &m_Pos0.y, -angle );
    }
    else
    {
        m_Pos0 = m_Pos;
    }
}


bool TEXTE_MODULE::HitTest( const wxPoint& aPosition ) const
{
    wxPoint  rel_pos;
    EDA_RECT area = GetTextBox( -1, -1 );

    /* Rotate refPos to - angle to test if refPos is within area (which
     * is relative to an horizontal text)
     */
    rel_pos = aPosition;
    RotatePoint( &rel_pos, m_Pos, -GetDrawRotation() );

    if( area.Contains( rel_pos ) )
        return true;

    return false;
}


/*
 * Function GetBoundingBox (virtual)
 * returns the bounding box of this Text (according to text and footprint
 * orientation)
 */
const EDA_RECT TEXTE_MODULE::GetBoundingBox() const
{
    double   angle = GetDrawRotation();
    EDA_RECT text_area = GetTextBox( -1, -1 );

    if( angle )
        text_area = text_area.GetBoundingBoxRotated( m_Pos, m_Orient );

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
    if( panel == NULL )
        return;

    MODULE* module = static_cast<MODULE*>( m_Parent );

    /* parent must *not* be NULL (a module text without a footprint
       parent has no sense) */
    wxASSERT( module );

    BOARD* brd = GetBoard( );

    /* Reference and values takes the color from the corresponding Visibles
       other texts take the color of the layer they are on */
    EDA_COLOR_T color;

    /* For reference and value suppress the element if the layer it is
     * on is on a disabled side, user text also has standard layer
     * hiding.
     * If the whole module side is disabled this isn't even called */
    LAYER_ID text_layer = GetLayer();
    if( (IsFrontLayer( text_layer ) && !brd->IsElementVisible( MOD_TEXT_FR_VISIBLE )) || 
        (IsBackLayer( text_layer ) && !brd->IsElementVisible( MOD_TEXT_BK_VISIBLE )) )
        return;

    switch( GetType() )
    {
    case TEXT_is_REFERENCE:
    case TEXT_is_VALUE:
        if( IsFrontLayer( text_layer ) )
            color = brd->GetVisibleElementColor( MOD_TEXT_FR_VISIBLE );
        else
            color = brd->GetVisibleElementColor( MOD_TEXT_BK_VISIBLE );
        break;

    default:    // Otherwise the compiler is not sure about initializing color
    case TEXT_is_DIVERS:
        if( brd->IsLayerVisible( m_Layer ) )
            color = brd->GetLayerColor( GetLayer() );
        else
            return;
    }

    // 'Ghost' the element if forced show
    if( m_NoShow )
    {
        if( !brd->IsElementVisible( MOD_TEXT_INVISIBLE ) )
            return;
        color = brd->GetVisibleElementColor( MOD_TEXT_INVISIBLE );
    }

    // Draw mode compensation for the width
    PCB_BASE_FRAME* frame = static_cast<PCB_BASE_FRAME*>( panel->GetParent() );
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

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}

/* Draws a line from the TEXTE_MODULE origin to parent MODULE origin.
*/
void TEXTE_MODULE::DrawUmbilical( EDA_DRAW_PANEL* aPanel,
                                  wxDC*           aDC,
                                  GR_DRAWMODE     aDrawMode,
                                  const wxPoint&  aOffset )
{
    MODULE* parent = static_cast<MODULE*>( GetParent() );

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
    const wxChar *reference = GetChars( static_cast<MODULE*>( GetParent() )->GetReference() );

    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        text.Printf( _( "Reference %s" ), reference );
        break;

    case TEXT_is_VALUE:
        text.Printf( _( "Value %s of %s" ), GetChars( m_Text ), reference );
        break;

    default:    // wrap this one in quotes:
        text.Printf( _( "Text \"%s\" on %s of %s" ), GetChars( m_Text ),
                     GetChars( GetLayerName() ), reference );
        break;
    }

    return text;
}


EDA_ITEM* TEXTE_MODULE::Clone() const
{
    return new TEXTE_MODULE( *this );
}


const BOX2I TEXTE_MODULE::ViewBBox() const
{
    double   angle = GetDrawRotation();
    EDA_RECT text_area = GetTextBox( -1, -1 );

    if( angle )
        text_area = text_area.GetBoundingBoxRotated( m_Pos, angle );

    return BOX2I( text_area.GetPosition(), text_area.GetSize() );
}


void TEXTE_MODULE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( m_NoShow )      // Hidden text
    {
        aLayers[0] = ITEM_GAL_LAYER( MOD_TEXT_INVISIBLE );
    }
    else
    {
        switch( m_Type )
        {
        case TEXT_is_REFERENCE:
            aLayers[0] = ITEM_GAL_LAYER( MOD_REFERENCES_VISIBLE );
            break;

        case TEXT_is_VALUE:
            aLayers[0] = ITEM_GAL_LAYER( MOD_VALUES_VISIBLE );
            break;

        case TEXT_is_DIVERS:
            aLayers[0] = GetLayer();
        }
    }

    aCount = 1;
}

