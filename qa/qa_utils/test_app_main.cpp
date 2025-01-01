/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


/*
 * This is a program launcher for a single KIFACE DSO. It only mimics a KIWAY,
 * not actually implements one, since only a single DSO is supported by it.
 *
 * It is compiled multiple times, once for each standalone program and as such
 * gets different compiler command line supplied #defines from CMake.
 */


#include <typeinfo>
#include <string_utils.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/snglinst.h>
#include <wx/html/htmlwin.h>
#include <pgm_base.h>
#include <kiface_base.h>

#include <kiway.h>
#include <pgm_base.h>
#include <kiway_player.h>
#include <confirm.h>
#include <settings/settings_manager.h>
#include <qa_utils/utility_registry.h>


static struct IFACE : public KIFACE_BASE
{
    // Of course all are overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override
    {
        return true;
    }

    void OnKifaceEnd() override {}

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        assert( false );
        return nullptr;
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the requested object which must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }
}
kiface( "pcb_test_frame", KIWAY::FACE_PCB );


KIWAY    Kiway( KFCTL_STANDALONE );


static struct PGM_TEST_FRAME : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {
        printf("Destroy\n");
        Kiway.OnKiwayEnd();

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::Destroy();
    }


    void MacOpenFile( const wxString& aFileName )   override
    {
        wxFileName filename( aFileName );

        if( filename.FileExists() )
        {
    #if 0
            // this pulls in EDA_DRAW_FRAME type info, which we don't want in
            // the single_top link image.
            KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
    #else
            KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
    #endif

            if( frame )
                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
        }
    }
}
program;


KIFACE_BASE& Kiface()
{
    return kiface;
}


/**
 * Implement a bare naked wxApp (so that we don't become dependent on functionality in a wxApp
 * derivative that we cannot deliver under wxPython).
 */
struct APP_TEST : public wxApp
{
    APP_TEST()
    {
        SetPgm( &program );
    }

    bool OnInit() override
    {

        try
        {
            if( !program.OnPgmInit() )
            {
                program.OnPgmExit();
                return false;
            }


            KI_TEST::COMBINED_UTILITY c_util;
            auto app = wxApp::GetInstance();
            auto ret = c_util.HandleCommandLine( app->argc, app->argv );

            if( ret != KI_TEST::RET_CODES::OK)
                return false;

            return true;
        }
        catch( const std::exception& e )
        {
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                        From_UTF8( typeid(e).name() ), From_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch(...)
        {
            wxLogError( wxT( "Unhandled exception of unknown type" ) );
        }

        program.OnPgmExit();

        return false;
    }

    int  OnExit() override
    {
        program.OnPgmExit();
        return wxApp::OnExit();
    }

    int OnRun() override
    {
        int ret = -1;

        try
        {
            ret = wxApp::OnRun();
        }
        catch( const std::exception& e )
        {
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                From_UTF8( typeid(e).name() ),
                From_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch(...)
        {
            wxLogError( wxT( "Unhandled exception of unknown type" ) );
        }

        program.OnPgmExit();
        return ret;
    }
};


bool PGM_TEST_FRAME::OnPgmInit()
{
    return true;
}


void SetTopFrame ( wxFrame* aFrame )
{
    Pgm().App().SetTopWindow( aFrame );      // wxApp gets a face.
    aFrame->Show();
}


IMPLEMENT_APP_NO_MAIN(APP_TEST);


#ifndef TEST_APP_NO_MAIN

int main( int argc, char** argv )
{
    wxInitialize( argc, argv );

#ifdef TEST_APP_GUI
    Pgm().InitPgm( false, true );
#else
    Pgm().InitPgm( true, true );
#endif

    auto ret = wxEntry( argc, argv );

    // This causes some glib warnings on GTK3 (http://trac.wxwidgets.org/ticket/18274)
    // but without it, Valgrind notices a lot of leaks from WX
    wxUninitialize();

    return ret;
}

#endif
