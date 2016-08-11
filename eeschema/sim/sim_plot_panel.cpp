/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "sim_plot_panel.h"

#include <algorithm>
#include <limits>


void CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_visible )
        return;

    const auto& dataX = m_trace->GetDataX();
    const auto& dataY = m_trace->GetDataY();

    if( dataX.size() <= 1 )
        return;

    if( m_moved )
    {
        m_coords.x = aWindow.p2x( m_dim.x );

        // Find the closest point coordinates
        auto maxXIt = std::upper_bound( dataX.begin(), dataX.end(), m_coords.x );
        int maxIdx = maxXIt - dataX.begin();
        int minIdx = maxIdx - 1;

        // Out of bounds checks
        if( minIdx < 0 )
        {
            minIdx = 0;
            maxIdx = 1;
            m_coords.x = dataX[0];
        }
        else if( maxIdx >= (int) dataX.size() )
        {
            maxIdx = dataX.size() - 1;
            minIdx = maxIdx - 1;
            m_coords.x = dataX[maxIdx];
        }

        const double leftX = dataX[minIdx];
        const double rightX = dataX[maxIdx];
        const double leftY = dataY[minIdx];
        const double rightY = dataY[maxIdx];

        m_coords.y = leftY + ( rightY - leftY ) / ( rightX - leftX ) * ( m_coords.x - leftX );
        m_moved = false;
    }
    else
    {
        UpdateReference();
    }

    // Line length in horizontal and vertical dimensions
    const int horLen = aWindow.GetScrX();
    const int verLen = aWindow.GetScrY();

    const wxPoint cursorPos( m_window->x2p( m_coords.x ), m_window->y2p( m_coords.y ) );

    aDC.SetPen( wxPen( *wxBLACK, 1, m_continuous ? wxSOLID : wxLONG_DASH ) );

    aDC.DrawLine( -horLen, cursorPos.y, horLen, cursorPos.y );
    aDC.DrawLine( cursorPos.x, -verLen, cursorPos.x, verLen );
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style, const wxString& name )
    : mpWindow( parent, id, pos, size, style ), m_colorIdx( 0 )
{
    //SetMargins( 10, 10, 10, 10 );
    LockAspect();

    m_axis_x = new mpScaleX( wxT( "T [s]" ) );
    m_axis_x->SetTicks( false );
    AddLayer( m_axis_x );

    m_axis_y = new mpScaleY( wxT( "U [V]" ) );
    m_axis_y->SetTicks( false );
    AddLayer( m_axis_y );

    m_legend = new mpInfoLegend( wxRect( 0, 0, 40, 40 ), wxWHITE_BRUSH );
    AddLayer( m_legend );

    //m_coords = new mpInfoCoords( wxRect( 80, 20, 10, 10 ), wxWHITE_BRUSH );
    //AddLayer( m_coords );
}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}


bool SIM_PLOT_PANEL::AddTrace( const wxString& aSpiceName, const wxString& aName, int aPoints,
                                const double* aT, const double* aY, int aFlags )
{
    TRACE* t = NULL;

    // Find previous entry, if there is one
    auto prev = m_traces.find( aName );
    bool addedNewEntry = ( prev == m_traces.end() );

    if( addedNewEntry )
    {
        // New entry
        t = new TRACE( aName, aSpiceName );
        t->SetPen( wxPen( generateColor(), 1, wxSOLID ) );
        m_traces[aName] = t;

        // It is a trick to keep legend always on the top
        DelLayer( m_legend );
        AddLayer( t );
        AddLayer( m_legend );
    }
    else
    {
        t = prev->second;
    }

    t->SetData( std::vector<double>( aT, aT + aPoints ), std::vector<double>( aY, aY + aPoints ) );
    UpdateAll();

    return addedNewEntry;
}


bool SIM_PLOT_PANEL::DeleteTrace( const wxString& aName )
{
    auto trace = m_traces.find( aName );

    if( trace != m_traces.end() )
    {
        m_traces.erase( trace );
        DelLayer( trace->second, true, true );

        return true;
    }

    return false;
}


void SIM_PLOT_PANEL::DeleteAllTraces()
{
    for( auto& t : m_traces )
    {
        DelLayer( t.second, true );
    }

    m_traces.clear();
}


void SIM_PLOT_PANEL::ShowGrid( bool aEnable )
{
    m_axis_x->SetTicks( !aEnable );
    m_axis_y->SetTicks( !aEnable );
    UpdateAll();
}


bool SIM_PLOT_PANEL::IsGridShown() const
{
    assert( m_axis_x->GetTicks() == m_axis_y->GetTicks() );
    return !m_axis_x->GetTicks();
}


wxColour SIM_PLOT_PANEL::generateColor()
{
    /// @todo have a look at:
    /// http://stanford.edu/~mwaskom/software/seaborn/tutorial/color_palettes.html
    /// https://github.com/Gnuplotting/gnuplot-palettes

    const unsigned long colors[] = { 0x000080, 0x008000, 0x800000, 0x008080, 0x800080, 0x808000, 0x808080 };

    //const unsigned long colors[] = { 0xe3cea6, 0xb4781f, 0x8adfb2, 0x2ca033, 0x999afb, 0x1c1ae3, 0x6fbffd, 0x007fff, 0xd6b2ca, 0x9a3d6a };

    // hls
    //const unsigned long colors[] = { 0x0f1689, 0x0f7289, 0x35890f, 0x0f8945, 0x89260f, 0x890f53, 0x89820f, 0x630f89 };

    // pastels, good for dark background
    //const unsigned long colors[] = { 0x2fd8fe, 0x628dfa, 0x53d8a6, 0xa5c266, 0xb3b3b3, 0x94c3e4, 0xca9f8d, 0xac680e };

    const unsigned int colorCount = sizeof(colors) / sizeof(unsigned long);

    /// @todo generate shades to avoid repeating colors
    return wxColour( colors[m_colorIdx++ % colorCount] );
}
