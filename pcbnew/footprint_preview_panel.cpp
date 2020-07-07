/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <mutex>
#include <utility>

#include "pcbnew_settings.h"
#include <class_board.h>
#include <class_module.h>
#include <eda_draw_frame.h>
#include <footprint_preview_panel.h>
#include <fp_lib_table.h>
#include <id.h>
#include <io_mgr.h>
#include <kiway.h>
#include <math/box2.h>
#include <painter.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <wx/stattext.h>

/**
 * Threadsafe interface class between loader thread and panel class.
 */
class FP_THREAD_IFACE
{
    using CACHE_ENTRY = FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY;

    public:
        FP_THREAD_IFACE() : m_panel( nullptr )
        {
        }

        /// Retrieve a cache entry by LIB_ID
        OPT<CACHE_ENTRY> GetFromCache( const LIB_ID& aFPID )
        {
            std::lock_guard<std::mutex> lock( m_lock );
            auto it = m_cachedFootprints.find( aFPID );

            if( it != m_cachedFootprints.end() )
                return it->second;
            else
                return NULLOPT;
        }

        /**
         * Push an entry to the loading queue and a placeholder to the cache;
         * return the placeholder.
         */
        CACHE_ENTRY AddToQueue( const LIB_ID& aEntry )
        {
            std::lock_guard<std::mutex> lock( m_lock );

            CACHE_ENTRY ent            = { aEntry, nullptr, FPS_LOADING };
            m_cachedFootprints[aEntry] = ent;
            m_loaderQueue.push_back( ent );

            return ent;
        }

        /// Pop an entry from the queue, or empty option if none is available.
        OPT<CACHE_ENTRY> PopFromQueue()
        {
            std::lock_guard<std::mutex> lock( m_lock );

            if( m_loaderQueue.empty() )
            {
                return NULLOPT;
            }
            else
            {
                auto ent = m_loaderQueue.front();
                m_loaderQueue.pop_front();
                return ent;
            }
        }

        /// Add an entry to the cache.
        void AddToCache( const CACHE_ENTRY& aEntry )
        {
            std::lock_guard<std::mutex> lock( m_lock );
            m_cachedFootprints[aEntry.fpid] = aEntry;
        }

        /**
         * Threadsafe accessor to set the current footprint.
         */
        void SetCurrentFootprint( LIB_ID aFp )
        {
            std::lock_guard<std::mutex> lock( m_lock );
            m_current_fp = std::move( aFp );
        }

        /**
         * Threadsafe accessor to get the current footprint.
         */
        LIB_ID GetCurrentFootprint()
        {
            std::lock_guard<std::mutex> lock( m_lock );
            return m_current_fp;
        }

        /**
         * Set the associated panel, for QueueEvent() and GetTable().
         */
        void SetPanel( FOOTPRINT_PREVIEW_PANEL* aPanel )
        {
            std::lock_guard<std::mutex> lock( m_lock );
            m_panel = aPanel;
        }

        /**
         * Get the associated panel.
         */
        FOOTPRINT_PREVIEW_PANEL* GetPanel()
        {
            std::lock_guard<std::mutex> lock( m_lock );
            return m_panel;
        }

        /**
         * Post an event to the panel, if the panel still exists. Return whether
         * the event was posted.
         */
        bool QueueEvent( const wxEvent& aEvent )
        {
            std::lock_guard<std::mutex> lock( m_lock );

            if( m_panel )
            {
                m_panel->GetEventHandler()->QueueEvent( aEvent.Clone() );
                return true;
            }
            else
            {
                return false;
            }
        }

        /**
         * Get an FP_LIB_TABLE, or null if the panel is dead.
         */
        FP_LIB_TABLE* GetTable()
        {
            std::lock_guard<std::mutex> lock( m_lock );
            return m_panel ? m_panel->Prj().PcbFootprintLibs() : nullptr;
        }

    private:
        std::deque<CACHE_ENTRY>       m_loaderQueue;
        std::map<LIB_ID, CACHE_ENTRY> m_cachedFootprints;
        LIB_ID                        m_current_fp;
        FOOTPRINT_PREVIEW_PANEL*      m_panel;
        std::mutex                    m_lock;
};


