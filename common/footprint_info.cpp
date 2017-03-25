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


/**
 * @file footprint_info.cpp
 */


/*
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */

#include <class_module.h>
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
#include <wildcards_and_files_ext.h>


FOOTPRINT_INFO* FOOTPRINT_LIST::GetModuleInfo( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return NULL;

    for( auto& fp : m_list )
    {
        LIB_ID fpid;

        wxCHECK_MSG( fpid.Parse( TO_UTF8( aFootprintName ) ) < 0, NULL,
                wxString::Format(
                        "'%s' is not a valid LIB_ID.", aFootprintName ) );

        wxString libNickname = FROM_UTF8( fpid.GetLibNickname() );
        wxString footprintName = FROM_UTF8( fpid.GetLibItemName() );

        if( libNickname == fp->GetNickname() && footprintName == fp->GetFootprintName() )
            return &*fp;
    }

    return NULL;
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
        msg += wxT( "<p>" ) + error->Problem() + wxT( "</p>" );
    }

    dlg.AddHTML_Text( msg );

    dlg.ShowModal();
}


static std::unique_ptr<FOOTPRINT_LIST> get_instance_from_id( KIWAY& aKiway, int aId )
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

    return std::unique_ptr<FOOTPRINT_LIST>( (FOOTPRINT_LIST*) ( ptr ) );
}


std::unique_ptr<FOOTPRINT_LIST> FOOTPRINT_LIST::GetInstance( KIWAY& aKiway )
{
    return get_instance_from_id( aKiway, KIFACE_NEW_FOOTPRINT_LIST );
}


FOOTPRINT_ASYNC_LOADER::FOOTPRINT_ASYNC_LOADER() : m_list( nullptr )
{
}


void FOOTPRINT_ASYNC_LOADER::SetList( FOOTPRINT_LIST* aList )
{
    m_list = aList;
}


void FOOTPRINT_ASYNC_LOADER::Start(
        FP_LIB_TABLE* aTable, wxString const* aNickname, unsigned aNThreads )
{
    m_started = true;

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


int FOOTPRINT_ASYNC_LOADER::GetProgress() const
{
    if( !m_started )
        return 0;
    else if( m_total_libs == 0 || !m_list )
        return 100;
    else
    {
        int loaded = m_list->CountFinished();
        int prog = ( 100 * loaded ) / m_total_libs;

        if( loaded == m_total_libs )
            return 100;
        else if( loaded < m_total_libs && prog >= 100 )
            return 99;
        else if( prog <= 0 )
            return 1;
        else
            return prog;
    }
}


void FOOTPRINT_ASYNC_LOADER::SetCompletionCallback( std::function<void()> aCallback )
{
    m_completion_cb = aCallback;
}


bool FOOTPRINT_ASYNC_LOADER::IsSameTable( FP_LIB_TABLE* aOther )
{
    STRING_FORMATTER sof;
    aOther->Format( &sof, 0 );
    return m_last_table == sof.GetString();
}
