/**
 * @file pcbnew/pcbplot.cpp
 */

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

#include <fctsys.h>
#include <plot_common.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <pcbstruct.h>
#include <base_units.h>
#include <class_board.h>
#include <pcbnew.h>
#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <wx/ffile.h>
#include <dialog_plot.h>


/** Get the 'traditional' gerber extension depending on the layer
*/
static wxString GetGerberExtension( int layer )/*{{{*/
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
}/*}}}*/

/* Complete a plot filename: forces the output directory,
 * add a suffix to the name and sets the specified extension
 * the suffix is usually the layer name
 * replaces not allowed chars in suffix by '_'
 */
void BuildPlotFileName( wxFileName *aFilename,
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

/*
 * Fix the output directory pathname to absolute and ensure it exists
 * (Creates it if not exists)
 */
bool EnsureOutputDirectory( wxFileName *aOutputDir,
                            const wxString& aBoardFilename,
                            wxTextCtrl* aMessageBox )
{
    wxString boardFilePath = wxFileName( aBoardFilename ).GetPath();

    if( !aOutputDir->MakeAbsolute( boardFilePath ) )
    {
        wxString msg;
        msg.Printf( _( "Cannot make %s absolute with respect to %s!" ),
                    GetChars( aOutputDir->GetPath() ),
                    GetChars( boardFilePath ) );
        wxMessageBox( msg, _( "Plot" ), wxOK | wxICON_ERROR );
        return false;
    }

    wxString outputPath( aOutputDir->GetPath() );
    if( !wxFileName::DirExists( outputPath ) )
    {
        if( wxMkdir( outputPath ) )
        {
            if( aMessageBox )
            {
                wxString msg;
                msg.Printf( _( "Directory %s created.\n" ), GetChars( outputPath ) );
                aMessageBox->AppendText( msg );
                    return true;
            }
        }
        else
        {
            if( aMessageBox )
                wxMessageBox( _( "Cannot create output directory!" ),
                              _( "Plot" ), wxOK | wxICON_ERROR );
            return false;
        }
    }
    return true;
}

/*
 * DIALOG_PLOT:Plot
 * Actually creates the files
 */
void DIALOG_PLOT::Plot( wxCommandEvent& event )
{
    int        layer;

    applyPlotSettings();

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString boardFilename = m_parent->GetBoard()->GetFileName();
    if( !EnsureOutputDirectory( &outputDir, boardFilename, m_messagesBox ) )
        return;

    m_plotOpts.SetAutoScale( false );
    m_plotOpts.SetScale( 1 );
    switch( m_plotOpts.GetScaleSelection() )
    {
    default:
        break;

    case 0:     // Autoscale option
        m_plotOpts.SetAutoScale( true );
        break;

    case 2:     // 3:2 option
        m_plotOpts.SetScale( 1.5 );
        break;

    case 3:     // 2:1 option
        m_plotOpts.SetScale( 2 );
        break;

    case 4:     // 3:1 option
        m_plotOpts.SetScale( 3 );
        break;
    }

    /* If the scale factor edit controls are disabled or the scale value
     * is 0, don't adjust the base scale factor.   This fixes a bug when
     * the default scale adjust is initialized to 0 and saved in program
     * settings resulting in a divide by zero fault.
     */
    if( m_fineAdjustXscaleOpt->IsEnabled()  && m_XScaleAdjust != 0.0 )
        m_plotOpts.SetFineScaleAdjustX( m_XScaleAdjust );

    if( m_fineAdjustYscaleOpt->IsEnabled() && m_YScaleAdjust != 0.0 )
        m_plotOpts.SetFineScaleAdjustY( m_YScaleAdjust );

    if( m_PSFineAdjustWidthOpt->IsEnabled() )
        m_plotOpts.SetWidthAdjust( m_PSWidthAdjust );

    wxString file_ext( GetDefaultPlotExtension( m_plotOpts.GetFormat() ) );

    // Test for a reasonable scale value
    // XXX could this actually happen? isn't it constrained in the apply
    // function?
    if( m_plotOpts.GetScale() < PLOT_MIN_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very small value" ) );

    if( m_plotOpts.GetScale() > PLOT_MAX_SCALE )
        DisplayInfoMessage( this,
                            _( "Warning: Scale option set to a very large value" ) );

    // Save the current plot options in the board
    m_parent->SetPlotSettings( m_plotOpts );

    long layerMask = 1;

    for( layer = 0; layer < NB_LAYERS; layer++, layerMask <<= 1 )
    {
        if( m_plotOpts.GetLayerSelection() & layerMask )
        {
            // Pick the basename from the board file
            wxFileName fn( boardFilename );

            // Use Gerber Extensions based on layer number
            // (See http://en.wikipedia.org/wiki/Gerber_File)
            if( ( m_plotOpts.GetFormat() == PLOT_FORMAT_GERBER )
                && m_useGerberExtensions->GetValue() )
                file_ext = GetGerberExtension( layer );

            // Create file name (from the English layer name for non copper layers).
            BuildPlotFileName( &fn, outputDir.GetPath(),
                               m_board->GetLayerName( layer, false ),
                               file_ext );

            LOCALE_IO toggle;
            BOARD *board = m_parent->GetBoard();
            PLOTTER *plotter = StartPlotBoard(board, &m_plotOpts,
                                              fn.GetFullPath(),
                                              wxEmptyString );

            // Print diags in messages box:
            wxString msg;
            if( plotter )
            {
                PlotOneBoardLayer( board, plotter, layer, m_plotOpts );
                plotter->EndPlot();
                delete plotter;

                msg.Printf( _( "Plot file <%s> created" ), GetChars( fn.GetFullPath() ) );
            }
            else
                msg.Printf( _( "Unable to create <%s>" ), GetChars( fn.GetFullPath() ) );

            msg << wxT( "\n" );
            m_messagesBox->AppendText( msg );
        }
    }

    // If no layer selected, we have nothing plotted.
    // Prompt user if it happens because he could think there is a bug in Pcbnew.
    if( !m_plotOpts.GetLayerSelection() )
        DisplayError( this, _( "No layer selected" ) );
}

void PCB_EDIT_FRAME::ToPlotter( wxCommandEvent& event )
{
    DIALOG_PLOT dlg( this );
    dlg.ShowModal();
}

/** Batch plotter constructor, nothing interesting here */
PLOT_CONTROLLER::PLOT_CONTROLLER( BOARD *aBoard )
    : m_plotter( NULL ), m_board( aBoard )
{
}

/** Batch plotter destructor, ensures that the last plot is closed */
PLOT_CONTROLLER::~PLOT_CONTROLLER()
{
    ClosePlot();
}

/* IMPORTANT THING TO KNOW: the locale during plots *MUST* be kept as
 * C/POSIX using a LOCALE_IO object on the stack. This even when
 * opening/closing the plotfile, since some drivers do I/O even then */

/** Close the current plot, nothing happens if it isn't open */
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

/** Open a new plotfile; works as a factory for plotter objects
 */
bool PLOT_CONTROLLER::OpenPlotfile( const wxString &aSuffix, /*{{{*/
                                    PlotFormat aFormat,
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
    if( EnsureOutputDirectory( &outputDir, boardFilename, NULL ) )
    {
        wxFileName fn( boardFilename );
        BuildPlotFileName( &fn, outputDirName,
                aSuffix, GetDefaultPlotExtension( aFormat ) );

        m_plotter = StartPlotBoard( m_board, &m_plotOpts, fn.GetFullPath(),
                                    aSheetDesc );
    }
    return( m_plotter != NULL );
}/*}}}*/

/** Plot a single layer on the current plotfile */
bool PLOT_CONTROLLER::PlotLayer( int aLayer )/*{{{*/
{
    LOCALE_IO toggle;

    // No plot open, nothing to do...
    if( !m_plotter )
        return false;

    // Fully delegated to the parent
    PlotOneBoardLayer( m_board, m_plotter, aLayer, m_plotOpts );

    return true;
}/*}}}*/

