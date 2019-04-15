/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb/kicadpcb.h"

class KICAD2MCAD : public wxAppConsole
{
public:
    virtual bool OnInit() override;
    virtual int OnRun() override;
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
    ///> Returns file extension for the selected output format
    wxString getOutputExt() const;

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
    double   m_minDistance;
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
        { wxCMD_LINE_OPTION, NULL, "min-distance",
            _( "Minimum distance between points to treat them as separate ones (default 0.01 mm)" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
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
    m_xOrigin = 0.0;
    m_yOrigin = 0.0;
    m_minDistance = MIN_DISTANCE;

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
                m_xOrigin *= 25.4;
                m_yOrigin *= 25.4;
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
        istr >> m_minDistance;

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
                m_minDistance *= 25.4;
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
    {
        tfname.Assign( fname.GetFullPath() );
        tfname.SetExt( getOutputExt() );
    }
    else
    {
        tfname.Assign( m_outputFile );

        // Set the file extension if the user's requested
        // file name does not have an extension.
        if( !tfname.HasExt() )
            tfname.SetExt( getOutputExt() );
    }

    if( tfname.FileExists() && !m_overwrite )
    {
        std::cerr << "** Output already exists. "
            << "Enable the force overwrite flag to overwrite it." << std::endl;

        return -1;
    }

    wxString outfile = tfname.GetFullPath();
    KICADPCB pcb;

    pcb.SetOrigin( m_xOrigin, m_yOrigin );
    pcb.SetMinDistance( m_minDistance );

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
                res = pcb.WriteIGES( outfile );
            else
        #endif
                res = pcb.WriteSTEP( outfile );

            if( !res )
                return -1;
        }
        catch( const Standard_Failure& e )
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


wxString KICAD2MCAD::getOutputExt() const
{
#ifdef SUPPORTS_IGES
    if( m_fmtIGES )
        return wxString( "igs" );
    else
#endif
        return wxString( "stp" );
}
