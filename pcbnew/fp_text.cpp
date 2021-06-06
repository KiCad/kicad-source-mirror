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

#include <pcb_edit_frame.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <core/mirror.h>
#include <footprint.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <kicad_string.h>
#include <painter.h>
#include <geometry/shape_compound.h>

FP_TEXT::FP_TEXT( FOOTPRINT* aParentFootprint, TEXT_TYPE text_type ) :
    BOARD_ITEM( aParentFootprint, PCB_FP_TEXT_T ),
    EDA_TEXT()
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    m_Type = text_type;
    m_keepUpright = true;

    // Set text thickness to a default value
    SetTextThickness( Millimeter2iu( DEFAULT_TEXT_WIDTH ) );
    SetLayer( F_SilkS );

    // Set position and give a default layer if a valid parent footprint exists
    if( parentFootprint && parentFootprint->Type() == PCB_FOOTPRINT_T )
    {
        SetTextPos( parentFootprint->GetPosition() );

        if( IsBackLayer( parentFootprint->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            SetMirrored( true );
        }
    }

    SetDrawCoord();
}


FP_TEXT::~FP_TEXT()
{
}


void FP_TEXT::SetTextAngle( double aAngle )
{
    EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
}


bool FP_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox();
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );

    RotatePoint( &location, GetTextPos(), -GetDrawRotation() );

    return rect.Contains( location );
}


bool FP_TEXT::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );
    else
        return rect.Intersects( GetTextBox(), GetDrawRotation() );
}


void FP_TEXT::KeepUpright( double aOldOrientation, double aNewOrientation )
{
    if( !IsKeepUpright() )
        return;

    double newAngle = GetTextAngle() + aNewOrientation;
    NORMALIZE_ANGLE_POS( newAngle );
    bool   needsFlipped = newAngle >= 1800.0;

    if( needsFlipped )
    {
        SetHorizJustify( static_cast<EDA_TEXT_HJUSTIFY_T>( -GetHorizJustify() ) );
        SetTextAngle( GetTextAngle() + 1800.0 );
        SetDrawCoord();
    }
}


void FP_TEXT::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // Used in footprint editing
    // Note also in footprint editor, m_Pos0 = m_Pos

    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aRotCentre, aAngle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() + aAngle );
    SetLocalCoord();
}


void FP_TEXT::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    // flipping the footprint is relative to the X axis
    if( aFlipLeftRight )
    {
        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
        SetTextAngle( 1800 - GetTextAngle() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
    SetLocalCoord();
}

bool FP_TEXT::IsParentFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}


void FP_TEXT::Mirror( const wxPoint& aCentre, bool aMirrorAroundXAxis )
{
    // the position is mirrored, but the text itself is not mirrored

    if( aMirrorAroundXAxis )
        SetTextY( ::MIRRORVAL( GetTextPos().y, aCentre.y ) );
    else
        SetTextX( ::MIRRORVAL( GetTextPos().x, aCentre.x ) );

    SetLocalCoord();
}


void FP_TEXT::Move( const wxPoint& aMoveVector )
{
    Offset( aMoveVector );
    SetLocalCoord();
}


int FP_TEXT::GetLength() const
{
    return GetText().Len();
}


void FP_TEXT::SetDrawCoord()
{
    const FOOTPRINT* parentFootprint = static_cast<const FOOTPRINT*>( m_parent );

    SetTextPos( m_Pos0 );

    if( parentFootprint  )
    {
        double angle = parentFootprint->GetOrientation();

        wxPoint pt = GetTextPos();
        RotatePoint( &pt, angle );
        SetTextPos( pt );

        Offset( parentFootprint->GetPosition() );
    }
}


void FP_TEXT::SetLocalCoord()
{
    const FOOTPRINT* parentFootprint = static_cast<const FOOTPRINT*>( m_parent );

    if( parentFootprint )
    {
        m_Pos0 = GetTextPos() - parentFootprint->GetPosition();

        double angle = parentFootprint->GetOrientation();

        RotatePoint( &m_Pos0.x, &m_Pos0.y, -angle );
    }
    else
    {
        m_Pos0 = GetTextPos();
    }
}

const EDA_RECT FP_TEXT::GetBoundingBox() const
{
    double   angle = GetDrawRotation();
    EDA_RECT text_area = GetTextBox();

    if( angle )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return text_area;
}


double FP_TEXT::GetDrawRotation() const
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );
    double     rotation = GetTextAngle();

    if( parentFootprint )
        rotation += parentFootprint->GetOrientation();

    if( m_keepUpright )
    {
        // Keep angle between 0 .. 90 deg. Otherwise the text is not easy to read
        while( rotation > 900 )
            rotation -= 1800;

        while( rotation < 0 )
            rotation += 1800;
    }
    else
    {
        NORMALIZE_ANGLE_POS( rotation );
    }

    return rotation;
}


