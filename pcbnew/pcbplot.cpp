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
 * @file pcbnew/pcbplot.cpp
 */

#include <fctsys.h>
#include <plot_common.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <pcbstruct.h>
#include <base_units.h>
#include <reporter.h>
#include <class_board.h>
#include <pcbnew.h>
#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <wx/ffile.h>
#include <dialog_plot.h>
#include <macros.h>


const wxString GetGerberExtension( LAYER_NUM aLayer )
{
    if( IsCopperLayer( aLayer ) )
    {
        if( aLayer == F_Cu )
            return wxT( "gtl" );
        else if( aLayer == B_Cu )
            return wxT( "gbl" );
        else
        {
            return wxT( "gbr" );
        }
    }
    else
    {
        switch( aLayer )
        {
        case B_Adhes:       return wxT( "gba" );
        case F_Adhes:       return wxT( "gta" );

        case B_Paste:       return wxT( "gbp" );
        case F_Paste:       return wxT( "gtp" );

        case B_SilkS:       return wxT( "gbo" );
        case F_SilkS:       return wxT( "gto" );

        case B_Mask:        return wxT( "gbs" );
        case F_Mask:        return wxT( "gts" );

        case Dwgs_User:
        case Cmts_User:
        case Eco1_User:
        case Eco2_User:
        case Edge_Cuts:
        default:            return wxT( "gbr" );
        }
    }
}


wxString GetGerberFileFunction( const BOARD *aBoard, LAYER_NUM aLayer )
{
    wxString attrib = wxEmptyString;

    switch( aLayer )
    {
    case F_Adhes:
        attrib = wxString( wxT( "Glue,Top" ) );
        break;

    case B_Adhes:
        attrib = wxString( wxT( "Glue,Bot" ) );
        break;

    case F_SilkS:
        attrib = wxString( wxT( "Legend,Top" ) );
        break;

    case B_SilkS:
        attrib = wxString( wxT( "Legend,Bot" ) );
        break;

    case F_Mask:
        attrib = wxString( wxT( "Soldermask,Top" ) );
        break;

    case B_Mask:
        attrib = wxString( wxT( "Soldermask,Bot" ) );
        break;

    case F_Paste:
        attrib = wxString( wxT( "Paste,Top" ) );
        break;

    case B_Paste:
        attrib = wxString( wxT( "Paste,Bot" ) );
        break;

    case Edge_Cuts:
        attrib = wxString( wxT( "Profile" ) );
        break;

    case Dwgs_User:
        attrib = wxString( wxT( "Drawing" ) );
        break;

    case Cmts_User:
        attrib = wxString( wxT( "Other,Comment" ) );
        break;

    case Eco1_User:
        attrib = wxString( wxT( "Other,ECO1" ) );
        break;

    case Eco2_User:
        attrib = wxString( wxT( "Other,ECO2" ) );
        break;

    case B_Cu:
        attrib = wxString::Format( wxT( "Copper,L%d" ), aBoard->GetCopperLayerCount() );
        break;

    case F_Cu:
    default:
        if( IsCopperLayer( aLayer ) )
        {
            attrib = wxString::Format( wxT( "Copper,L%d" ), aLayer+1 );
        }
        break;
    }

    // Add the signal type of the layer, if relevant
    if( IsCopperLayer( aLayer ) )
    {
        LAYER_T type = aBoard->GetLayerType( ToLAYER_ID( aLayer ) );

        switch( type )
        {
        case LT_SIGNAL:
            attrib += wxString( wxT( ",Signal" ) );
            break;
        case LT_POWER:
            attrib += wxString( wxT( ",Plane" ) );
            break;
        case LT_MIXED:
            attrib += wxString( wxT( ",Mixed" ) );
            break;
        default:
            ;   // do nothing (but avoid a warning for unhandled LAYER_T values from GCC)
        }
    }

    return attrib;
}


void BuildPlotFileName( wxFileName*     aFilename,
                        const wxString& aOutputDir,
                        const wxString& aSuffix,
                        const wxString& aExtension )
{
    aFilename->SetPath( aOutputDir );

    // Set the file extension
    aFilename->SetExt( aExtension );

    /* remove leading and trailing spaces if any from the suffix, if
       something survives add it to the name;
       also the suffix can contain some not allowed chars in filename (/ \ .),
       so change them to underscore
    */
    wxString suffix = aSuffix;
    suffix.Trim( true );
    suffix.Trim( false );

    suffix.Replace( wxT("."), wxT("_") );
    suffix.Replace( wxT("/"), wxT("_") );
    suffix.Replace( wxT("\\"), wxT("_") );

    if( !suffix.IsEmpty() )
        aFilename->SetName( aFilename->GetName() + wxT( "-" ) + suffix );
}


