/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <sstream>
#include <iostream>
#include <sstream>

#include "kicad2step.h"
#include "pcb/kicadpcb.h"
#include "kicad2step_frame_base.h"
#include "panel_kicad2step.h"
#include <Standard_Failure.hxx>     // In open cascade

class KICAD2STEP_FRAME : public KICAD2STEP_FRAME_BASE
{
public:
    KICAD2STEP_FRAME( const wxString& title );
};

// Horrible hack until we decouple things more
static PANEL_KICAD2STEP* openPanel = nullptr;
void ReportMessage( const wxString& aMessage )
{
    if( openPanel != nullptr )
        openPanel->AppendMessage( aMessage );
}


KICAD2MCAD_PRMS::KICAD2MCAD_PRMS()
{
#ifdef SUPPORTS_IGES
    m_fmtIGES = false;
#endif
    m_overwrite = false;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_includeVirtual = true;
    m_substModels = false;
    m_xOrigin = 0.0;
    m_yOrigin = 0.0;
    m_minDistance = MIN_DISTANCE;

}


KICAD2STEP_FRAME::KICAD2STEP_FRAME( const wxString& title ) :
        KICAD2STEP_FRAME_BASE( NULL, wxID_ANY, title )
{
}

PANEL_KICAD2STEP::PANEL_KICAD2STEP( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style ) :
        wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	m_tcMessages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY );
	bSizer->Add( m_tcMessages, 1, wxALL | wxEXPAND, 5 );

	SetSizer( bSizer );
	Layout();
	bSizer->Fit( this );
}


void PANEL_KICAD2STEP::AppendMessage( const wxString& aMessage )
{
    m_tcMessages->AppendText( aMessage );
    wxSafeYield();
}


// Smart class that will swap streambufs and replace them when object goes out of scope.
// ( ensure the initial stream buffer is restored )
// see:
// https://groups.google.com/forum/#!topic/borland.public.cppbuilder.language/Uua6t3VhELA
// It is useful here to redirect for instance cout or cerr to a string stream
class STREAMBUF_SWAPPER
{
public:
    STREAMBUF_SWAPPER( std::ostream& orig, std::ostream& replacement ) :
            m_buf( orig.rdbuf() ),
            m_str( orig )
    {
        orig.rdbuf( replacement.rdbuf() );
    }

    ~STREAMBUF_SWAPPER()
    {
        m_str.rdbuf( m_buf );
    }

private:
    std::streambuf* m_buf;
    std::ostream& m_str;
};


int PANEL_KICAD2STEP::RunConverter()
{
    wxFileName fname( m_params.m_filename );

    if( !fname.FileExists() )
    {
        wxMessageBox( wxString::Format( _( "No such file: %s" ), m_params.m_filename ) );
        return -1;
    }

    wxFileName out_fname;

    if( m_params.m_outputFile.empty() )
    {
        out_fname.Assign( fname.GetFullPath() );
        out_fname.SetExt( m_params.getOutputExt() );
    }
    else
    {
        out_fname.Assign( m_params.m_outputFile );

        // Set the file extension if the user's requested
        // file name does not have an extension.
        if( !out_fname.HasExt() )
            out_fname.SetExt( m_params.getOutputExt() );
    }

    if( out_fname.FileExists() && !m_params.m_overwrite )
    {
        ReportMessage( _( "** Output already exists. Export aborted. **\n"
                          "Enable the force overwrite flag to overwrite it." ) );

        return -1;
    }

    wxString outfile = out_fname.GetFullPath();
    KICADPCB pcb;

    pcb.SetOrigin( m_params.m_xOrigin, m_params.m_yOrigin );
    pcb.SetMinDistance( m_params.m_minDistance );
    ReportMessage( wxString::Format( _( "Read file: '%s'\n" ), m_params.m_filename ) );

    // create the new streams to "redirect" cout and cerr output to
    // msgs_from_opencascade and errors_from_opencascade
    std::ostringstream msgs_from_opencascade;
    std::ostringstream errors_from_opencascade;
    STREAMBUF_SWAPPER  swapper_cout( std::cout, msgs_from_opencascade );
    STREAMBUF_SWAPPER  swapper_cerr( std::cerr, errors_from_opencascade );

    if( pcb.ReadFile( m_params.m_filename ) )
    {
        if( m_params.m_useDrillOrigin )
            pcb.UseDrillOrigin( true );

        if( m_params.m_useGridOrigin )
            pcb.UseGridOrigin( true );

        bool res;

        try
        {
            ReportMessage( _( "Build STEP data\n" ) );

            res = pcb.ComposePCB( m_params.m_includeVirtual, m_params.m_substModels );

            if( !res )
            {
                ReportMessage( _( "\n** Error building STEP board model. Export aborted. **\n" ) );
                return -1;
            }

            ReportMessage( _( "Write STEP file\n" ) );

#ifdef SUPPORTS_IGES
            if( m_fmtIGES )
                res = pcb.WriteIGES( outfile );
            else
#endif
                res = pcb.WriteSTEP( outfile );

            if( !res )
            {
                ReportMessage( _( "\n** Error writing STEP file. **\n" ) );
                return -1;
            }
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( e.GetMessageString() );
            ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
            return -1;
        }
        catch( ... )
        {
            ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
            return -1;
        }
    }

    wxString msgs, errs;
    msgs << msgs_from_opencascade.str();
    ReportMessage( msgs );

    ReportMessage( wxString::Format( _( "\nSTEP file '%s' created.\n" ), outfile ) );

    errs << errors_from_opencascade.str();
    ReportMessage( errs );

    // Check the output log for an indication of success
    bool success =  msgs.Contains( "Done" );
    wxString msg;

    if( !errs.IsEmpty() )    // Any troubles?
    {
        if( !success )
        {
            msg = _( "Unable to create STEP file.\n"
                     "Check that the board has a valid outline and models." );
        }
        else
        {
            msg = _( "STEP file has been created, but there are warnings." );
        }
    }

    ReportMessage( msg );

    return 0;
}


wxString KICAD2MCAD_PRMS::getOutputExt() const
{
#ifdef SUPPORTS_IGES
    if( m_fmtIGES )
        return wxString( "igs" );
    else
#endif
        return wxString( "step" );
}


KICAD2STEP::KICAD2STEP( KICAD2MCAD_PRMS aParams ) : m_params( aParams ), m_panel( nullptr )
{
}


int KICAD2STEP::Run()
{
    // create the main application window
    KICAD2STEP_FRAME* frame = new KICAD2STEP_FRAME( "Kicad2step" );

    m_panel = frame->m_panelKicad2Step;
    m_panel->m_params = m_params;

    // and show it (a wxFrame is not shown when created initially)
    frame->Show( true );
    frame->Iconize( false );

    openPanel = m_panel;

    int diag = m_panel->RunConverter();

    openPanel = nullptr;

    return diag;
}


void KICAD2STEP::ReportMessage( const wxString& aMessage )
{
    m_panel->AppendMessage( aMessage );
}