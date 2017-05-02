/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This executable tests for OpenGL compatibility. This is done in a separate
 * process to contain any crashes.
 *
 * Early version, intended just for user testing.
 */

#include <wx/app.h>
#include <wx/debug.h>
#include <wx/settings.h>
#include <wx/frame.h>
#include <wx/regex.h>
#include <gal/opengl/opengl_gal.h>
#include <pgm_base.h>
#include <iostream>

// Required OpenGL version
#define REQUIRED_MAJOR 2L
#define REQUIRED_MINOR 1L

static wxRegEx OGLVersionRegex( R"(^(\d+)\.(\d+))", wxRE_ADVANCED );

static const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 8, 0 };

PGM_BASE& Pgm()
{
    throw std::logic_error("Dummy function called");
}


class OGLTEST_APP : public wxApp
{
public:
    virtual bool OnInit() override;
};

wxIMPLEMENT_APP( OGLTEST_APP );


class WX_QUIET
{
    wxLogLevel m_old_level;
    wxAssertHandler_t m_old_handler;

public:
    WX_QUIET()
    {
        m_old_level = wxLog::GetLogLevel();
        m_old_handler = wxSetAssertHandler( nullptr );
        wxLog::SetLogLevel( wxLOG_FatalError );
    }

    ~WX_QUIET()
    {
        wxLog::SetLogLevel( m_old_level );
        wxSetAssertHandler( m_old_handler );
    }
};


bool OGLTEST_APP::OnInit()
{
    WX_QUIET be_quiet;

    auto frame = new wxFrame( nullptr, wxID_ANY, "OpenGL test", wxDefaultPosition, wxDefaultSize,
           wxTOPLEVEL_EX_DIALOG );

    KIGFX::GAL_DISPLAY_OPTIONS gal_opts;

    auto canvas = new KIGFX::OPENGL_GAL( gal_opts, frame );
    auto context = new wxGLContext( canvas );

    frame->Raise();
    frame->Show();
    canvas->SetCurrent( *context );
    auto size = canvas->GetSize();
    canvas->ResizeScreen( size.GetWidth(), size.GetHeight() );

    printf( "INFO: Instantiated GL window\n" );

    char* pversion = (char*) glGetString( GL_VERSION );

    if( !pversion )
    {
        printf( "FAIL: Cannot get OpenGL version\n" );
        return false;
    }

    wxString version = wxString::FromUTF8( pversion );

    if( !OGLVersionRegex.Matches( version ) )
    {
        printf( "FAIL: Cannot interpret OpenGL version %s\n", pversion );
        return false;
    }

    wxString smajor = OGLVersionRegex.GetMatch( version, 1 );
    wxString sminor = OGLVersionRegex.GetMatch( version, 2 );

    long major, minor;

    smajor.ToLong( &major );
    sminor.ToLong( &minor );

    if( major < REQUIRED_MAJOR || ( major == REQUIRED_MAJOR && minor < REQUIRED_MINOR ) )
    {
        printf( "FAIL: Found OpenGL version %ld.%ld, required at least %ld.%ld\n",
                major, minor, REQUIRED_MAJOR, REQUIRED_MINOR );
        return false;
    }
    else
    {
        printf( "INFO: Found OpenGL version %ld.%ld\n", major, minor );
    }

    canvas->BeginDrawing();
    printf( "INFO: Successfully called OPENGL_GAL::BeginDrawing\n" );
    canvas->EndDrawing();

    bool supported = wxGLCanvas::IsDisplaySupported( &glAttributes[0] );

    if( supported )
        printf( "PASS\n" );
    else
    {
        printf( "FAIL: Display settings not supported\n" );
        return false;
    }

    return false;
}
