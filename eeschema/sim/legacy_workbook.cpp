/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <wx/debug.h>

#include <project/project_file.h>
#include <confirm.h>
#include <string_utils.h>
#include <sim/simulator_frame_ui.h>
#include <sim/simulator_frame.h>
#include <sim/sim_plot_tab.h>


void SIMULATOR_FRAME_UI::parseTraceParams( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace,
                                           const wxString& aSignalName, const wxString& aParams )
{
    auto addCursor =
            [&]( int aCursorId, double x )
            {
                CURSOR* cursor = new CURSOR( aTrace, aPlotTab );

                cursor->SetName( aSignalName );
                cursor->SetCoordX( x );

                aTrace->SetCursor( aCursorId, cursor );
                aPlotTab->GetPlotWin()->AddLayer( cursor );
            };

    wxArrayString items = wxSplit( aParams, '|' );

    for( const wxString& item : items )
    {
        if( item.StartsWith( wxS( "rgb" ) ) )
        {
            wxColour color;
            color.Set( item );
            aTrace->SetTraceColour( color );
            aPlotTab->UpdateTraceStyle( aTrace );
        }
        else if( item.StartsWith( wxS( "cursor1" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );
            double        val;

            if( parts.size() == 3 )
            {
                parts[0].AfterFirst( '=' ).ToDouble( &val );
                m_cursorFormats[0][0].FromString( parts[1] );
                m_cursorFormats[0][1].FromString( parts[2] );
                addCursor( 1, val );
            }
        }
        else if( item.StartsWith( wxS( "cursor2" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );
            double        val;

            if( parts.size() == 3 )
            {
                parts[0].AfterFirst( '=' ).ToDouble( &val );
                m_cursorFormats[1][0].FromString( parts[1] );
                m_cursorFormats[1][1].FromString( parts[2] );
                addCursor( 2, val );
            }
        }
        else if( item.StartsWith( wxS( "cursorD" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );

            if( parts.size() == 3 )
            {
                m_cursorFormats[2][0].FromString( parts[1] );
                m_cursorFormats[2][1].FromString( parts[2] );
            }
        }
        else if( item == wxS( "dottedSecondary" ) )
        {
            aPlotTab->SetDottedSecondary( true );
        }
        else if( item == wxS( "hideGrid" ) )
        {
            aPlotTab->ShowGrid( false );
        }
    }
}


bool SIMULATOR_FRAME_UI::loadLegacyWorkbook( const wxString& aPath )
{
    wxTextFile file( aPath );

#define EXPECTING( msg )                                                                          \
        DisplayErrorMessage( this, wxString::Format( _( "Error loading workbook: line %d: %s." ), \
                                                     file.GetCurrentLine()+1,                     \
                                                     msg ) )

    if( !file.Open() )
        return false;

    long     version = 1;
    wxString firstLine = file.GetFirstLine();
    wxString pageCountLine;

    if( firstLine.StartsWith( wxT( "version " ) ) )
    {
        if( !firstLine.substr( 8 ).ToLong( &version ) )
        {
            EXPECTING( _( "expecting version" ) );
            file.Close();

            return false;
        }

        pageCountLine = file.GetNextLine();
    }
    else
    {
        pageCountLine = firstLine;
    }

    long tabCount;

    if( !pageCountLine.ToLong( &tabCount ) )
    {
        EXPECTING( _( "expecting simulation tab count" ) );
        file.Close();

        return false;
    }

    std::map<SIM_PLOT_TAB*, std::vector<std::tuple<long, wxString, wxString>>> traceInfo;

    for( long i = 0; i < tabCount; ++i )
    {
        long simType, tracesCount;

        if( !file.GetNextLine().ToLong( &simType ) )
        {
            EXPECTING( _( "expecting simulation tab type" ) );
            file.Close();

            return false;
        }

        wxString          command = UnescapeString( file.GetNextLine() );
        wxString          simCommand;
        int               simOptions = NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS;
        wxStringTokenizer tokenizer( command, "\r\n", wxTOKEN_STRTOK );

        if( version >= 2 )
        {
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
        }

        if( version >= 3 )
        {
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
        }

        while( tokenizer.HasMoreTokens() )
        {
            wxString line = tokenizer.GetNextToken();

            if( line.StartsWith( wxT( ".kicad adjustpaths" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
            else if( line.StartsWith( wxT( ".save all" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
            else if( line.StartsWith( wxT( ".probe alli" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
            else if( line.StartsWith( wxT( ".probe allp" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
            else
                simCommand += line + wxT( "\n" );
        }

        SIM_TAB*      simTab = NewSimTab( simCommand );
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab );

        simTab->SetSimOptions( simOptions );

        if( !file.GetNextLine().ToLong( &tracesCount ) )
        {
            EXPECTING( _( "expecting trace count" ) );
            file.Close();

            return false;
        }

        if( plotTab )
            traceInfo[plotTab] = {};

        for( long j = 0; j < tracesCount; ++j )
        {
            long traceType;
            wxString name, param;

            if( !file.GetNextLine().ToLong( &traceType ) )
            {
                EXPECTING( _( "expecting trace type" ) );
                file.Close();

                return false;
            }

            name = file.GetNextLine();

            if( name.IsEmpty() )
            {
                EXPECTING( _( "expecting trace name" ) );
                file.Close();

                return false;
            }

            param = file.GetNextLine();

            if( param.IsEmpty() )
            {
                EXPECTING( _( "expecting trace color" ) );
                file.Close();

                return false;
            }

            if( plotTab )
                traceInfo[plotTab].emplace_back( std::make_tuple( traceType, name, param ) );
        }

        if( version > 4 )
        {
            long measurementCount;

            if( !file.GetNextLine().ToLong( &measurementCount ) )
            {
                EXPECTING( _( "expecting measurement count" ) );
                file.Close();

                return false;
            }

            for( int ii = 0; ii < (int) measurementCount; ++ ii )
            {
                wxString measurement = file.GetNextLine();

                if( measurement.IsEmpty() )
                {
                    EXPECTING( _( "expecting measurement definition" ) );
                    file.Close();

                    return false;
                }

                wxString format = file.GetNextLine();

                if( format.IsEmpty() )
                {
                    EXPECTING( _( "expecting measurement format definition" ) );
                    file.Close();

                    return false;
                }

                if( plotTab )
                    plotTab->Measurements().emplace_back( measurement, format );
            }
        }
    }

    long userDefinedSignalCount;

    if( file.GetNextLine().ToLong( &userDefinedSignalCount ) )
    {
        for( int ii = 0; ii < (int) userDefinedSignalCount; ++ii )
            m_userDefinedSignals[ ii ] = file.GetNextLine();
    }

    for( const auto& [ plotTab, traceInfoVector ] : traceInfo )
    {
        for( const auto& [ traceType, signalName, param ] : traceInfoVector )
        {
            if( traceType == SPT_UNKNOWN && signalName == wxS( "$LEGEND" ) )
            {
                wxArrayString coords = wxSplit( param, ' ' );

                if( coords.size() >= 2 )
                {
                    long x = 0;
                    long y = 0;

                    coords[0].ToLong( &x );
                    coords[1].ToLong( &y );
                    plotTab->SetLegendPosition( wxPoint( (int) x, (int) y ) );
                }

                plotTab->ShowLegend( true );
            }
            else
            {
                wxString vectorName = vectorNameFromSignalName( plotTab, signalName, nullptr );
                TRACE*   trace = plotTab->GetOrAddTrace( vectorName, (int) traceType );

                if( version >= 4 && trace )
                    parseTraceParams( plotTab, trace, signalName, param );
            }
        }

        plotTab->UpdatePlotColors();
    }

    if( SIM_TAB* simTab = GetCurrentSimTab() )
    {
        m_simulatorFrame->LoadSimulator( simTab->GetSimCommand(), simTab->GetSimOptions() );

        if( version >= 5 )
        {
            simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( 0 ) );

            if( simTab )
                simTab->SetLastSchTextSimCommand( file.GetNextLine() );
        }
    }

    file.Close();
    return true;
}


