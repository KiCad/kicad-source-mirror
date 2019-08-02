/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <gr_text.h>
#include <kicad_string.h>
#include <richio.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <msgpanel.h>
#include <base_units.h>
#include <bitmaps.h>
#include <class_board.h>
#include <class_module.h>
#include <view/view.h>
#include <pcbnew.h>


TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, TEXT_TYPE text_type ) :
    BOARD_ITEM( parent, PCB_MODULE_TEXT_T ),
    EDA_TEXT()
{
    MODULE* module = static_cast<MODULE*>( m_Parent );

    m_Type = text_type;
    m_keepUpright = true;

    // Set text thickness to a default value
    SetThickness( Millimeter2iu( DEFAULT_TEXT_WIDTH ) );
    SetLayer( F_SilkS );

    // Set position and give a default layer if a valid parent footprint exists
    if( module && ( module->Type() == PCB_MODULE_T ) )
    {
        SetTextPos( module->GetPosition() );

        if( IsBackLayer( module->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            SetMirrored( true );
        }
    }

    SetDrawCoord();
}


TEXTE_MODULE::~TEXTE_MODULE()
{
}


void TEXTE_MODULE::SetTextAngle( double aAngle )
{
    EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
}


bool TEXTE_MODULE::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox( -1 );
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );

    RotatePoint( &location, GetTextPos(), -GetDrawRotation() );

    return rect.Contains( location );
}


bool TEXTE_MODULE::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );
    else
        return rect.Intersects( GetTextBox( -1 ), GetDrawRotation() );
}


void TEXTE_MODULE::KeepUpright( double aOldOrientation, double aNewOrientation )
{
    if( !IsKeepUpright() )
        return;

    double currentAngle = GetTextAngle() + aOldOrientation;
    double newAngle = GetTextAngle() + aNewOrientation;

    NORMALIZE_ANGLE_POS( currentAngle );
    NORMALIZE_ANGLE_POS( newAngle );

    bool   isFlipped = currentAngle >= 1800.0;
    bool   needsFlipped = newAngle >= 1800.0;

    if( isFlipped != needsFlipped )
    {
        if( GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
            SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
            SetHorizJustify(GR_TEXT_HJUSTIFY_LEFT );

        SetTextAngle( GetTextAngle() + 1800.0 );
        SetDrawCoord();
    }
}


void TEXTE_MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // Used in footprint editing
    // Note also in module editor, m_Pos0 = m_Pos

    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aRotCentre, aAngle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() + aAngle );
    SetLocalCoord();
}


