/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2016 Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <widgets/footprint_preview_panel.h>
#include <pcb_draw_panel_gal.h>

#include <kiway.h>
#include <io_mgr.h>
#include <fp_lib_table.h>
#include <view/view.h>
#include <math/box2.h>
#include <class_module.h>
#include <class_board.h>

#include <boost/bind.hpp>
#include <make_unique.h>

#include <wx/stattext.h>


class FP_LOADER_THREAD: public wxThread
{
    FOOTPRINT_PREVIEW_PANEL* m_parent;
    std::shared_ptr<FOOTPRINT_PREVIEW_PANEL::IFACE> m_iface;

public:
    FP_LOADER_THREAD( FOOTPRINT_PREVIEW_PANEL* aParent,
                      std::shared_ptr<FOOTPRINT_PREVIEW_PANEL::IFACE> const& aIface ):
        wxThread( wxTHREAD_DETACHED ),
        m_parent( aParent ),
        m_iface( aIface )
    {}

    ~FP_LOADER_THREAD()
    {}

    virtual void* Entry() override
    {
        while(!TestDestroy())
        {
            auto ent = m_iface->PopFromQueue();

            if( ent )
            {
                FP_LIB_TABLE*   fptbl = m_parent->Prj().PcbFootprintLibs();

                if(!fptbl)
                    continue;

                ent->module = NULL;

                try {
                    ent->module = fptbl->FootprintLoadWithOptionalNickname( ent->fpid );

                    if(ent->module == NULL)
                        ent->status = FPS_NOT_FOUND;

                } catch( const IO_ERROR& ioe )
                {
                    ent->status = FPS_NOT_FOUND;
                }


                if(ent->status != FPS_NOT_FOUND )
                    ent->status = FPS_READY;

                m_iface->AddToCache( *ent );

                if( ent->fpid == m_parent->m_currentFPID )
                {
                    wxCommandEvent event( wxEVT_COMMAND_TEXT_UPDATED, 1 );
                    m_parent->GetEventHandler()->AddPendingEvent ( event );
                }

            } else {
                wxMilliSleep(100);
            }
        }

        return NULL;
    }
};


boost::optional<FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY>
FOOTPRINT_PREVIEW_PANEL::IFACE::PopFromQueue()
{
    wxMutexLocker lock( m_loaderLock );

    if( m_loaderQueue.empty() )
    {
        return boost::none;
    }
    else
    {
        auto ent = m_loaderQueue.front();
        m_loaderQueue.pop_front();
        return ent;
    }
}


FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY
FOOTPRINT_PREVIEW_PANEL::IFACE::AddToQueue( LIB_ID const & aEntry )
{
    wxMutexLocker lock( m_loaderLock );

    CACHE_ENTRY ent = { aEntry, NULL, FPS_LOADING };
    m_cachedFootprints[aEntry] = ent;
    m_loaderQueue.push_back( ent );

    return ent;
}


void FOOTPRINT_PREVIEW_PANEL::IFACE::AddToCache(
        FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY const & aEntry )
{
    wxMutexLocker lock( m_loaderLock );

    m_cachedFootprints[aEntry.fpid] = aEntry;
}


boost::optional<FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY>
FOOTPRINT_PREVIEW_PANEL::IFACE::GetFromCache( LIB_ID const & aFPID )
{
    wxMutexLocker lock( m_loaderLock );
    auto it = m_cachedFootprints.find( aFPID );

    if( it != m_cachedFootprints.end() )
        return it->second;
    else
        return boost::none;
}


