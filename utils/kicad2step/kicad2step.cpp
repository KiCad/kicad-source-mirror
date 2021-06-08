/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb/kicadpcb.h"
#include "kicad2step_frame_base.h"
#include "panel_kicad2step.h"
#include <Standard_Failure.hxx>     // In open cascade

class KICAD2STEP_FRAME;

class KICAD2MCAD_APP : public wxApp
{
public:
    KICAD2MCAD_APP() : wxApp(), m_frame( nullptr ), m_Panel( nullptr )
    {}
    virtual bool OnInit() override;
    virtual int OnRun() override;
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
    KICAD2STEP_FRAME * m_frame;
    KICAD2MCAD_PRMS m_params;

public:
    PANEL_KICAD2STEP* m_Panel;
};

wxIMPLEMENT_APP(KICAD2MCAD_APP);

class KICAD2STEP_FRAME : public KICAD2STEP_FRAME_BASE
{
public:
    KICAD2STEP_FRAME(const wxString& title);

private:
};

KICAD2MCAD_PRMS::KICAD2MCAD_PRMS()
{
#ifdef SUPPORTS_IGES
    m_fmtIGES = false;
#endif
    m_overwrite = false;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_includeVirtual = true;
    m_xOrigin = 0.0;
    m_yOrigin = 0.0;
    m_minDistance = MIN_DISTANCE;

}


void ReportMessage( const wxString& aMessage )
{
    KICAD2MCAD_APP& app = wxGetApp();
    app.m_Panel->AppendMessage( aMessage );
}


