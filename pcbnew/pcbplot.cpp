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


wxString GetGerberExtension( LAYER_NUM layer )
{
    switch( layer )
    {
    case LAYER_N_FRONT:
        return wxString( wxT( "gtl" ) );

    case LAYER_N_2:
    case LAYER_N_3:
    case LAYER_N_4:
    case LAYER_N_5:
    case LAYER_N_6:
    case LAYER_N_7:
    case LAYER_N_8:
    case LAYER_N_9:
    case LAYER_N_10:
    case LAYER_N_11:
    case LAYER_N_12:
    case LAYER_N_13:
    case LAYER_N_14:
    case LAYER_N_15:

        // TODO: see if we use .gbr or a layer identifier (gb1 .. gbnn ?)
        // according to the new internal layers designation
        // (1 is the first internal layer from the front layer)
        return wxString( wxT( "gbr" ) );

    case LAYER_N_BACK:
        return wxString( wxT( "gbl" ) );

    case ADHESIVE_N_BACK:
        return wxString( wxT( "gba" ) );

    case ADHESIVE_N_FRONT:
        return wxString( wxT( "gta" ) );

    case SOLDERPASTE_N_BACK:
        return wxString( wxT( "gbp" ) );

    case SOLDERPASTE_N_FRONT:
        return wxString( wxT( "gtp" ) );

    case SILKSCREEN_N_BACK:
        return wxString( wxT( "gbo" ) );

    case SILKSCREEN_N_FRONT:
        return wxString( wxT( "gto" ) );

    case SOLDERMASK_N_BACK:
        return wxString( wxT( "gbs" ) );

    case SOLDERMASK_N_FRONT:
        return wxString( wxT( "gts" ) );

    case DRAW_N:
    case COMMENT_N:
    case ECO1_N:
    case ECO2_N:
    case EDGE_N:
    default:
        return wxString( wxT( "gbr" ) );
    }
}


wxString GetGerberFileFunction( const BOARD *aBoard, LAYER_NUM aLayer )
{
    wxString attrib = wxEmptyString;

    switch( aLayer )
    {
    case LAYER_N_BACK:
        attrib = wxString::Format( wxT( "Copper,L%d" ), aBoard->GetCopperLayerCount() );
        break;

    case LAYER_N_2:
    case LAYER_N_3:
    case LAYER_N_4:
    case LAYER_N_5:
    case LAYER_N_6:
    case LAYER_N_7:
    case LAYER_N_8:
    case LAYER_N_9:
    case LAYER_N_10:
    case LAYER_N_11:
    case LAYER_N_12:
    case LAYER_N_13:
    case LAYER_N_14:
    case LAYER_N_15:
        // LAYER_N_2 is the first inner layer counting from the bottom; this
        // must be converted to a 1-based number starting from the top
        attrib = wxString::Format( wxT( "Copper,L%d" ),
                                   aBoard->GetCopperLayerCount() - ( aLayer - LAYER_N_2 + 1 ) );
        break;

    case LAYER_N_FRONT:
        attrib = wxString( wxT( "Copper,L1" ) );
        break;

    case ADHESIVE_N_FRONT:
        attrib = wxString( wxT( "Glue,Top" ) );
        break;

    case ADHESIVE_N_BACK:
        attrib = wxString( wxT( "Glue,Bot" ) );
        break;

    case SILKSCREEN_N_FRONT:
        attrib = wxString( wxT( "Legend,Top" ) );
        break;

    case SILKSCREEN_N_BACK:
        attrib = wxString( wxT( "Legend,Bot" ) );
        break;

    case SOLDERMASK_N_FRONT:
        attrib = wxString( wxT( "Soldermask,Top" ) );
        break;

    case SOLDERMASK_N_BACK:
        attrib = wxString( wxT( "Soldermask,Bot" ) );
        break;

    case SOLDERPASTE_N_FRONT:
        attrib = wxString( wxT( "Paste,Top" ) );
        break;

    case SOLDERPASTE_N_BACK:
        attrib = wxString( wxT( "Paste,Bot" ) );
        break;

    case EDGE_N:
        attrib = wxString( wxT( "Profile" ) );
        break;

    case DRAW_N:
        attrib = wxString( wxT( "Drawing" ) );
        break;

    case COMMENT_N:
        attrib = wxString( wxT( "Other,Comment" ) );
        break;

    case ECO1_N:
    case ECO2_N:
        attrib = wxString::Format( wxT( "Other,ECO%d" ), aLayer - ECO1_N + 1 );
        break;
    }

    // Add the signal type of the layer, if relevant
    if( FIRST_COPPER_LAYER <= aLayer && aLayer <= LAST_COPPER_LAYER ) {
        LAYER_T type = aBoard->GetLayerType( aLayer );
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
