/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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

#include <wx/utils.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <iostream>
#include <sstream>
#include <string>

#include "kicadpcb.h"
#include "sexpr/sexpr.h"
#include "sexpr/sexpr_parser.h"
#include "kicadmodule.h"
#include "kicadcurve.h"
#include "oce_utils.h"


/*
 * GetKicadConfigPath() is taken from KiCad's common.cpp source:
 * Copyright (C) 2014-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers
 */
static wxString GetKicadConfigPath()
{
    wxFileName cfgpath;

    // From the wxWidgets wxStandardPaths::GetUserConfigDir() help:
    //      Unix: ~ (the home directory)
    //      Windows: "C:\Documents and Settings\username\Application Data"
    //      Mac: ~/Library/Preferences
    cfgpath.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

#if !defined( __WINDOWS__ ) && !defined( __WXMAC__ )
    wxString envstr;

    if( !wxGetEnv( "XDG_CONFIG_HOME", &envstr ) || envstr.IsEmpty() )
    {
        // XDG_CONFIG_HOME is not set, so use the fallback
        cfgpath.AppendDir( ".config" );
    }
    else
    {
        // Override the assignment above with XDG_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }
#endif

    cfgpath.AppendDir( "kicad" );

    if( !cfgpath.DirExists() )
    {
        cfgpath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    return cfgpath.GetPath();
}


KICADPCB::KICADPCB()
{
    wxFileName cfgdir( GetKicadConfigPath(), "" );
    cfgdir.AppendDir( "3d" );
    m_resolver.Set3DConfigDir( cfgdir.GetPath() );
    m_thickness = 1.6;
    m_pcb = NULL;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_hasGridOrigin = false;
    m_hasDrillOrigin = false;

    return;
}


KICADPCB::~KICADPCB()
{
    for( auto i : m_modules )
        delete i;

    for( auto i : m_curves )
        delete i;

    if( m_pcb )
        delete m_pcb;

    return;
}


bool KICADPCB::ReadFile( const wxString& aFileName )
{
    wxFileName fname( aFileName );

    if( fname.GetExt() != "kicad_pcb" )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << "  * expecting extension 'kicad_pcb', got '";
        ostr << fname.GetExt().ToUTF8() << "'\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );

        return false;
    }

    if( !fname.FileExists() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << "  * no such file: '" << aFileName.ToUTF8() << "'\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );

        return false;
    }

    fname.Normalize();
    m_filename = fname.GetFullPath().ToUTF8();
    m_resolver.SetProjectDir( fname.GetPath() );

    try
    {
        SEXPR::PARSER parser;
        std::string infile( fname.GetFullPath().ToUTF8() );
        SEXPR::SEXPR* data = parser.ParseFromFile( infile );

        if( NULL == data )
        {
            std::ostringstream ostr;
            ostr << "* no data in file: '" << aFileName.ToUTF8() << "'\n";
            wxLogMessage( "%s\n", ostr.str().c_str() );

            return false;
        }

        if( !parsePCB( data ) )
            return false;

    }
    catch( std::exception& e )
    {
        std::ostringstream ostr;
        ostr << "* error reading file: '" << aFileName.ToUTF8() << "'\n";
        ostr << "  * " << e.what() << "\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );

        return false;
    }
    catch( ... )
    {
        std::ostringstream ostr;
        ostr << "* unexpected exception while reading file: '" << aFileName.ToUTF8() << "'\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );

        return false;
    }

    return true;
}


bool KICADPCB::WriteSTEP( const wxString& aFileName, bool aOverwrite )
{
    if( m_pcb )
    {
        std::string filename( aFileName.ToUTF8() );
        return m_pcb->WriteSTEP( filename, aOverwrite );
    }

    return false;
}


#ifdef SUPPORTS_IGES
bool KICADPCB::WriteIGES( const wxString& aFileName, bool aOverwrite )
{
    if( m_pcb )
    {
        std::string filename( aFileName.ToUTF8() );
        return m_pcb->WriteIGES( filename, aOverwrite );
    }

    return false;
}
#endif


bool KICADPCB::parsePCB( SEXPR::SEXPR* data )
{
    if( NULL == data )
        return false;

    if( data->IsList() )
    {
        size_t nc = data->GetNumberOfChildren();
        SEXPR::SEXPR* child = data->GetChild( 0 );
        std::string name = child->GetSymbol();

        if( name != "kicad_pcb" )
        {
            std::ostringstream ostr;
            ostr << "* data is not a valid PCB file: '" << m_filename << "'\n";
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }

        bool result = true;

        for( size_t i = 1; i < nc && result; ++i )
        {
            child = data->GetChild( i );

            if( !child->IsList() )
            {
                std::ostringstream ostr;
                ostr << "* corrupt PCB file (line " << child->GetLineNumber();
                ostr << "): '" << m_filename << "'\n";
                wxLogMessage( "%s\n", ostr.str().c_str() );
                return false;
            }

            std::string symname( child->GetChild( 0 )->GetSymbol() );

            if( symname == "general" )
                result = result && parseGeneral( child );
            else if( symname == "setup" )
                result = result && parseSetup( child );
            else if( symname == "module" )
                result = result && parseModule( child );
            else if( symname == "gr_arc" )
                result = result && parseCurve( child, CURVE_ARC );
            else if( symname == "gr_line" )
                result = result && parseCurve( child, CURVE_LINE );
            else if( symname == "gr_circle" )
                result = result && parseCurve( child, CURVE_CIRCLE );
        }

        return result;
    }

    std::ostringstream ostr;
    ostr << "* data is not a valid PCB file: '" << m_filename << "'\n";
    wxLogMessage( "%s\n", ostr.str().c_str() );

    return false;
}


bool KICADPCB::parseGeneral( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            std::ostringstream ostr;
            ostr << "* corrupt PCB file (line " << child->GetLineNumber();
            ostr << "): '" << m_filename << "'\n";
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }

        // at the moment only the thickness is of interest in
        // the general section
        if( child->GetChild( 0 )->GetSymbol() != "thickness" )
            continue;

        m_thickness = child->GetChild( 1 )->GetDouble();
        return true;
    }

    std::ostringstream ostr;
    ostr << "* corrupt PCB file (line " << child->GetLineNumber();
    ostr << "): '" << m_filename << "'\n";
    ostr << "* no PCB thickness specified in general section\n";
    wxLogMessage( "%s\n", ostr.str().c_str() );

    return false;
}


