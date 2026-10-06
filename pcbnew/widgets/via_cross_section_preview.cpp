/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/via_cross_section_preview.h>

#include <algorithm>
#include <vector>
#include <kiplatform/ui.h>
#include <math/util.h>
#include <wx/dcclient.h>
#include <wx/intl.h>
#include <wx/region.h>


using GEOMETRY = VIA_CROSS_SECTION_GEOMETRY;


// Paints each material as one body: first every piece in the edge colour one pixel larger,
// then every piece in the fill colour, so touching pieces merge and only the outline of the
// whole body remains. A mixed value is filled hatched so it cannot be mistaken for "off".
class BODY_PAINTER
{
public:
    BODY_PAINTER( wxDC& aDC, double aScale, double aOriginX, double aOriginY ) :
            m_dc( aDC ),
            m_scale( aScale ),
            m_originX( aOriginX ),
            m_originY( aOriginY )
    {
    }

    wxPoint ToDevice( const GEOMETRY::POINT& aPoint ) const
    {
        return wxPoint( KiROUND( m_originX + aPoint.x * m_scale ), KiROUND( m_originY + aPoint.y * m_scale ) );
    }

    wxRect DeviceRect( const GEOMETRY::PIECE& aShape ) const
    {
        wxPoint tl = ToDevice( aShape.m_Points[0] );
        wxPoint br = ToDevice( aShape.m_Points[1] );
        return wxRect( tl.x, tl.y, br.x - tl.x, br.y - tl.y );
    }

    std::vector<wxPoint> DevicePoints( const GEOMETRY::PIECE& aShape ) const
    {
        std::vector<wxPoint> pts;

        for( const GEOMETRY::POINT& pt : aShape.m_Points )
            pts.push_back( ToDevice( pt ) );

        return pts;
    }

    /// The device region a shape covers, for clipping
    wxRegion Region( const GEOMETRY::PIECE& aShape ) const
    {
        if( aShape.m_Rect )
            return wxRegion( DeviceRect( aShape ) );

        std::vector<wxPoint> pts = DevicePoints( aShape );
        return wxRegion( pts.size(), pts.data() );
    }

    void Material( const wxColour& aFill, const wxColour& aEdge, const GEOMETRY::BODY& aBody )
    {
        wxBrush solidBrush( aFill );
        wxBrush hatchBrush( aFill, wxBRUSHSTYLE_CROSSDIAG_HATCH );

        m_dc.SetPen( *wxTRANSPARENT_PEN );
        m_dc.SetBrush( wxBrush( aEdge ) );

        for( const GEOMETRY::PIECE& shape : aBody.m_Pieces )
        {
            if( shape.m_Rect )
            {
                wxRect r = DeviceRect( shape );
                m_dc.DrawRectangle( r.x - 1, r.y - 1, r.width + 2, r.height + 2 );
            }
            else
            {
                std::vector<wxPoint> pts = DevicePoints( shape );
                m_dc.SetPen( wxPen( aEdge, 3 ) );
                m_dc.DrawPolygon( (int) pts.size(), pts.data() );
                m_dc.SetPen( *wxTRANSPARENT_PEN );
            }
        }

        for( const GEOMETRY::PIECE& shape : aBody.m_Pieces )
        {
            m_dc.SetBrush( shape.m_Mixed ? hatchBrush : solidBrush );

            if( shape.m_Rect )
            {
                m_dc.DrawRectangle( DeviceRect( shape ) );
            }
            else
            {
                std::vector<wxPoint> pts = DevicePoints( shape );
                m_dc.DrawPolygon( (int) pts.size(), pts.data() );
            }
        }
    }

private:
    wxDC&  m_dc;
    double m_scale;
    double m_originX;
    double m_originY;
};


VIA_CROSS_SECTION_PREVIEW::VIA_CROSS_SECTION_PREVIEW( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                                                      const wxSize& aSize, long aStyle ) :
        wxPanel()
{
    // The dialog shows through the hole and the cuts, the way it does through a static bitmap
    if( !SetBackgroundStyle( wxBG_STYLE_TRANSPARENT ) )
        SetBackgroundStyle( wxBG_STYLE_PAINT );

    Create( aParent, aId, aPos, aSize, aStyle );
    SetMinSize( FromDIP( wxSize( 160, 70 ) ) );
    SetToolTip( _( "Schematic cross section. Layer positions and cut depths are to scale with the "
                   "board thickness, widths are relative to the hole. Copper and mask thickness "
                   "are exaggerated." ) );
    Bind( wxEVT_PAINT, &VIA_CROSS_SECTION_PREVIEW::onPaint, this );
}