/**
 * Footprint loader thread to prevent footprint loading from locking the UI.
 * Interface is via a FP_THREAD_IFACE.
 */
class FP_LOADER_THREAD : public wxThread
{
    using CACHE_ENTRY = FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY;

    std::shared_ptr<FP_THREAD_IFACE> m_iface;

public:
    FP_LOADER_THREAD( const std::shared_ptr<FP_THREAD_IFACE>& aIface )
            : wxThread( wxTHREAD_DETACHED ), m_iface( aIface )
    {
    }


    ~FP_LOADER_THREAD()
    {
    }


    void ProcessEntry( CACHE_ENTRY& aEntry )
    {
        FP_LIB_TABLE* fptbl = m_iface->GetTable();

        if( !fptbl )
            return;

        try
        {
            aEntry.module.reset( fptbl->FootprintLoadWithOptionalNickname( aEntry.fpid ) );

            if( !aEntry.module )
                aEntry.status = FPS_NOT_FOUND;
        }
        catch( const IO_ERROR& )
        {
            aEntry.status = FPS_NOT_FOUND;
        }

        if( aEntry.status != FPS_NOT_FOUND )
            aEntry.status = FPS_READY;

        m_iface->AddToCache( aEntry );

        if( aEntry.fpid == m_iface->GetCurrentFootprint() )
        {
            wxCommandEvent evt( wxEVT_COMMAND_TEXT_UPDATED, 1 );
            m_iface->QueueEvent( evt );
        }
    }


    virtual void* Entry() override
    {
        while( m_iface->GetPanel() )
        {
            auto ent = m_iface->PopFromQueue();

            if( ent )
                ProcessEntry( *ent );
            else
                wxMilliSleep( 100 );
        }

        return nullptr;
    }
};


FOOTPRINT_PREVIEW_PANEL::FOOTPRINT_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent,
        std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aOpts, GAL_TYPE aGalType )
        : PCB_DRAW_PANEL_GAL( aParent, -1, wxPoint( 0, 0 ), wxSize( 200, 200 ), *aOpts, aGalType ),
          KIWAY_HOLDER( aKiway, KIWAY_HOLDER::PANEL ),
          m_displayOptions( std::move( aOpts ) ),
          m_currentModule( nullptr ),
          m_footprintDisplayed( true )
{
    m_iface = std::make_shared<FP_THREAD_IFACE>();
    m_iface->SetPanel( this );
    m_loader = new FP_LOADER_THREAD( m_iface );
    m_loader->Run();

    SetStealsFocus( false );
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    m_dummyBoard = std::make_unique<BOARD>();
    UpdateColors();
    SyncLayersVisibility( m_dummyBoard.get() );

    Raise();
    Show( true );
    StartDrawing();

    Connect( wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler( FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate ), NULL, this );
}


FOOTPRINT_PREVIEW_PANEL::~FOOTPRINT_PREVIEW_PANEL( )
{
    if( m_currentModule )
    {
        GetView()->Remove( m_currentModule.get() );
        GetView()->Clear();
        m_currentModule->SetParent( nullptr );
    }

    m_iface->SetPanel( nullptr );
}


FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY FOOTPRINT_PREVIEW_PANEL::CacheAndReturn( const LIB_ID& aFPID )
{
    auto opt_ent = m_iface->GetFromCache( aFPID );

    if( opt_ent )
        return *opt_ent;
    else
        return m_iface->AddToQueue( aFPID );
}


// This is separate to avoid having to export CACHE_ENTRY to the global namespace
void FOOTPRINT_PREVIEW_PANEL::CacheFootprint( const LIB_ID& aFPID )
{
    (void) CacheAndReturn( aFPID );
}