void TEXTE_MODULE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    // flipping the footprint is relative to the X axis
    if( aFlipLeftRight )
        SetTextX( ::Mirror( GetTextPos().x, aCentre.x ) );
    else
        SetTextY( ::Mirror( GetTextPos().y, aCentre.y ) );

    SetTextAngle( -GetTextAngle() );

    SetLayer( FlipLayer( GetLayer() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
    SetLocalCoord();

    // adjust justified text for mirroring
    if( GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT || GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
    {
        if( ( GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT ) == IsMirrored() )
            SetHorizJustify( (EDA_TEXT_HJUSTIFY_T)-GetHorizJustify() );
        else
            SetHorizJustify( (EDA_TEXT_HJUSTIFY_T)-GetHorizJustify() );

        SetDrawCoord();
    }
}

bool TEXTE_MODULE::IsParentFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}


void TEXTE_MODULE::Mirror( const wxPoint& aCentre, bool aMirrorAroundXAxis )
{
    // Used in modedit, to transform the footprint
    // the mirror is around the Y axis or X axis if aMirrorAroundXAxis = true
    // the position is mirrored, but the text itself is not mirrored
    if( aMirrorAroundXAxis )
        SetTextY( ::Mirror( GetTextPos().y, aCentre.y ) );
    else
        SetTextX( ::Mirror( GetTextPos().x, aCentre.x ) );

    SetLocalCoord();
}


void TEXTE_MODULE::Move( const wxPoint& aMoveVector )
{
    Offset( aMoveVector );
    SetLocalCoord();
}


int TEXTE_MODULE::GetLength() const
{
    return GetText().Len();
}


void TEXTE_MODULE::SetDrawCoord()
{
    const MODULE* module = static_cast<const MODULE*>( m_Parent );

    SetTextPos( m_Pos0 );

    if( module  )
    {
        double angle = module->GetOrientation();

        wxPoint pt = GetTextPos();
        RotatePoint( &pt, angle );
        SetTextPos( pt );

        Offset( module->GetPosition() );
    }
}


void TEXTE_MODULE::SetLocalCoord()
{
    const MODULE* module = static_cast<const MODULE*>( m_Parent );

    if( module )
    {
        m_Pos0 = GetTextPos() - module->GetPosition();

        double angle = module->GetOrientation();

        RotatePoint( &m_Pos0.x, &m_Pos0.y, -angle );
    }
    else
    {
        m_Pos0 = GetTextPos();
    }
}

const EDA_RECT TEXTE_MODULE::GetBoundingBox() const
{
    double   angle = GetDrawRotation();
    EDA_RECT text_area = GetTextBox( -1, -1 );

    if( angle )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return text_area;
}


void TEXTE_MODULE::Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset )
{
    /* parent must *not* be NULL (a footprint text without a footprint parent has no sense) */
    wxASSERT( m_Parent );

    BOARD*         brd = GetBoard( );
    KIGFX::COLOR4D color = aFrame->Settings().Colors().GetLayerColor( GetLayer() );
    PCB_LAYER_ID   text_layer = GetLayer();

    if( !brd->IsLayerVisible( m_Layer )
      || ( IsFrontLayer( text_layer ) && !brd->IsElementVisible( LAYER_MOD_TEXT_FR ) )
      || ( IsBackLayer( text_layer ) && !brd->IsElementVisible( LAYER_MOD_TEXT_BK ) ) )
        return;

    if( !brd->IsElementVisible( LAYER_MOD_REFERENCES ) && GetText() == wxT( "%R" ) )
        return;

    if( !brd->IsElementVisible( LAYER_MOD_VALUES ) && GetText() == wxT( "%V" ) )
        return;

    // Invisible texts are still drawn (not plotted) in LAYER_MOD_TEXT_INVISIBLE
    // Just because we must have to edit them (at least to make them visible)
    if( !IsVisible() )
    {
        if( !brd->IsElementVisible( LAYER_MOD_TEXT_INVISIBLE ) )
            return;

        color = aFrame->Settings().Colors().GetItemColor( LAYER_MOD_TEXT_INVISIBLE );
    }

    auto displ_opts = (PCB_DISPLAY_OPTIONS*)( aFrame->GetDisplayOptions() );

    // Draw mode compensation for the width
    int width = GetThickness();

    if( displ_opts && displ_opts->m_DisplayModTextFill == SKETCH )
        width = -width;

    wxPoint pos = GetTextPos() - aOffset;

    // Draw the text proper, with the right attributes
    wxSize size   = GetTextSize();
    double orient = GetDrawRotation();

    // If the text is mirrored : negate size.x (mirror / Y axis)
    if( IsMirrored() )
        size.x = -size.x;

    GRText( aDC, pos, color, GetShownText(), orient, size, GetHorizJustify(), GetVertJustify(),
            width, IsItalic(), IsBold() );
}


double TEXTE_MODULE::GetDrawRotation() const
{
    MODULE* module = (MODULE*) m_Parent;
    double  rotation = GetTextAngle();

    if( module )
        rotation += module->GetOrientation();

    if( m_keepUpright )
    {
        // Keep angle between -90 .. 90 deg. Otherwise the text is not easy to read
        while( rotation > 900 )
            rotation -= 1800;

        while( rotation < -900 )
            rotation += 1800;
    }
    else
    {
        NORMALIZE_ANGLE_POS( rotation );
    }

    return rotation;
}