FOOTPRINT_PREVIEW_PANEL::FOOTPRINT_PREVIEW_PANEL(
        KIWAY* aKiway, wxWindow* aParent, KIGFX::GAL_DISPLAY_OPTIONS& aOpts, GAL_TYPE aGalType )
    : PCB_DRAW_PANEL_GAL ( aParent, -1, wxPoint( 0, 0 ), wxSize(200, 200), aOpts, aGalType  ),
      KIWAY_HOLDER( aKiway ),
      m_footprintDisplayed( true ),
      m_label( NULL ),
      m_hidesizer( NULL )
{

    m_iface = std::make_shared<IFACE>();
    m_loader = new FP_LOADER_THREAD( this, m_iface );
    m_loader->Run();

    SetStealsFocus( false );
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    m_dummyBoard = std::make_unique<BOARD>();

    UseColorScheme( m_dummyBoard->GetColorsSettings() );
    SyncLayersVisibility( &*m_dummyBoard );

    Raise();
    Show(true);
    StartDrawing();

    Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate ), NULL, this );

}


FOOTPRINT_PREVIEW_PANEL::~FOOTPRINT_PREVIEW_PANEL( )
{
    m_loader->Delete();
}


FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY
FOOTPRINT_PREVIEW_PANEL::CacheFootprint ( const LIB_ID& aFPID )
{
    auto opt_ent = m_iface->GetFromCache( aFPID );

    if( opt_ent )
        return *opt_ent;
    else
        return m_iface->AddToQueue( aFPID );
}


void FOOTPRINT_PREVIEW_PANEL::renderFootprint(  MODULE *module )
{
    Freeze();
    GetView()->Clear();
    module->SetParent ( &*m_dummyBoard );
    module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, GetView(), _1, -1 ) );

    GetView()->Add ( module );

    GetView()->SetVisible( module, true );
    GetView()->Update( module, KIGFX::ALL );

    BOX2I bbox = module->ViewBBox();
    bbox.Merge ( module->Value().ViewBBox() );
    bbox.Merge ( module->Reference().ViewBBox() );

    if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
    {
        // Autozoom
        GetView()->SetViewport( BOX2D( bbox.GetOrigin(), bbox.GetSize() ) );

        // Add a margin
        GetView()->SetScale( GetView()->GetScale() * 0.7 );

        Refresh();
    }

    Thaw();
}


void FOOTPRINT_PREVIEW_PANEL::DisplayFootprint ( const LIB_ID& aFPID )
{
    m_currentFPID = aFPID;
    m_footprintDisplayed = false;

    CACHE_ENTRY fpe = CacheFootprint ( m_currentFPID );

    switch( fpe.status )
    {
    case FPS_NOT_FOUND:
        SetStatusText( _( "Footprint not found" ) );
        break;

    case FPS_LOADING:
        SetStatusText( _( "Loading..." ) );
        break;

    case FPS_READY:
        if ( !m_footprintDisplayed )
        {
            ClearStatus();
            renderFootprint( fpe.module );
            m_footprintDisplayed = true;
            Refresh();
        }
    }
}


void FOOTPRINT_PREVIEW_PANEL::LinkErrorLabel( wxStaticText* aLabel )
{
    m_label = aLabel;
}


void FOOTPRINT_PREVIEW_PANEL::SetHideSizer( wxSizer* aSizer )
{
    m_hidesizer = aSizer;
}


void FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate( wxCommandEvent& event )
{
    DisplayFootprint( m_currentFPID );
}


void FOOTPRINT_PREVIEW_PANEL::SetStatusText( wxString const & aText )
{
    Freeze();
    if( m_label )
    {
        m_label->SetLabel( aText );
    }

    if( m_hidesizer )
    {
        m_hidesizer->ShowItems( true );
        Hide();
    }

    GetParent()->Layout();
    Thaw();
}


void FOOTPRINT_PREVIEW_PANEL::ClearStatus()
{
    Freeze();
    if( m_label )
    {
        m_label->SetLabel( wxEmptyString );
    }

    if( m_hidesizer )
    {
        Show();
        m_hidesizer->ShowItems( false );
    }

    GetParent()->Layout();
    Thaw();
}


FOOTPRINT_PREVIEW_PANEL* FOOTPRINT_PREVIEW_PANEL::New( KIWAY* aKiway, wxWindow* aParent )
{
    KIGFX::GAL_DISPLAY_OPTIONS gal_opts;

    return new FOOTPRINT_PREVIEW_PANEL(
            aKiway, aParent, gal_opts, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );
}