void VIA_CROSS_SECTION_PREVIEW::SetState( const STATE& aState )
{
    m_state = aState;

    const double gridHeight = GEOMETRY( m_state ).GridHeight();
    SetMinSize( FromDIP( wxSize( 160, KiROUND( 160 * gridHeight / GEOMETRY::GRID_W ) ) ) );
    Refresh();
}


void VIA_CROSS_SECTION_PREVIEW::onPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );

    if( GetBackgroundStyle() != wxBG_STYLE_TRANSPARENT )
    {
        dc.SetBackground( wxBrush( GetParent()->GetBackgroundColour() ) );
        dc.Clear();
    }

    const bool dark = KIPLATFORM::UI::IsDarkTheme();

    const wxColour copper( 0xf2, 0x91, 0x00 );
    const wxColour copperEdge = dark ? wxColour( 0xbf, 0x26, 0x41 ) : wxColour( 0x8c, 0x5a, 0x00 );
    const wxColour core = dark ? wxColour( 0x5f, 0x5a, 0x47 ) : wxColour( 0xe9, 0xe1, 0xc6 );
    const wxColour coreEdge = dark ? wxColour( 0x8e, 0x86, 0x6a ) : wxColour( 0x9c, 0x92, 0x6e );
    const wxColour mask = dark ? wxColour( 0xf2, 0x64, 0x7e ) : wxColour( 0x2e, 0x8b, 0x2e );
    const wxColour maskEdge = dark ? wxColour( 0xbf, 0x26, 0x41 ) : wxColour( 0x14, 0x52, 0x14 );
    const wxColour cover( 0x42, 0xb8, 0xeb );
    const wxColour coverEdge( 0x1a, 0x81, 0xc4 );
    const wxColour plug = dark ? wxColour( 0xa9, 0xa9, 0xa9 ) : wxColour( 0xc4, 0xc4, 0xc4 );
    const wxColour plugEdge = dark ? wxColour( 0xd8, 0xd8, 0xd8 ) : wxColour( 0x6e, 0x6e, 0x6e );

    const GEOMETRY geometry( m_state );

    const wxSize size = GetClientSize();
    const double scale = std::min( size.x / GEOMETRY::GRID_W, size.y / geometry.GridHeight() );
    const double originX = ( size.x - GEOMETRY::GRID_W * scale ) / 2;
    const double originY = ( size.y - geometry.GridHeight() * scale ) / 2;

    BODY_PAINTER painter( dc, scale, originX, originY );

    // Air is never painted. Backdrills are clipped out of everything under the mask, and the
    // cuts out of the mask and cover as well, so the dialog shows through them.
    wxRegion underMask( 0, 0, size.x, size.y );
    wxRegion overMask( 0, 0, size.x, size.y );

    for( const GEOMETRY::PIECE& bore : geometry.Bores() )
        underMask.Subtract( painter.Region( bore ) );

    for( const GEOMETRY::PIECE& cut : geometry.Cuts() )
    {
        underMask.Subtract( painter.Region( cut ) );
        overMask.Subtract( painter.Region( cut ) );
    }

    for( const GEOMETRY::BODY& body : geometry.Bodies() )
    {
        dc.SetDeviceClippingRegion( body.m_Clip == GEOMETRY::CLIP::UNDER_MASK ? underMask : overMask );

        switch( body.m_Material )
        {
        case GEOMETRY::MATERIAL::SOLDER_MASK: painter.Material( mask, maskEdge, body ); break;
        case GEOMETRY::MATERIAL::CORE: painter.Material( core, coreEdge, body ); break;
        case GEOMETRY::MATERIAL::PLUG: painter.Material( plug, plugEdge, body ); break;
        case GEOMETRY::MATERIAL::COPPER: painter.Material( copper, copperEdge, body ); break;
        case GEOMETRY::MATERIAL::COVER: painter.Material( cover, coverEdge, body ); break;
        }

        dc.DestroyClippingRegion();
    }
}