// see class_text_mod.h
void TEXTE_MODULE::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
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
    aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), Line, DARKCYAN ) );

    Line = GetShownText();
    aList.push_back( MSG_PANEL_ITEM( _( "Text" ), Line, BROWN ) );

    wxASSERT( m_Type >= TEXT_is_REFERENCE && m_Type <= TEXT_is_DIVERS );
    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), text_type_msg[m_Type], DARKGREEN ) );

    if( !IsVisible() )
        msg = _( "No" );
    else
        msg = _( "Yes" );

    aList.push_back( MSG_PANEL_ITEM( _( "Display" ), msg, DARKGREEN ) );

    // Display text layer
    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), DARKGREEN ) );

    if( IsMirrored() )
        msg = _( "Yes" );
    else
        msg = _( "No" );

    aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), msg, DARKGREEN ) );

    msg.Printf( wxT( "%.1f" ), GetTextAngleDegrees() );
    aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, DARKGREEN ) );

    msg = MessageTextFromValue( aUnits, GetThickness(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Thickness" ), msg, DARKGREEN ) );

    msg = MessageTextFromValue( aUnits, GetTextWidth(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, RED ) );

    msg = MessageTextFromValue( aUnits, GetTextHeight(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg, RED ) );
}


wxString TEXTE_MODULE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        return wxString::Format( _( "Reference %s" ),
                                 static_cast<MODULE*>( GetParent() )->GetReference() );

    case TEXT_is_VALUE:
        return wxString::Format( _( "Value %s of %s" ),
                                 GetShownText(),
                                 static_cast<MODULE*>( GetParent() )->GetReference() );

    default:    // wrap this one in quotes:
        return wxString::Format( _( "Text \"%s\" of %s on %s" ),
                                 ShortenedShownText(),
                                 static_cast<MODULE*>( GetParent() )->GetReference(),
                                 GetLayerName() );
    }
}


BITMAP_DEF TEXTE_MODULE::GetMenuImage() const
{
    return footprint_text_xpm;
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
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return BOX2I( text_area.GetPosition(), text_area.GetSize() );
}


void TEXTE_MODULE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_MOD_TEXT_INVISIBLE;

    aCount = 1;
}


unsigned int TEXTE_MODULE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    const int HIDE = std::numeric_limits<unsigned int>::max();

    if( !aView )
        return 0;

    // Hidden text gets put on the LAYER_MOD_TEXT_INVISIBLE for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    // Handle Render tab switches
    if( ( m_Type == TEXT_is_VALUE || GetText() == wxT( "%V" ) )
            && !aView->IsLayerVisible( LAYER_MOD_VALUES ) )
        return HIDE;

    if( ( m_Type == TEXT_is_REFERENCE || GetText() == wxT( "%R" ) )
            && !aView->IsLayerVisible( LAYER_MOD_REFERENCES ) )
        return HIDE;

    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( IsFrontLayer( m_Layer ) && !aView->IsLayerVisible( LAYER_MOD_TEXT_FR ) )
        return HIDE;

    if( IsBackLayer( m_Layer ) && !aView->IsLayerVisible( LAYER_MOD_TEXT_BK ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0;
}


wxString TEXTE_MODULE::GetShownText() const
{
    /* First order optimization: no % means that no processing is
     * needed; just hope that RVO and copy constructor implementation
     * avoid to copy the whole block; anyway it should be better than
     * rebuild the string one character at a time...
     * Also it seems wise to only expand macros in user text (but there
     * is no technical reason, probably) */

    if( (m_Type != TEXT_is_DIVERS) || (wxString::npos == GetText().find('%')) )
        return GetText();

    wxString newbuf;
    const MODULE *module = static_cast<MODULE*>( GetParent() );

    for( wxString::const_iterator it = GetText().begin(); it != GetText().end(); ++it )
    {
        // Process '%' and copy everything else
        if( *it != '%' )
            newbuf.append(*it);
        else
        {
            /* Look at the next character (if is it there) and append
             * its expansion */
            ++it;

            if( it != GetText().end() )
            {
                switch( char(*it) )
                {
                case '%':
                    newbuf.append( '%' );
                    break;

                case 'R':
                    if( module )
                        newbuf.append( module->GetReference() );
                    break;

                case 'V':
                    if( module )
                        newbuf.append( module->GetValue() );
                    break;

                default:
                    newbuf.append( '?' );
                    break;
                }
            }
            else
                break; // The string is over and we can't ++ anymore
        }
    }

    return newbuf;
}