void FOOTPRINT_PREVIEW_PANEL::renderFootprint( std::shared_ptr<MODULE> aModule )
{
    if( m_currentModule )
    {
        GetView()->Remove( m_currentModule.get() );
        GetView()->Clear();
        m_currentModule->SetParent( nullptr );
    }

    aModule->SetParent( m_dummyBoard.get() );

    GetView()->Add( aModule.get() );
    GetView()->SetVisible( aModule.get(), true );
    GetView()->Update( aModule.get(), KIGFX::ALL );

    // Save a reference to the module's shared pointer to say we are using it in the
    // preview panel
    m_currentModule = aModule;

    BOX2I bbox = aModule->ViewBBox();
    bbox.Merge( aModule->Value().ViewBBox() );
    bbox.Merge( aModule->Reference().ViewBBox() );

    if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
    {
        // Autozoom
        GetView()->SetViewport( BOX2D( bbox.GetOrigin(), bbox.GetSize() ) );

        // Add a margin
        GetView()->SetScale( GetView()->GetScale() * 0.7 );

        Refresh();
    }
}


void FOOTPRINT_PREVIEW_PANEL::DisplayFootprint ( const LIB_ID& aFPID )
{
    m_currentFPID = aFPID;
    m_iface->SetCurrentFootprint( aFPID );
    m_footprintDisplayed = false;

    CACHE_ENTRY fpe = CacheAndReturn( m_currentFPID );

    if( m_handler )
        m_handler( fpe.status );

    if( fpe.status == FPS_READY )
    {
        if( !m_footprintDisplayed )
        {
            renderFootprint( fpe.module );
            m_footprintDisplayed = true;
            Refresh();
        }
    }
}


void FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate( wxCommandEvent& aEvent )
{
    DisplayFootprint( m_currentFPID );
}


void FOOTPRINT_PREVIEW_PANEL::SetStatusHandler( FOOTPRINT_STATUS_HANDLER aHandler )
{
    m_handler = aHandler;
}


wxWindow* FOOTPRINT_PREVIEW_PANEL::GetWindow()
{
    return static_cast<wxWindow*>( this );
}


FOOTPRINT_PREVIEW_PANEL* FOOTPRINT_PREVIEW_PANEL::New( KIWAY* aKiway, wxWindow* aParent )
{
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    if( cfg->m_Window.grid.sizes.empty() )
    {
        cfg->m_Window.grid.sizes = { "1000 mil",
                                     "500 mil",
                                     "250 mil",
                                     "200 mil",
                                     "100 mil",
                                     "50 mil",
                                     "25 mil",
                                     "20 mil",
                                     "10 mil",
                                     "5 mil",
                                     "2 mil",
                                     "1 mil",
                                     "5.0 mm",
                                     "2.5 mm",
                                     "1.0 mm",
                                     "0.5 mm",
                                     "0.25 mm",
                                     "0.2 mm",
                                     "0.1 mm",
                                     "0.05 mm",
                                     "0.025 mm",
                                     "0.01 mm" };
    }

    if( cfg->m_Window.zoom_factors.empty() )
    {
        cfg->m_Window.zoom_factors = { 0.035,
                                       0.05,
                                       0.08,
                                       0.13,
                                       0.22,
                                       0.35,
                                       0.6,
                                       1.0,
                                       1.5,
                                       2.2,
                                       3.5,
                                       5.0,
                                       8.0,
                                       13.0,
                                       20.0,
                                       35.0,
                                       50.0,
                                       80.0,
                                       130.0,
                                       220.0,
                                       300.0 };
    }

    for( double& factor : cfg->m_Window.zoom_factors )
        factor = std::min( factor, MAX_ZOOM_FACTOR );

    std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> gal_opts;

    gal_opts = std::make_unique<KIGFX::GAL_DISPLAY_OPTIONS>();
    gal_opts->ReadConfig( *Pgm().GetCommonSettings(), cfg->m_Window, aParent );

    auto canvasType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );
    auto panel = new FOOTPRINT_PREVIEW_PANEL( aKiway, aParent, std::move( gal_opts ), canvasType );

    panel->UpdateColors();

    const GRID_SETTINGS& gridCfg = cfg->m_Window.grid;

    panel->GetGAL()->SetGridVisibility( gridCfg.show );

    //Bounds checking cannot include number of elements as an index!
    int gridIdx = std::max( 0, std::min( gridCfg.last_size_idx, (int) gridCfg.sizes.size() - 1 ) );
    int gridSize = (int) ValueFromString( EDA_UNITS::INCHES, gridCfg.sizes[ gridIdx ], true );
    panel->GetGAL()->SetGridSize( VECTOR2D( gridSize, gridSize ) );

    return panel;
}