bool KICADPCB::parseSetup( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            std::ostringstream ostr;
            ostr << "* corrupt PCB file (line " << child->GetLineNumber();
            ostr << "): '" << m_filename << "'\n";
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }

        // at the moment only the Grid and Drill origins are of interest in
        // the setup section
        if( child->GetChild( 0 )->GetSymbol() == "grid_origin" )
        {
            if( child->GetNumberOfChildren() != 3 )
            {
                std::ostringstream ostr;
                ostr << "* corrupt PCB file (line " << child->GetLineNumber();
                ostr << "): '" << m_filename << "'\n";
                ostr << "* grid_origin has " << child->GetNumberOfChildren();
                ostr << " children (expected: 3)\n";
                wxLogMessage( "%s\n", ostr.str().c_str() );

                return false;
            }

            m_gridOrigin.x = child->GetChild( 1 )->GetDouble();
            m_gridOrigin.y = child->GetChild( 2 )->GetDouble();
            m_hasGridOrigin = true;
        }
        else if( child->GetChild( 0 )->GetSymbol() == "aux_axis_origin" )
        {
            if( child->GetNumberOfChildren() != 3 )
            {
                std::ostringstream ostr;
                ostr << "* corrupt PCB file (line " << child->GetLineNumber();
                ostr << "): '" << m_filename << "'\n";
                ostr << "* aux_axis_origin has " << child->GetNumberOfChildren();
                ostr << " children (expected: 3)\n";
                wxLogMessage( "%s\n", ostr.str().c_str() );

                return false;
            }

            m_drillOrigin.x = child->GetChild( 1 )->GetDouble();
            m_drillOrigin.y = child->GetChild( 2 )->GetDouble();
            m_hasDrillOrigin = true;
        }

    }

    return true;
}


bool KICADPCB::parseModule( SEXPR::SEXPR* data )
{
    KICADMODULE* mp = new KICADMODULE();

    if( !mp->Read( data ) )
    {
        delete mp;
        return false;
    }

    m_modules.push_back( mp );
    return true;
}


bool KICADPCB::parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType )
{
    KICADCURVE* mp = new KICADCURVE();

    if( !mp->Read( data, aCurveType ) )
    {
        delete mp;
        return false;
    }

    // reject any curves not on the Edge.Cuts layer
    if( mp->GetLayer() != LAYER_EDGE )
    {
        delete mp;
        return true;
    }

    m_curves.push_back( mp );
    return true;
}


bool KICADPCB::ComposePCB( bool aComposeVirtual )
{
    if( m_pcb )
        return true;

    if( m_modules.empty() && m_curves.empty() )
    {
        std::ostringstream ostr;
        ostr << "** " << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << "*  no PCB data to render\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    DOUBLET origin;

    // Determine the coordinate system reference:
    // Precedence of reference point is Drill Origin > Grid Origin > User Offset
    if( m_useDrillOrigin && m_hasDrillOrigin )
    {
        origin = m_drillOrigin;
    }
    else if( m_useGridOrigin && m_hasDrillOrigin )
    {
        origin = m_gridOrigin;
    }
    else
    {
        origin = m_origin;
    }

    m_pcb = new PCBMODEL();
    m_pcb->SetPCBThickness( m_thickness );

    for( auto i : m_curves )
    {
        if( CURVE_NONE == i->m_form || LAYER_EDGE != i->m_layer )
            continue;

        // adjust the coordinate system
        KICADCURVE lcurve = *i;
        lcurve.m_start.y = -( lcurve.m_start.y - origin.y );
        lcurve.m_end.y = -( lcurve.m_end.y - origin.y );
        lcurve.m_start.x -= origin.x;
        lcurve.m_end.x -= origin.x;

        if( CURVE_ARC == lcurve.m_form )
            lcurve.m_angle = -lcurve.m_angle;

        m_pcb->AddOutlineSegment( &lcurve );
    }

    for( auto i : m_modules )
        i->ComposePCB( m_pcb, &m_resolver, origin, aComposeVirtual );

    if( !m_pcb->CreatePCB() )
    {
        std::ostringstream ostr;
        ostr << "** " << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "\n";
        ostr << "*  could not create PCB solid model\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        delete m_pcb;
        m_pcb = NULL;
        return false;
    }

    return true;
}