bool EnsureOutputDirectory( wxFileName*     aOutputDir,
                            const wxString& aBoardFilename,
                            REPORTER*       aReporter )
{
    wxString msg;
    wxString boardFilePath = wxFileName( aBoardFilename ).GetPath();

    if( !aOutputDir->MakeAbsolute( boardFilePath ) )
    {
        if( aReporter )
        {
            msg.Printf( _( "*** Error: cannot make path '%s' absolute with respect to '%s'! ***" ),
                        GetChars( aOutputDir->GetPath() ),
                        GetChars( boardFilePath ) );
            aReporter->Report( msg );
        }

        return false;
    }

    wxString outputPath( aOutputDir->GetPath() );

    if( !wxFileName::DirExists( outputPath ) )
    {
        if( wxMkdir( outputPath ) )
        {
            if( aReporter )
            {
                msg.Printf( _( "Output directory '%s' created.\n" ), GetChars( outputPath ) );
                aReporter->Report( msg );
                return true;
            }
        }
        else
        {
            if( aReporter )
            {
                msg.Printf( _( "*** Error: cannot create output directory '%s'! ***\n" ),
                            GetChars( outputPath ) );
                aReporter->Report( msg );
            }

            return false;
        }
    }

    return true;
}


PLOT_CONTROLLER::PLOT_CONTROLLER( BOARD *aBoard )
    : m_plotter( NULL ), m_board( aBoard )
{
}


PLOT_CONTROLLER::~PLOT_CONTROLLER()
{
    ClosePlot();
}


/* IMPORTANT THING TO KNOW: the locale during plots *MUST* be kept as
 * C/POSIX using a LOCALE_IO object on the stack. This even when
 * opening/closing the plotfile, since some drivers do I/O even then */

void PLOT_CONTROLLER::ClosePlot()
{
    LOCALE_IO toggle;

    if( m_plotter )
    {
        m_plotter->EndPlot();
        delete m_plotter;
        m_plotter = NULL;
    }
}


bool PLOT_CONTROLLER::OpenPlotfile( const wxString &aSuffix,
                                    PlotFormat     aFormat,
                                    const wxString &aSheetDesc )
{
    LOCALE_IO toggle;

    /* Save the current format: sadly some plot routines depends on this
       but the main reason is that the StartPlot method uses it to
       dispatch the plotter creation */
    m_plotOpts.SetFormat( aFormat );

    // Ensure that the previous plot is closed
    ClosePlot();

    // Now compute the full filename for the output and start the plot
    // (after ensuring the output directory is OK)
    wxString outputDirName = m_plotOpts.GetOutputDirectory() ;
    wxFileName outputDir = wxFileName::DirName( outputDirName );
    wxString boardFilename = m_board->GetFileName();

    if( EnsureOutputDirectory( &outputDir, boardFilename ) )
    {
        wxFileName fn( boardFilename );
        BuildPlotFileName( &fn, outputDirName, aSuffix, GetDefaultPlotExtension( aFormat ) );

        m_plotter = StartPlotBoard( m_board, &m_plotOpts, UNDEFINED_LAYER, fn.GetFullPath(), aSheetDesc );
    }

    return( m_plotter != NULL );
}


bool PLOT_CONTROLLER::PlotLayer( LAYER_NUM aLayer )
{
    LOCALE_IO toggle;

    // No plot open, nothing to do...
    if( !m_plotter )
        return false;

    // Fully delegated to the parent
    PlotOneBoardLayer( m_board, m_plotter, aLayer, m_plotOpts );

    return true;
}


void PLOT_CONTROLLER::SetColorMode( bool aColorMode )
{
    if( !m_plotter )
        return;

    m_plotter->SetColorMode( aColorMode );
}


bool PLOT_CONTROLLER::GetColorMode()
{
    if( !m_plotter )
        return false;

    return m_plotter->GetColorMode();
}
