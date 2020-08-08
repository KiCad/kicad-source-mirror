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
#include <gr_text.h>
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <bitmaps.h>
#include <class_board.h>
#include <class_module.h>
#include <view/view.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>

TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, TEXT_TYPE text_type ) :
    BOARD_ITEM( parent, PCB_MODULE_TEXT_T ),
    EDA_TEXT()
{
    MODULE* module = static_cast<MODULE*>( m_Parent );

    m_Type = text_type;
    m_keepUpright = true;

    // Set text thickness to a default value
    SetTextThickness( Millimeter2iu( DEFAULT_TEXT_WIDTH ) );
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
    EDA_RECT rect = GetTextBox();
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
        return rect.Intersects( GetTextBox(), GetDrawRotation() );
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
        SetHorizJustify( static_cast<EDA_TEXT_HJUSTIFY_T>( -GetHorizJustify() ) );
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
    EDA_RECT text_area = GetTextBox();

    if( angle )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return text_area;
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
void TEXTE_MODULE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
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
    aList.emplace_back( _( "Footprint" ), Line, DARKCYAN );

    Line = GetShownText();
    aList.emplace_back( _( "Text" ), Line, BROWN );

    wxASSERT( m_Type >= TEXT_is_REFERENCE && m_Type <= TEXT_is_DIVERS );
    aList.emplace_back( _( "Type" ), text_type_msg[m_Type], DARKGREEN );

    if( !IsVisible() )
        msg = _( "No" );
    else
        msg = _( "Yes" );

    aList.emplace_back( _( "Display" ), msg, DARKGREEN );

    // Display text layer
    aList.emplace_back( _( "Layer" ), GetLayerName(), DARKGREEN );

    if( IsMirrored() )
        msg = _( "Yes" );
    else
        msg = _( "No" );

    aList.emplace_back( _( "Mirror" ), msg, DARKGREEN );

    msg.Printf( wxT( "%.1f" ), GetTextAngleDegrees() );
    aList.emplace_back( _( "Angle" ), msg, DARKGREEN );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextThickness(), true );
    aList.emplace_back( _( "Thickness" ), msg, DARKGREEN );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextWidth(), true );
    aList.emplace_back( _( "Width" ), msg, RED );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextHeight(), true );
    aList.emplace_back( _( "Height" ), msg, RED );
}


wxString TEXTE_MODULE::GetSelectMenuText( EDA_UNITS aUnits ) const
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
    EDA_RECT text_area = GetTextBox();

    if( angle != 0.0 )
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
    if( ( m_Type == TEXT_is_VALUE || GetText() == wxT( "${VALUE}" ) )
            && !aView->IsLayerVisible( LAYER_MOD_VALUES ) )
        return HIDE;

    if( ( m_Type == TEXT_is_REFERENCE || GetText() == wxT( "${REFERENCE}" ) )
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


wxString TEXTE_MODULE::GetShownText( int aDepth ) const
{
    const MODULE* module = static_cast<MODULE*>( GetParent() );
    wxASSERT( module );

    std::function<bool( wxString* )> moduleResolver =
            [&]( wxString* token ) -> bool
            {
                return module && module->ResolveTextVar( token, aDepth );
            };

    bool     processTextVars = false;
    wxString text = EDA_TEXT::GetShownText( &processTextVars );

    if( processTextVars )
    {
        PROJECT* project = nullptr;

        if( module && module->GetParent() )
            project = static_cast<BOARD*>( module->GetParent() )->GetProject();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &moduleResolver, project );
    }

    return text;
}


std::shared_ptr<SHAPE> TEXTE_MODULE::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return GetEffectiveTextShape();
}


static struct TEXTE_MODULE_DESC
{
    TEXTE_MODULE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( TEXTE_MODULE );
        propMgr.AddTypeCast( new TYPE_CAST<TEXTE_MODULE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<TEXTE_MODULE, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( TEXTE_MODULE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( TEXTE_MODULE ), TYPE_HASH( EDA_TEXT ) );
    }
} _TEXTE_MODULE_DESC;
