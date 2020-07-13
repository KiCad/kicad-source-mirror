/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <wx/file.h>
#include <wx/snglinst.h>
#include <class_board.h>
#include <fp_lib_table.h>
#include <footprint_viewer_frame.h>
#include <footprint_wizard_frame.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_pcb_target.h>
#include <class_module.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_edge_mod.h>
#include <tools/pcb_actions.h>
#include <router/router_tool.h>
#include <dialog_find.h>
#include <dialog_filter_selection.h>


static struct IFACE : public KIFACE_I
{
    // Of course all are overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override
    {
        return true;
    }

    void OnKifaceEnd() override {}

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        assert( false );
        return nullptr;
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }
}
kiface( "pcb_test_frame", KIWAY::FACE_PCB );

static struct PGM_TEST_FRAME : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {
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

PGM_BASE& Pgm()
{
    printf("Pgm @ %p\n", &program );
    return program;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face
// is run from a python script, mot from a Kicad application
PGM_BASE* PgmOrNull()
{
    return nullptr; //&program;
}


KIFACE_I& Kiface()
{
    return kiface;
}


FP_LIB_TABLE GFootprintTable;

DIALOG_FIND::DIALOG_FIND( PCB_BASE_FRAME* aParent ) : DIALOG_FIND_BASE( aParent )
{
    // these members are initialized to avoid warnings about non initialized vars
    m_frame = aParent;
    m_hitList.clear();
    m_it = m_hitList.begin();
    m_upToDate = false;
}

void DIALOG_FIND::onFindNextClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onFindPreviousClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onSearchAgainClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onTextEnter( wxCommandEvent& event )
{
}

void DIALOG_FIND::onClose( wxCommandEvent& aEvent )
{
}


DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title,
                                    const wxPoint& pos, const wxSize& size, long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    // these members are initialized only to avoid warnings about non initialized vars
    searchStringLabel = nullptr;
    m_searchCombo = nullptr;
    m_matchCase = nullptr;
    m_matchWords = nullptr;
    m_wildcards = nullptr;
    m_wrap = nullptr;
    m_includeValues = nullptr;
    m_includeReferences = nullptr;
    m_includeTexts = nullptr;
    m_includeMarkers = nullptr;
    m_includeVias = nullptr;
    m_findNext = nullptr;
    m_findPrevious = nullptr;
    m_searchAgain = nullptr;
    m_closeButton = nullptr;
}


DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
}


MODULE* PCB_BASE_FRAME::GetFootprintFromBoardByReference()
{
    return nullptr;
}


DIALOG_FILTER_SELECTION_BASE::DIALOG_FILTER_SELECTION_BASE( wxWindow* parent, wxWindowID id,
                                                            const wxString& title,
                                                            const wxPoint& pos, const wxSize& size,
                                                            long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    // these members are initialized only to avoid warnings about non initialized vars
    m_Include_Modules = nullptr;
    m_Include_PcbTextes = nullptr;
    m_IncludeLockedModules = nullptr;
    m_Include_Draw_Items = nullptr;
    m_Include_Tracks = nullptr;
    m_Include_Vias = nullptr;
    m_Include_Edges_Items = nullptr;
    m_Include_Zones = nullptr;
    m_sdbSizer1 = nullptr;
    m_staticLine = nullptr;
    m_sdbSizer1OK = nullptr;
    m_sdbSizer1Cancel = nullptr;
}


DIALOG_FILTER_SELECTION_BASE::~DIALOG_FILTER_SELECTION_BASE()
{
}


DIALOG_FILTER_SELECTION::DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions ) :
        DIALOG_FILTER_SELECTION_BASE( aParent ),
        m_options( aOptions )
{
    // silence another compiler warning about m_options not being used
    if( m_options.includeModules )
    {
    }
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( wxCommandEvent& aEvent )
{
}


void DIALOG_FILTER_SELECTION::ExecuteCommand( wxCommandEvent& event )
{
}


void ROUTER_TOOL::NeighboringSegmentFilter( const VECTOR2I&, GENERAL_COLLECTOR& )
{
}

