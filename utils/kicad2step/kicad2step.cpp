/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <wx/string.h>
#include <wx/filename.h>
#include <sstream>
#include <iostream>
#include <sstream>
#include <Standard_Failure.hxx>

#include "kicadpcb.h"

class KICAD2MCAD : public wxAppConsole
{
public:
    virtual bool OnInit() override;
    virtual int OnRun() override;
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
#ifdef SUPPORTS_IGES
    bool     m_fmtIGES;
#endif
    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeVirtual;
    wxString m_filename;
    wxString m_outputFile;
    double   m_xOrigin;
    double   m_yOrigin;
    bool     m_inch;
};

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
        { wxCMD_LINE_SWITCH, "h", NULL, _( "display this message" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_NONE }
    };


wxIMPLEMENT_APP_CONSOLE( KICAD2MCAD );


bool KICAD2MCAD::OnInit()
{
#ifdef SUPPORTS_IGES
    m_fmtIGES = false;
#endif
    m_overwrite = false;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_includeVirtual = true;
    m_inch = false;
    m_xOrigin = 0.0;
    m_yOrigin = 0.0;

    if( !wxAppConsole::OnInit() )
        return false;

    return true;
}


void KICAD2MCAD::OnInitCmdLine( wxCmdLineParser& parser )
{
    parser.SetDesc( cmdLineDesc );
    parser.SetSwitchChars( "-" );
    return;
}


bool KICAD2MCAD::OnCmdLineParsed( wxCmdLineParser& parser )
{
    #ifdef SUPPORTS_IGES
      if( parser.Found( "fmt-iges" ) )
        m_fmtIGES = true;
    #endif

    if( parser.Found( "f" ) )
        m_overwrite = true;

    if( parser.Found( "grid-origin" ) )
        m_useGridOrigin = true;

    if( parser.Found( "drill-origin" ) )
        m_useDrillOrigin = true;

    if( parser.Found( "no-virtual" ) )
        m_includeVirtual = false;

    wxString tstr;

    if( parser.Found( "user-origin", &tstr ) )
    {
        std::istringstream istr;
        istr.str( std::string( tstr.ToUTF8() ) );
        istr >> m_xOrigin;

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

        istr >> m_yOrigin;

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
                m_inch = true;
            }
            else if( tunit.compare( "mm" ) )
            {
                parser.Usage();
                return false;
            }
        }
    }

    if( parser.Found( "o", &tstr ) )
        m_outputFile = tstr;


    if( parser.GetParamCount() < 1 )
    {
        parser.Usage();
        return false;
    }

    m_filename = parser.GetParam( 0 );

    return true;
}


int KICAD2MCAD::OnRun()
{
    wxFileName fname( m_filename );

    if( !fname.FileExists() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << "  * no such file: '" << m_filename.ToUTF8() << "'\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );

        return -1;
    }

    wxFileName tfname;

    if( m_outputFile.empty() )
        tfname.Assign( fname.GetFullPath() );
    else
        tfname.Assign( m_outputFile );

#ifdef SUPPORTS_IGES
    if( m_fmtIGES )
        tfname.SetExt( "igs" );
    else
#endif
        tfname.SetExt( "stp" );

    wxString outfile = tfname.GetFullPath();

    KICADPCB pcb;

    if( m_inch )
        pcb.SetOrigin( m_xOrigin * 25.4, m_yOrigin * 25.4 );
    else
        pcb.SetOrigin( m_xOrigin, m_yOrigin );

    if( pcb.ReadFile( m_filename ) )
    {
        if( m_useDrillOrigin )
            pcb.UseDrillOrigin( true );

        if( m_useGridOrigin )
            pcb.UseGridOrigin( true );

        bool res;

        try
        {
            pcb.ComposePCB( m_includeVirtual );

        #ifdef SUPPORTS_IGES
            if( m_fmtIGES )
                res = pcb.WriteIGES( outfile, m_overwrite );
            else
        #endif
                res = pcb.WriteSTEP( outfile, m_overwrite );

            if( !res )
                return -1;
        }
        catch( Standard_Failure e )
        {
            e.Print( std::cerr );
            return -1;
        }
        catch( ... )
        {
            std::cerr << "** (no exception information)\n";
            return -1;
        }
    }

    return 0;
}
