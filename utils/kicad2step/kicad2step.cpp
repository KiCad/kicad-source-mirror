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
#include <Standard_Failure.hxx>

#include "kicadpcb.h"

class KICAD2MCAD : public wxAppConsole
{
public:
    virtual bool OnInit();
    virtual int OnRun();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

private:
#ifdef SUPPORTS_IGES
    bool     m_fmtIGES;
#endif
    bool     m_overwrite;
    wxString m_filename;
    double   m_xOrigin;
    double   m_yOrigin;
};

static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
        { wxCMD_LINE_OPTION, "f", NULL, "input file name",
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
#ifdef SUPPORTS_IGES
        { wxCMD_LINE_SWITCH, "i", NULL, "IGES output (default STEP)",
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
#endif
        { wxCMD_LINE_SWITCH, "w", NULL, "overwrite output file",
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, "x", NULL, "X origin of board",
            wxCMD_LINE_VAL_DOUBLE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, "y", NULL, "Y origin of board (pcbnew coordinate system)",
            wxCMD_LINE_VAL_DOUBLE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, "h", NULL, "display this message",
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
      if( parser.Found( "i" ) )
        m_fmtIGES = true;
    #endif

    if( parser.Found( "w" ) )
        m_overwrite = true;

    parser.Found( "x", &m_xOrigin );
    parser.Found( "y", &m_yOrigin );

    wxString fname;
    parser.Found( "f", &fname );
    m_filename = fname;

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

#ifdef SUPPORTS_IGES
    if( m_fmtIGES )
        fname.SetExt( "igs" );
    else
#endif
        fname.SetExt( "stp" );

    wxString outfile = fname.GetFullPath();

    KICADPCB pcb;
    pcb.SetOrigin( m_xOrigin, m_yOrigin );

    if( pcb.ReadFile( m_filename ) )
    {
        bool res;

        try
        {
            pcb.ComposePCB();

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
