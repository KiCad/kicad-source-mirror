/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <build_version.h>


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
            return wxString::Format( wxT( "g%d" ), aLayer+1 );
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

        case Edge_Cuts:     return wxT( "gm1" );

        case Dwgs_User:
        case Cmts_User:
        case Eco1_User:
        case Eco2_User:
        default:            return wxT( "gbr" );
        }
    }
}


wxString GetGerberFileFunctionAttribute( const BOARD *aBoard,
                LAYER_NUM aLayer, bool aUseX1CompatibilityMode )
{
    wxString attrib;

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
        // Board outline.
        // Can be "Profile,NP" (Not Plated: usual) or "Profile,P"
        // This last is the exception (Plated)
        attrib = wxString( wxT( "Profile,NP" ) );
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

    case B_Fab:
        attrib = wxString( wxT( "Other,Fab,Bot" ) );
        break;

    case F_Fab:
        attrib = wxString( wxT( "Other,Fab,Top" ) );
        break;

    case B_Cu:
        attrib = wxString::Format( wxT( "Copper,L%d,Bot" ), aBoard->GetCopperLayerCount() );
        break;

    case F_Cu:
        attrib = wxString::Format( wxT( "Copper,L1,Top" ) );
        break;

    default:
        if( IsCopperLayer( aLayer ) )
            attrib = wxString::Format( wxT( "Copper,L%d,Inr" ), aLayer+1 );
        else
            attrib = wxString::Format( wxT( "Other,User" ), aLayer+1 );
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
            break;   // do nothing (but avoid a warning for unhandled LAYER_T values from GCC)
        }
    }

    wxString fileFct;

    if( aUseX1CompatibilityMode )
        fileFct.Printf( "G04 #@! TF.FileFunction,%s*", GetChars( attrib ) );
    else
        fileFct.Printf( "%%TF.FileFunction,%s*%%", GetChars( attrib ) );

    return fileFct;
}

/* Add some X2 attributes to the file header, as defined in the
 * Gerber file format specification J4 and J5
 */
#define USE_J5_ATTR
void AddGerberX2Attribute( PLOTTER * aPlotter,
            const BOARD *aBoard, LAYER_NUM aLayer )
{
    wxString text;

#ifdef USE_J5_ATTR
    // Creates the TF,.GenerationSoftware. Format is:
    // %TF,.GenerationSoftware,<vendor>,<application name>[,<application version>]*%
    text.Printf( wxT( "%%TF.GenerationSoftware,KiCad,Pcbnew,%s*%%" ), GetBuildVersion() );
    aPlotter->AddLineToHeader( text );

    // creates the TF.CreationDate ext:
    // The attribute value must conform to the full version of the ISO 8601
    // date and time format, including time and time zone. Note that this is
    // the date the Gerber file was effectively created,
    // not the time the project of PCB was started
    wxDateTime date( wxDateTime::GetTimeNow() );
    // Date format: see http://www.cplusplus.com/reference/ctime/strftime
    wxString msg = date.Format( wxT( "%z" ) );  // Extract the time zone offset
    // The time zone offset format is + (or -) mm or hhmm  (mm = number of minutes, hh = number of hours)
    // we want +(or -) hh:mm
    if( msg.Len() > 3 )
        msg.insert( 3, ":", 1 ),
    text.Printf( wxT( "%%TF.CreationDate,%s%s*%%" ), GetChars( date.FormatISOCombined() ), GetChars( msg ) );
    aPlotter->AddLineToHeader( text );

    // Creates the TF,.JobID. Format is (from Gerber file format doc):
    // %TF.JobID,<project id>,<project GUID>,<revision id>*%
    // <project id> is the name of the project, restricted to basic ASCII symbols only,
    // and comma not accepted
    // All illegal chars will be replaced by underscore
    // <project GUID> is a 32 hexadecimal digits string which is an unique id of a project.
    // This is a random 128-bit number expressed in 32 hexadecimal digits.
    // See en.wikipedia.org/wiki/GUID for more information
    // However Kicad does not handle such a project GUID, so it is built from the board name
    // Rem: <project id> accepts only ASCII 7 code (only basic ASCII codes are allowed in gerber files).
    wxFileName fn = aBoard->GetFileName();
    msg = fn.GetFullName();
    wxString guid;

    // Build a 32 digits GUID from the board name:
    for( unsigned ii = 0; ii < msg.Len(); ii++ )
    {
        int cc1 = int( msg[ii] ) & 0x0F;
        int cc2 = ( int( msg[ii] ) >> 4) & 0x0F;
        guid << wxString::Format( wxT( "%X%X" ), cc2, cc1 );

        if( guid.Len() >= 32 )
            break;
    }

    // guid has 32 digits, so add missing digits
    int cnt = 32 - guid.Len();

    if( cnt > 0 )
        guid.Append( '0', cnt );

    // build the <project id> string: this is the board short filename (without ext)
    // and all non ASCII chars and comma are replaced by '_'
    msg = fn.GetName();
    msg.Replace( wxT( "," ), wxT( "_" ) );

    // build the <rec> string. All non ASCII chars and comma are replaced by '_'
    wxString rev = ((BOARD*)aBoard)->GetTitleBlock().GetRevision();
    rev.Replace( wxT( "," ), wxT( "_" ) );

    if( rev.IsEmpty() )
        rev = wxT( "rev?" );

    text.Printf( wxT( "%%TF.JobID,%s,%s,%s*%%" ), msg.ToAscii(), GetChars( guid ), rev.ToAscii() );
    aPlotter->AddLineToHeader( text );
#endif

    // Add the TF.FileFunction
    text = GetGerberFileFunctionAttribute( aBoard, aLayer, false );
    aPlotter->AddLineToHeader( text );
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


PLOT_CONTROLLER::PLOT_CONTROLLER( BOARD *aBoard )
{
    m_plotter = NULL;
    m_board = aBoard;
    m_plotLayer = UNDEFINED_LAYER;
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
    GetPlotOptions().SetFormat( aFormat );

    // Ensure that the previous plot is closed
    ClosePlot();

    // Now compute the full filename for the output and start the plot
    // (after ensuring the output directory is OK)
    wxString outputDirName = GetPlotOptions().GetOutputDirectory() ;
    wxFileName outputDir = wxFileName::DirName( outputDirName );
    wxString boardFilename = m_board->GetFileName();

    if( EnsureFileDirectoryExists( &outputDir, boardFilename ) )
    {
        wxFileName fn( boardFilename );
        BuildPlotFileName( &fn, outputDirName, aSuffix, GetDefaultPlotExtension( aFormat ) );

        m_plotter = StartPlotBoard( m_board, &GetPlotOptions(), ToLAYER_ID( GetLayer() ), fn.GetFullPath(), aSheetDesc );
    }

    return( m_plotter != NULL );
}


bool PLOT_CONTROLLER::PlotLayer()
{
    LOCALE_IO toggle;

    // No plot open, nothing to do...
    if( !m_plotter )
        return false;

    // Fully delegated to the parent
    PlotOneBoardLayer( m_board, m_plotter, ToLAYER_ID( GetLayer() ), GetPlotOptions() );

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
