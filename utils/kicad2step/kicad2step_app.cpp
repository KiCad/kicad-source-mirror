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
#include <regex>

#include "kicad2step.h"
#include "kicad2step_frame_base.h"
#include <Standard_Failure.hxx> // In open cascade

#define REGEX_QUANTITY "([\\s]*[+-]?[\\d]*[.]?[\\d]*)"
#define REGEX_DELIMITER "(?:[\\s]*x)"
#define REGEX_UNIT "([m]{2}|(?:in))"

class KICAD2MCAD_APP : public wxApp
{
public:
    KICAD2MCAD_APP() : wxApp(), m_converter( nullptr ) {}

    virtual bool OnInit() override;
    virtual int  OnRun() override;
    virtual void OnInitCmdLine( wxCmdLineParser& parser ) override;
    virtual bool OnCmdLineParsed( wxCmdLineParser& parser ) override;

private:
    KICAD2STEP*     m_converter;
    KICAD2MCAD_PRMS m_params;
};


wxIMPLEMENT_APP( KICAD2MCAD_APP );


static const wxCmdLineEntryDesc cmdLineDesc[] = {
    { wxCMD_LINE_PARAM, NULL, NULL, _( "pcb_filename" ).mb_str(), wxCMD_LINE_VAL_STRING,
      wxCMD_LINE_OPTION_MANDATORY },
    { wxCMD_LINE_OPTION, "o", "output-filename", _( "output filename" ).mb_str(),
      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },

#ifdef SUPPORTS_IGES
    { wxCMD_LINE_SWITCH, "fmt-iges", NULL, _( "IGES output (default STEP)" ).mb_str(),
      wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
#endif

    { wxCMD_LINE_SWITCH, "f", "force", _( "overwrite output file" ).mb_str(), wxCMD_LINE_VAL_NONE,
      wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, NULL, "drill-origin", _( "Use Drill Origin for output origin" ).mb_str(),
      wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, NULL, "grid-origin", _( "Use Grid Origin for output origin" ).mb_str(),
      wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, NULL, "user-origin",
      _( "User-specified output origin ex. 1x1in, 1x1inch, 25.4x25.4mm (default mm)" ).mb_str(),
      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, NULL, "no-virtual",
      _( "Exclude 3D models for components with 'virtual' attribute" ).mb_str(),
      wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, NULL, "subst-models",
      _( "Substitute STEP or IGS models with the same name in place of VRML models" ).mb_str(),
      wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, NULL, "min-distance",
      _( "Minimum distance between points to treat them as separate ones (default 0.01 mm)" )
              .mb_str(),
      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "h", NULL, _( "display this message" ).mb_str(), wxCMD_LINE_VAL_NONE,
      wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
};


void KICAD2MCAD_APP::OnInitCmdLine( wxCmdLineParser& parser )
{
    parser.SetDesc( cmdLineDesc );
    parser.SetSwitchChars( wxT( "-" ) );
}


bool KICAD2MCAD_APP::OnInit()
{
    if( !wxApp::OnInit() )
        return false;

    // create the main application window
    m_converter = new KICAD2STEP( m_params );

    return true;
}


int KICAD2MCAD_APP::OnRun()
{
    int diag = m_converter->Run();
    wxApp::OnRun(); // Start the main loop event, to manage the main frame

    return diag;
}


bool KICAD2MCAD_APP::OnCmdLineParsed( wxCmdLineParser& parser )
{
#ifdef SUPPORTS_IGES
    if( parser.Found( wxT( "fmt-iges" ) ) )
        m_fmtIGES = true;
#endif

    if( parser.Found( wxT( "f" ) ) )
        m_params.m_overwrite = true;

    if( parser.Found( wxT( "grid-origin" ) ) )
        m_params.m_useGridOrigin = true;

    if( parser.Found( wxT( "drill-origin" ) ) )
        m_params.m_useDrillOrigin = true;

    if( parser.Found( wxT( "no-virtual" ) ) )
        m_params.m_includeVirtual = false;

    if( parser.Found( wxT( "subst-models" ) ) )
        m_params.m_substModels = true;

    wxString tstr;

    if( parser.Found( wxT( "user-origin" ), &tstr ) )
    {
        std::regex  re_pattern( REGEX_QUANTITY REGEX_DELIMITER REGEX_QUANTITY REGEX_UNIT,
                                std::regex_constants::icase );
        std::smatch sm;
        std::string str( tstr.ToUTF8() );
        std::regex_search( str, sm, re_pattern );
        m_params.m_xOrigin = atof( sm.str( 1 ).c_str() );
        m_params.m_yOrigin = atof( sm.str( 2 ).c_str() );

        std::string tunit( sm[3] );

        if( tunit.size() > 0 ) // No unit accepted ( default = mm )
        {
            if( ( !sm.str( 1 ).compare( " " ) || !sm.str( 2 ).compare( " " ) ) || ( sm.size() != 4 ) )
            {
                parser.Usage();
                return false;
            }

            // only in, inch and mm are valid:
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

    if( parser.Found( wxT( "min-distance" ), &tstr ) )
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

    if( parser.Found( wxT( "o" ), &tstr ) )
        m_params.m_outputFile = tstr;


    if( parser.GetParamCount() < 1 )
    {
        parser.Usage();
        return false;
    }

    m_params.m_filename = parser.GetParam( 0 );

    return true;
}
