/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "sim_plot_panel.h"

#include <algorithm>
#include <limits>

static wxString formatFloat (double x, int nDigits)
{
    wxString rv, fmt;

    if(nDigits)
    {
        fmt = wxT("%.0Nf");
        fmt[3] = '0' + nDigits;
    } else {
        fmt = wxT("%.0f");
    }

    rv.Printf(fmt, x);

    return rv;
}

static wxString formatSI ( double x, const wxString& unit, int decimalDigits, double maxValue = 0.0, bool lockSuffix = false, char suffix = 0 )
{
    const int n_powers = 11;
    const struct { double exponent; char suffix; } powers[] = {
        {-18,'a'},
        {-15,'f'},
        {-12,'p'},
        {-9,'n'},
        {-6,'u'},
        {-3,'m'},
        {0, 0},
        {3, 'k'},
        {6, 'M'},
        {9, 'G'},
        {12, 'T'},
        {15, 'P'}
    };

    if ( x== 0.0)
    {
        return wxT("0") + unit;
    }

    for ( int i = 0; i <n_powers - 1;i++)
    {
        double r_cur = pow(10, powers[i].exponent);
        bool rangeHit;

        if (maxValue != 0.0)
           rangeHit = fabs(maxValue) >= r_cur && fabs(maxValue) < r_cur * 1000.0 ;
       else
           rangeHit = fabs(x) >= maxValue && fabs(x) < maxValue * 1000.0 ;

        if( (!lockSuffix && rangeHit) || (lockSuffix && suffix == powers[i].suffix ) )
        {
            double v = x / r_cur;
            wxString rv;

            rv = formatFloat ( v, decimalDigits );

            if(powers[i].suffix)
                rv += powers[i].suffix;
            rv += unit;

            return rv;
        }
    }

    return wxT("?");
}


class FREQUENCY_SCALE : public mpScaleXLog
{
public:
    FREQUENCY_SCALE(wxString name, int flags, bool ticks = false, unsigned int type = 0) :
        mpScaleXLog ( name, flags, ticks ,type ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("Hz"), 2 );
    }
};


class TIME_SCALE : public mpScaleX
{
public:
    TIME_SCALE(wxString name, int flags, bool ticks = false, unsigned int type = 0) :
        mpScaleX ( name, flags, ticks ,type ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("s"), 3, AbsVisibleMaxValue() );
    }
};

class GAIN_SCALE : public mpScaleY
{
public:
    GAIN_SCALE(wxString name, int flags,  bool ticks = false, unsigned int type = 0) :
        mpScaleY ( name, flags, ticks ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("dB"), 1, AbsVisibleMaxValue(), true, 0 );
    }
};

class PHASE_SCALE : public mpScaleY
{
public:
    PHASE_SCALE(wxString name, int flags, bool ticks = false, unsigned int type = 0) :
        mpScaleY ( name, flags, ticks ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("\u00B0"), 1, AbsVisibleMaxValue(), true, 0 );
    }
};

class VOLTAGE_SCALE : public mpScaleY
{
public:
    VOLTAGE_SCALE(wxString name, int flags, bool ticks = false, unsigned int type = 0) :
        mpScaleY ( name, flags, ticks ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("V"), 3, AbsVisibleMaxValue() );
    }
};

class CURRENT_SCALE : public mpScaleY
{
public:
    CURRENT_SCALE(wxString name, int flags, bool ticks = false, unsigned int type = 0) :
        mpScaleY ( name, flags, ticks ) {};

    const wxString getLabel( int n )
    {
        return formatSI ( m_labeledTicks[n], wxT("A"), 3, AbsVisibleMaxValue() );
    }
};

void CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_window )
        m_window = &aWindow;

    if( !m_visible )
        return;

    const auto& dataX = m_trace->GetDataX();
    const auto& dataY = m_trace->GetDataY();

    if( dataX.size() <= 1 )
        return;

    if( m_updateRequired )
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
        m_updateRequired = false;

        // Notify the parent window about the changes
        wxQueueEvent( aWindow.GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
    else
    {
        UpdateReference();
    }

    // Line length in horizontal and vertical dimensions
    const int horLen = aWindow.GetScrX();
    const int verLen = aWindow.GetScrY();

    const wxPoint cursorPos( aWindow.x2p( m_coords.x ), aWindow.y2p( m_coords.y ) );

    aDC.SetPen( wxPen( *wxBLACK, 1, m_continuous ? wxSOLID : wxLONG_DASH ) );

    aDC.DrawLine( -horLen, cursorPos.y, horLen, cursorPos.y );
    aDC.DrawLine( cursorPos.x, -verLen, cursorPos.x, verLen );
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( SIM_TYPE aType, wxWindow* parent, wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style, const wxString& name )
    : mpWindow( parent, id, pos, size, style ), m_colorIdx( 0 ),
        m_axis_x( nullptr ), m_axis_y1( nullptr ), m_axis_y2( nullptr ), m_type( aType )
{
    LimitView( true );
    SetMargins(50, 80, 50, 80);

    wxColour grey(96, 96, 96);
	SetColourTheme(*wxBLACK, *wxWHITE, grey);
    EnableDoubleBuffer(true);
    UpdateAll();

    switch( m_type )
    {
        case ST_AC:
            m_axis_x = new FREQUENCY_SCALE( wxT( "Frequency" ), mpALIGN_BOTTOM );
            m_axis_y1 = new GAIN_SCALE( wxT( "Gain" ), mpALIGN_LEFT );
            m_axis_y2 = new PHASE_SCALE( wxT( "Phase" ), mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale(m_axis_y1);

            break;
            #if 0

        case ST_DC:
            m_axis_x = new mpScaleX( wxT( "voltage [V]" ), mpALIGN_BORDER_BOTTOM );
            m_axis_y1 = new mpScaleY( wxT( "voltage [V]" ), mpALIGN_BORDER_LEFT );
            break;

        case ST_NOISE:
            m_axis_x = new mpScaleX( wxT( "frequency [Hz]" ), mpALIGN_BORDER_BOTTOM );
            m_axis_y1 = new mpScaleY( wxT( "noise [(V or A)^2/Hz]" ), mpALIGN_BORDER_LEFT );
            break;

#endif

        case ST_TRANSIENT:
            m_axis_x = new TIME_SCALE( wxT( "Time" ), mpALIGN_BOTTOM );
            m_axis_y1 = new VOLTAGE_SCALE( wxT( "Voltage" ), mpALIGN_LEFT );
            m_axis_y2 = new CURRENT_SCALE( wxT( "Current" ), mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale(m_axis_y1);
            break;

        default:
            // suppress warnings
            break;
    }

    if( m_axis_x )
    {
        m_axis_x->SetTicks( false );
        AddLayer( m_axis_x );
    }

    if( m_axis_y1 )
    {
        m_axis_y1->SetTicks( false );
        AddLayer( m_axis_y1 );
    }

    if( m_axis_y2 )
    {
        m_axis_y2->SetTicks( false );
        AddLayer( m_axis_y2 );
    }

    m_legend = new mpInfoLegend( wxRect( 0, 40, 200, 40 ), wxTRANSPARENT_BRUSH );

    AddLayer( m_legend );
    m_topLevel.push_back( m_legend );
    SetColourTheme(*wxBLACK, *wxWHITE, grey);

    EnableDoubleBuffer(true);
    UpdateAll();

}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}


bool SIM_PLOT_PANEL::IsPlottable( SIM_TYPE aSimType )
{
    switch( aSimType )
    {
        case ST_AC:
        case ST_DC:
        case ST_TRANSIENT:
            return true;

        default:
            return false;
    }
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
        switch ( m_type )
        {
            case ST_TRANSIENT:
                t = new TRACE_TRANSIENT( aName, aSpiceName );
                break;
            case ST_AC:
                t = new TRACE_FREQ_RESPONSE( aName, aSpiceName );
                break;
            default:
                assert(false);
        }

        assert(m_axis_x);
        assert(m_axis_y1);

        t->SetData( std::vector<double>( aT, aT + aPoints ), std::vector<double>( aY, aY + aPoints ) );
        t->SetScale ( m_axis_x, m_axis_y1 );
        t->SetPen( wxPen( generateColor(), 2, wxSOLID ) );
        m_traces[aName] = t;

        // It is a trick to keep legend & coords always on the top
        for( mpLayer* l : m_topLevel )
            DelLayer( l );

        AddLayer( (mpLayer *) t );

        for( mpLayer* l : m_topLevel )
            AddLayer( l );
    }
    else
    {
        t = prev->second;
    }

    UpdateAll();

    return addedNewEntry;
}


bool SIM_PLOT_PANEL::DeleteTrace( const wxString& aName )
{
    auto it = m_traces.find( aName );

    if( it != m_traces.end() )
    {
        m_traces.erase( it );
        TRACE* trace = it->second;

        if( CURSOR* cursor = trace->GetCursor() )
            DelLayer( cursor, true );

        DelLayer( trace, true, true );

        return true;
    }

    return false;
}


void SIM_PLOT_PANEL::DeleteAllTraces()
{
    for( auto& t : m_traces )
    {
        DeleteTrace( t.first );
    }

    m_traces.clear();
}


bool SIM_PLOT_PANEL::HasCursorEnabled( const wxString& aName ) const
{
    TRACE* t = GetTrace( aName );

    return t ? t->HasCursor() : false;
}


void SIM_PLOT_PANEL::EnableCursor( const wxString& aName, bool aEnable )
{
    TRACE* t = GetTrace( aName );

    if( t == nullptr || t->HasCursor() == aEnable )
        return;

    if( aEnable )
    {
        CURSOR* c = new CURSOR( t );
        t->SetCursor( c );
        AddLayer( c );
    }
    else
    {
        CURSOR* c = t->GetCursor();
        t->SetCursor( NULL );
        DelLayer( c, true );
    }

    // Notify the parent window about the changes
    wxQueueEvent( GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
}


wxColour SIM_PLOT_PANEL::generateColor()
{
    /// @todo have a look at:
    /// http://stanford.edu/~mwaskom/software/seaborn/tutorial/color_palettes.html
    /// https://github.com/Gnuplotting/gnuplot-palettes

    const unsigned long colors[] = { 0x0000ff, 0x00ff00, 0xff0000, 0x00ffff, 0xff00ff, 0xffff000, 0xffffff };

    //const unsigned long colors[] = { 0xe3cea6, 0xb4781f, 0x8adfb2, 0x2ca033, 0x999afb, 0x1c1ae3, 0x6fbffd, 0x007fff, 0xd6b2ca, 0x9a3d6a };

    // hls
    //const unsigned long colors[] = { 0x0f1689, 0x0f7289, 0x35890f, 0x0f8945, 0x89260f, 0x890f53, 0x89820f, 0x630f89 };

    // pastels, good for dark background
    //const unsigned long colors[] = { 0x2fd8fe, 0x628dfa, 0x53d8a6, 0xa5c266, 0xb3b3b3, 0x94c3e4, 0xca9f8d, 0xac680e };

    const unsigned int colorCount = sizeof(colors) / sizeof(unsigned long);

    /// @todo generate shades to avoid repeating colors
    return wxColour( colors[m_colorIdx++ % colorCount] );
}

wxDEFINE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );
