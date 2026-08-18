/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/line_ending_bitmap.h>
#include <line_ending.h>
#include <geometry/eda_angle.h>
#include <math/vector2d.h>
#include <wx/dcmemory.h>
#include <wx/window.h>
#include <algorithm>


wxBitmap MakeLineEndingBitmap( LINE_ENDING_STYLE aStyle, const wxSize& aSize,
                               const wxColour& aForeground, const wxColour& aBackground,
                               wxWindow* aWindow, bool aShapeOnRight )
{
    double scaleFactor = aWindow->GetDPIScaleFactor();

    wxSize physSize( aWindow->ToPhys( aSize.GetWidth() ),
                     aWindow->ToPhys( aSize.GetHeight() ) );

    wxBitmap bitmap( physSize );
    wxMemoryDC dc( bitmap );

    dc.SetBackground( wxBrush( aBackground ) );
    dc.Clear();

    int w = physSize.GetWidth();
    int h = physSize.GetHeight();
    int midY = h / 2;
    int marginL = static_cast<int>( 4 * scaleFactor );
    int marginR = static_cast<int>( 4 * scaleFactor );

    int lineStartX = marginL;
    int lineEndX = w - marginR;
    int lineDrawStartX = lineStartX;
    int lineDrawEndX = lineEndX;

    if( aStyle != LINE_ENDING_STYLE::NONE )
    {
        int shapeHeight = h - static_cast<int>( 4 * scaleFactor );
        int fakeLineWidth = std::max( 1, static_cast<int>( shapeHeight / LINE_ENDING::DEFAULT_RATIO_WIDTH ) );

        LINE_ENDING ending( aStyle );

        if( aStyle == LINE_ENDING_STYLE::ARROW_OPEN )
        {
            int len = static_cast<int>( fakeLineWidth * LINE_ENDING::DEFAULT_RATIO_LENGTH * 0.75 );
            int wid = static_cast<int>( fakeLineWidth * LINE_ENDING::DEFAULT_RATIO_WIDTH * 1.2 );
            ending = LINE_ENDING( aStyle, len, wid );
        }

        VECTOR2I tip;
        EDA_ANGLE tangent;

        if( aShapeOnRight )
        {
            tip = VECTOR2I( lineEndX, midY );
            tangent = EDA_ANGLE( 0.0, DEGREES_T );
        }
        else
        {
            tip = VECTOR2I( lineStartX, midY );
            tangent = EDA_ANGLE( 180.0, DEGREES_T );
        }

        std::vector<VECTOR2I> polygon;
        ending.GetShapes( tip, tangent, fakeLineWidth, polygon );

        if( !polygon.empty() )
        {
            std::vector<wxPoint> wxPts;
            wxPts.reserve( polygon.size() );

            for( const VECTOR2I& pt : polygon )
                wxPts.emplace_back( pt.x, pt.y );

            int shortenDepth = ending.GetShortenDepth( fakeLineWidth );

            if( aShapeOnRight )
                lineDrawEndX = lineEndX - shortenDepth;
            else
                lineDrawStartX = lineStartX + shortenDepth;

            dc.SetPen( wxPen( aForeground, static_cast<int>( scaleFactor ) ) );

            if( aStyle == LINE_ENDING_STYLE::ARROW_OPEN )
            {
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.SetPen( wxPen( aForeground, std::max( 1, static_cast<int>( 1.5 * scaleFactor ) ) ) );
                dc.DrawLines( static_cast<int>( wxPts.size() ), wxPts.data() );
            }
            else
            {
                dc.SetBrush( wxBrush( aForeground ) );
                dc.DrawPolygon( static_cast<int>( wxPts.size() ), wxPts.data() );
            }
        }
    }

    dc.SetPen( wxPen( aForeground, static_cast<int>( scaleFactor ) ) );
    dc.DrawLine( lineDrawStartX, midY, lineDrawEndX, midY );

    dc.SelectObject( wxNullBitmap );
    bitmap.SetScaleFactor( scaleFactor );
    return bitmap;
}