void FP_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent );
    wxString   msg;

    static const wxString text_type_msg[3] =
    {
        _( "Ref." ), _( "Value" ), _( "Text" )
    };

    aList.emplace_back( _( "Footprint" ), fp ? fp->GetReference() : _( "<invalid>" ) );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), UnescapeString( GetText() ) );

    wxASSERT( m_Type >= TEXT_is_REFERENCE && m_Type <= TEXT_is_DIVERS );
    aList.emplace_back( _( "Type" ), text_type_msg[m_Type] );

    if( IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Display" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    // Display text layer
    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    msg.Printf( wxT( "%g" ), GetTextAngleDegrees() );
    aList.emplace_back( _( "Angle" ), msg );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextThickness() );
    aList.emplace_back( _( "Thickness" ), msg );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextWidth() );
    aList.emplace_back( _( "Width" ), msg );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextHeight() );
    aList.emplace_back( _( "Height" ), msg );
}


wxString FP_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        return wxString::Format( _( "Reference '%s'" ),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );

    case TEXT_is_VALUE:
        return wxString::Format( _( "Value '%s' of %s" ),
                                 GetShownText(),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );

    default:
        return wxString::Format( _( "Footprint Text '%s' of %s" ),
                                 ShortenedShownText(),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );
    }
}


BITMAPS FP_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


EDA_ITEM* FP_TEXT::Clone() const
{
    return new FP_TEXT( *this );
}


const BOX2I FP_TEXT::ViewBBox() const
{
    double   angle = GetDrawRotation();
    EDA_RECT text_area = GetTextBox();

    if( angle != 0.0 )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return BOX2I( text_area.GetPosition(), text_area.GetSize() );
}


void FP_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_MOD_TEXT_INVISIBLE;

    aCount = 1;
}


double FP_TEXT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = (double)std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    // Hidden text gets put on the LAYER_MOD_TEXT_INVISIBLE for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    RENDER_SETTINGS* renderSettings = aView->GetPainter()->GetSettings();
    COLOR4D          backgroundColor = renderSettings->GetLayerColor( LAYER_PCB_BACKGROUND );

    // Handle Render tab switches
    if( m_Type == TEXT_is_VALUE || GetText() == wxT( "${VALUE}" ) )
    {
        if( !aView->IsLayerVisible( LAYER_MOD_VALUES )
                || renderSettings->GetLayerColor( LAYER_MOD_VALUES ) == backgroundColor )
        {
            return HIDE;
        }
    }

    if( m_Type == TEXT_is_REFERENCE || GetText() == wxT( "${REFERENCE}" ) )
    {
        if( !aView->IsLayerVisible( LAYER_MOD_REFERENCES )
                || renderSettings->GetLayerColor( LAYER_MOD_REFERENCES ) == backgroundColor )
        {
            return HIDE;
        }
    }

    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( IsFrontLayer( m_layer ) && !aView->IsLayerVisible( LAYER_MOD_TEXT_FR ) )
        return HIDE;

    if( IsBackLayer( m_layer ) && !aView->IsLayerVisible( LAYER_MOD_TEXT_BK ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0.0;
}


wxString FP_TEXT::GetShownText( int aDepth ) const
{
    const FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( GetParent() );
    wxASSERT( parentFootprint );
    const BOARD*  board = parentFootprint->GetBoard();

    std::function<bool( wxString* )> footprintResolver =
            [&]( wxString* token ) -> bool
            {
                return parentFootprint && parentFootprint->ResolveTextVar( token, aDepth );
            };

    std::function<bool( wxString* )> boardTextResolver =
            [&]( wxString* token ) -> bool
            {
                return board->ResolveTextVar( token, aDepth + 1 );
            };

    bool     processTextVars = false;
    wxString text = EDA_TEXT::GetShownText( &processTextVars );

    if( processTextVars )
    {
        PROJECT* project = nullptr;

        if( parentFootprint && parentFootprint->GetParent() )
            project = static_cast<BOARD*>( parentFootprint->GetParent() )->GetProject();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &footprintResolver, &boardTextResolver, project );
    }

    return text;
}


std::shared_ptr<SHAPE> FP_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return GetEffectiveTextShape();
}


static struct FP_TEXT_DESC
{
    FP_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( FP_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<FP_TEXT, wxString>( _HKI( "Parent" ),
                    NO_SETTER( FP_TEXT, wxString ), &FP_TEXT::GetParentAsString ) );
    }
} _FP_TEXT_DESC;