static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
        { wxCMD_LINE_PARAM, NULL, NULL, _( "pcb_filename" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
        { wxCMD_LINE_OPTION, "o", "output-filename", _( "output filename" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
#ifdef SUPPORTS_IGES
        { wxCMD_LINE_SWITCH, "fmt-iges", NULL, _("IGES output (default STEP)").mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
#endif
        { wxCMD_LINE_SWITCH, "f", "force", _( "overwrite output file" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, NULL, "drill-origin", _( "Use Drill Origin for output origin" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, NULL, "grid-origin", _( "Use Grid Origin for output origin" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, NULL, "user-origin",
            _( "User-specified output origin ex. 1x1in, 1x1inch, 25.4x25.4mm (default mm)" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, NULL, "no-virtual",
            _( "exclude 3D models for components with 'virtual' attribute" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, NULL, "min-distance",
            _( "Minimum distance between points to treat them as separate ones (default 0.01 mm)" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, "h", NULL, _( "display this message" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
    };


bool KICAD2MCAD_APP::OnInit()
{
    if( !wxApp::OnInit() )
        return false;

    // create the main application window
    m_frame = new KICAD2STEP_FRAME( "Kicad2step" );

    m_Panel = m_frame->m_panelKicad2Step;
    m_Panel->m_params = m_params;

    // and show it (a wxFrame is not shown when created initially)
    m_frame->Show( true );
    m_frame->Iconize( false );

    return true;
}


int KICAD2MCAD_APP::OnRun()
{
    int diag = m_Panel->RunConverter();
    wxApp::OnRun();     // Start the main loop event, to manage the main frame

    return diag;
}


KICAD2STEP_FRAME::KICAD2STEP_FRAME(const wxString& title)
       : KICAD2STEP_FRAME_BASE(NULL, wxID_ANY, title)
{
}


void KICAD2MCAD_APP::OnInitCmdLine( wxCmdLineParser& parser )
{
    parser.SetDesc( cmdLineDesc );
    parser.SetSwitchChars( "-" );
    return;
}


PANEL_KICAD2STEP::PANEL_KICAD2STEP( wxWindow* parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size, long style ):
                wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );

	m_tcMessages = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	bSizer->Add( m_tcMessages, 1, wxALL|wxEXPAND, 5 );

	SetSizer( bSizer );
	Layout();
	bSizer->Fit( this );
}


void PANEL_KICAD2STEP::AppendMessage( const wxString& aMessage )
{
    m_tcMessages->AppendText( aMessage );
    wxSafeYield();
}


bool KICAD2MCAD_APP::OnCmdLineParsed( wxCmdLineParser& parser )
{
    #ifdef SUPPORTS_IGES
      if( parser.Found( "fmt-iges" ) )
        m_fmtIGES = true;
    #endif

    if( parser.Found( "f" ) )
        m_params.m_overwrite = true;

    if( parser.Found( "grid-origin" ) )
        m_params.m_useGridOrigin = true;

    if( parser.Found( "drill-origin" ) )
        m_params. m_useDrillOrigin = true;

    if( parser.Found( "no-virtual" ) )
        m_params.m_includeVirtual = false;

    wxString tstr;

    if( parser.Found( "user-origin", &tstr ) )
    {
        std::istringstream istr;
        istr.str( std::string( tstr.ToUTF8() ) );
        istr >> m_params.m_xOrigin;

        if( istr.fail() )
        {
            parser.Usage();
            return false;
        }

        char tmpc;
        istr >> tmpc;

        if( istr.fail() || ( tmpc != 'x' && tmpc != 'X' ) )
        {
            parser.Usage();
            return false;
        }

        istr >> m_params.m_yOrigin;

        if( istr.fail() )
        {
            parser.Usage();
            return false;
        }

        if( !istr.eof() )
        {
            std::string tunit;
            istr >> tunit;

            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                m_params.m_xOrigin *= 25.4;
                m_params.m_yOrigin *= 25.4;
            }
            else if( tunit.compare( "mm" ) )
            {
                parser.Usage();
                return false;
            }
        }
    }


    if( parser.Found( "min-distance", &tstr ) )
    {
        std::istringstream istr;
        istr.str( std::string( tstr.ToUTF8() ) );
        istr >> m_params.m_minDistance;

        if( istr.fail() )
        {
            parser.Usage();
            return false;
        }

        if( !istr.eof() )
        {
            std::string tunit;
            istr >> tunit;

            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                m_params.m_minDistance *= 25.4;
            }
            else if( tunit.compare( "mm" ) )
            {
                parser.Usage();
                return false;
            }
        }
    }

    if( parser.Found( "o", &tstr ) )
        m_params.m_outputFile = tstr;


    if( parser.GetParamCount() < 1 )
    {
        parser.Usage();
        return false;
    }

    m_params.m_filename = parser.GetParam( 0 );

    return true;
}


// Smart class that will swap streambufs and replace them when object goes out of scope.
// ( ensure the initial stream buffer is restored )
// see:
// https://groups.google.com/forum/#!topic/borland.public.cppbuilder.language/Uua6t3VhELA
// It is useful here to redirect for instance cout or cerr to a string stream
class STREAMBUF_SWAPPER
{
public:
    STREAMBUF_SWAPPER( std::ostream & orig, std::ostream & replacement )
        : m_buf( orig.rdbuf() ), m_str( orig )
    {
        orig.rdbuf( replacement.rdbuf() );
    }

    ~STREAMBUF_SWAPPER()
    {
        m_str.rdbuf( m_buf);
    }

private:
    std::streambuf * m_buf;
    std::ostream & m_str;
};


int PANEL_KICAD2STEP::RunConverter()
{
    wxFileName fname( m_params.m_filename );

    if( !fname.FileExists() )
    {
        wxMessageBox( wxString::Format( "No such file: %s", m_params.m_filename ) );
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
        ReportMessage( "** Output already exists.\n"
                      "Enable the force overwrite flag to overwrite it." );

        return -1;
    }

    wxString outfile = out_fname.GetFullPath();
    KICADPCB pcb;

    pcb.SetOrigin( m_params.m_xOrigin, m_params.m_yOrigin );
    pcb.SetMinDistance( m_params.m_minDistance );
    ReportMessage( wxString::Format( "Read: %s\n", m_params.m_filename ) );

    // create the new streams to "redirect" cout and cerr output to
    // msgs_from_opencascade and errors_from_opencascade
    std::ostringstream msgs_from_opencascade;
    std::ostringstream errors_from_opencascade;
    STREAMBUF_SWAPPER swapper_cout(std::cout, msgs_from_opencascade);
    STREAMBUF_SWAPPER swapper_cerr(std::cerr, errors_from_opencascade);

    if( pcb.ReadFile( m_params.m_filename ) )
    {
        if( m_params.m_useDrillOrigin )
            pcb.UseDrillOrigin( true );

        if( m_params.m_useGridOrigin )
            pcb.UseGridOrigin( true );

        bool res;

        try
        {
            ReportMessage( "Build STEP data\n" );

            res = pcb.ComposePCB( m_params.m_includeVirtual );

            if( !res )
            {
                ReportMessage( "\n**Error building STEP board model. Abort export **\n" );
                return -1;
            }

            ReportMessage( "Write STEP file\n" );

        #ifdef SUPPORTS_IGES
            if( m_fmtIGES )
                res = pcb.WriteIGES( outfile );
            else
        #endif
                res = pcb.WriteSTEP( outfile );

            if( !res )
            {
                ReportMessage( "\nError Write STEP file\n" );
                return -1;
            }
        }
        catch( const Standard_Failure& e )
        {
            wxString err = e.GetMessageString();
            wxMessageBox( err, "Export Error" );

            ReportMessage( wxString::Format( "\nExport Error: %s\n", err ) );
            ReportMessage( "\n*** Abort export ***\n" );
            return -1;
        }
        catch( ... )
        {
            wxMessageBox( "(no exception information)", "Unknown error" );
            ReportMessage( "\nUnknown error\n*** Abort export ***\n" );
            return -1;
        }
    }

    wxString msgs, errs;
    msgs << msgs_from_opencascade.str();
    ReportMessage( msgs );

    ReportMessage( wxString::Format( "\nStep file %s created\n\n", outfile ) );

    errs << errors_from_opencascade.str();
    ReportMessage( errs );

    // Check the output log for an indication of success
    bool success =  msgs.Contains( "Done" );
    wxString msg;

    if( !errs.IsEmpty() )    // Any troubles?
    {
        if( !success )
            msg = "Unable to create STEP file.\n"
                  "Check that the board has a valid outline and models.";
        else
        {
            msg = "STEP file has been created, but there are warnings.";
        }
    }
    else    // No error messages: the file is expected OK
    {
        msg.Printf( "STEP file:\n%s\nhas been created successfully.", outfile );
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
