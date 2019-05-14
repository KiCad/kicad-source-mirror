/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */

#include <common.h>
#include <fctsys.h>
#include <footprint_info.h>
#include <fp_lib_table.h>
#include <html_messagebox.h>
#include <io_mgr.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <lib_id.h>
#include <macros.h>
#include <pgm_base.h>
#include <thread>
#include <utility>
#include <wildcards_and_files_ext.h>


FOOTPRINT_INFO* FOOTPRINT_LIST::GetModuleInfo( const wxString& aLibNickname,
                                               const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return NULL;

    for( auto& fp : m_list )
    {
        if( aLibNickname == fp->GetLibNickname() && aFootprintName == fp->GetFootprintName() )
            return &*fp;
    }

    return NULL;
}


FOOTPRINT_INFO* FOOTPRINT_LIST::GetModuleInfo( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return NULL;

    LIB_ID fpid;

    wxCHECK_MSG( fpid.Parse( aFootprintName, LIB_ID::ID_PCB ) < 0, NULL,
                 wxString::Format( wxT( "\"%s\" is not a valid LIB_ID." ), aFootprintName ) );

    return GetModuleInfo( fpid.GetLibNickname(), fpid.GetLibItemName() );
}


bool FOOTPRINT_INFO::InLibrary( const wxString& aLibrary ) const
{
    return aLibrary == m_nickname;
}


void FOOTPRINT_LIST::DisplayErrors( wxTopLevelWindow* aWindow )
{
    // @todo: go to a more HTML !<table>! ? centric output, possibly with
    // recommendations for remedy of errors.  Add numeric error codes
    // to PARSE_ERROR, and switch on them for remedies, etc.  Full
    // access is provided to everything in every exception!

    HTML_MESSAGE_BOX dlg( aWindow, _( "Load Error" ) );

    dlg.MessageSet( _( "Errors were encountered loading footprints:" ) );

    wxString msg;

    while( auto error = PopError() )
    {
        wxString tmp = error->Problem();

        // Preserve new lines in error messages so queued errors don't run together.
        tmp.Replace( "\n", "<BR>" );
        msg += wxT( "<p>" ) + tmp + wxT( "</p>" );
    }

    dlg.AddHTML_Text( msg );

    dlg.ShowModal();
}


static FOOTPRINT_LIST* get_instance_from_id( KIWAY& aKiway, int aId )
{
    void* ptr = nullptr;

    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
            return nullptr;

        ptr = kiface->IfaceOrAddress( aId );

        if( !ptr )
            return nullptr;
    }
    catch( ... )
    {
        return nullptr;
    }

    return static_cast<FOOTPRINT_LIST*>( ptr );
}


FOOTPRINT_LIST* FOOTPRINT_LIST::GetInstance( KIWAY& aKiway )
{
    FOOTPRINT_LIST* footprintInfo = get_instance_from_id( aKiway, KIFACE_FOOTPRINT_LIST );

    if( ! footprintInfo )
        return nullptr;

    if( !footprintInfo->GetCount() )
    {
        wxTextFile footprintInfoCache( aKiway.Prj().GetProjectPath() + "fp-info-cache" );
        footprintInfo->ReadCacheFromFile( &footprintInfoCache );
    }

    return footprintInfo;
}


FOOTPRINT_ASYNC_LOADER::FOOTPRINT_ASYNC_LOADER() : m_list( nullptr )
{
    m_total_libs = 0;
}


FOOTPRINT_ASYNC_LOADER::~FOOTPRINT_ASYNC_LOADER()
{
    // This is NOP if the load has finished
    Abort();
}


void FOOTPRINT_ASYNC_LOADER::SetList( FOOTPRINT_LIST* aList )
{
    m_list = aList;
}


void FOOTPRINT_ASYNC_LOADER::Start(
        FP_LIB_TABLE* aTable, wxString const* aNickname, unsigned aNThreads )
{
    // Capture the FP_LIB_TABLE into m_last_table. Formatting it as a string instead of storing the
    // raw data avoids having to pull in the FP-specific parts.
    STRING_FORMATTER sof;
    aTable->Format( &sof, 0 );
    m_last_table = sof.GetString();

    m_list->StartWorkers( aTable, aNickname, this, aNThreads );
}


bool FOOTPRINT_ASYNC_LOADER::Join()
{
    if( m_list )
    {
        bool rv = m_list->JoinWorkers();
        m_list = nullptr;
        return rv;
    }
    else
        return true;
}


void FOOTPRINT_ASYNC_LOADER::Abort()
{
    if( m_list )
    {
        m_list->StopWorkers();
        m_list = nullptr;
    }
}
