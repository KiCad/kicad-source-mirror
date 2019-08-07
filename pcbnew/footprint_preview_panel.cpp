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

#include <footprint_preview_panel.h>
#include <pcb_draw_panel_gal.h>
#include <kiface_i.h>
#include <kiway.h>
#include <io_mgr.h>
#include <fp_lib_table.h>
#include <view/view.h>
#include <math/box2.h>
#include <class_module.h>
#include <class_board.h>
#include <mutex>
#include <eda_draw_frame.h>
#include <utility>
#include <colors_design_settings.h>
#include <pcb_edit_frame.h>
#include <wx/stattext.h>
#include <pgm_base.h>
#include <painter.h>
#include <pcbnew_id.h>

/**
 * Threadsafe interface class between loader thread and panel class.
 */
class FP_THREAD_IFACE
{
    using CACHE_ENTRY = FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY;

    public:
        /// Retrieve a cache entry by LIB_ID
        OPT<CACHE_ENTRY> GetFromCache( LIB_ID const & aFPID )
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
        CACHE_ENTRY AddToQueue( LIB_ID const & aEntry )
        {
            std::lock_guard<std::mutex> lock( m_lock );

            CACHE_ENTRY ent = { aEntry, NULL, FPS_LOADING };
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
        void AddToCache( CACHE_ENTRY const & aEntry )
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
        bool QueueEvent( wxEvent const& aEvent )
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
            std::lock_guard<std::mutex> locK( m_lock );
            return m_panel ? m_panel->Prj().PcbFootprintLibs() : nullptr;
        }

    private:
        std::deque<CACHE_ENTRY> m_loaderQueue;
        std::map<LIB_ID, CACHE_ENTRY> m_cachedFootprints;
        LIB_ID m_current_fp;
        FOOTPRINT_PREVIEW_PANEL* m_panel = nullptr;
        std::mutex m_lock;
};


/**
 * Footprint loader thread to prevent footprint loading from locking the UI.
 * Interface is via a FP_THREAD_IFACE.
 */
class FP_LOADER_THREAD: public wxThread
{
    using CACHE_ENTRY = FOOTPRINT_PREVIEW_PANEL::CACHE_ENTRY;

    std::shared_ptr<FP_THREAD_IFACE> m_iface;

public:
    FP_LOADER_THREAD( std::shared_ptr<FP_THREAD_IFACE> const& aIface ):
        wxThread( wxTHREAD_DETACHED ),
        m_iface( aIface )
    {}


    ~FP_LOADER_THREAD()
    {}


    void ProcessEntry( CACHE_ENTRY& aEntry )
    {
        FP_LIB_TABLE* fptbl = m_iface->GetTable();

        if( !fptbl )
            return;

        aEntry.module = NULL;

        try {
            aEntry.module = fptbl->FootprintLoadWithOptionalNickname( aEntry.fpid );

            if( !aEntry.module )
                aEntry.status = FPS_NOT_FOUND;

        } catch( const IO_ERROR& )
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
                                                  std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aOpts,
                                                  GAL_TYPE aGalType )
    : PCB_DRAW_PANEL_GAL ( aParent, -1, wxPoint( 0, 0 ), wxSize(200, 200), *aOpts, aGalType  ),
      KIWAY_HOLDER( aKiway, KIWAY_HOLDER::PANEL ),
      m_DisplayOptions( std::move( aOpts ) ),
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
    m_colorsSettings = std::make_unique<COLORS_DESIGN_SETTINGS>( FRAME_PCB_FOOTPRINT_PREVIEW );
    m_colorsSettings->Load( Kiface().KifaceSettings() );

    UseColorScheme( m_colorsSettings.get() );
    SyncLayersVisibility( &*m_dummyBoard );

    Raise();
    Show( true );
    StartDrawing();

    Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate ), NULL, this );
}


FOOTPRINT_PREVIEW_PANEL::~FOOTPRINT_PREVIEW_PANEL( )
{
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
void FOOTPRINT_PREVIEW_PANEL::CacheFootprint( LIB_ID const& aFPID )
{
    (void) CacheAndReturn( aFPID );
}


void FOOTPRINT_PREVIEW_PANEL::renderFootprint(  MODULE *module )
{
    GetView()->Clear();
    module->SetParent( &*m_dummyBoard );

    GetView()->Add( module );
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
}


void FOOTPRINT_PREVIEW_PANEL::DisplayFootprint ( const LIB_ID& aFPID )
{
    m_currentFPID = aFPID;
    m_iface->SetCurrentFootprint( aFPID );
    m_footprintDisplayed = false;

    CACHE_ENTRY fpe = CacheAndReturn ( m_currentFPID );

    if( m_handler )
        m_handler( fpe.status );

    if( fpe.status == FPS_READY )
    {
        if ( !m_footprintDisplayed )
        {
            renderFootprint( fpe.module );
            m_footprintDisplayed = true;
            Refresh();
        }
    }
}


void FOOTPRINT_PREVIEW_PANEL::OnLoaderThreadUpdate( wxCommandEvent& event )
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
    PCB_EDIT_FRAME* pcbnew = static_cast<PCB_EDIT_FRAME*>( aKiway->Player( FRAME_PCB, false ) );
    wxConfigBase*   cfg = Kiface().KifaceSettings();
    wxConfigBase*   commonCfg = Pgm().CommonSettings();
    bool            btemp;
    int             itemp;
    wxString        msg;
    COLOR4D         ctemp;

    // Fetch grid & display settings from PCBNew if it's running; otherwise fetch them
    // from PCBNew's config settings.
    // We need a copy with a lifetime that matches the panel
    std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> gal_opts;

    if( pcbnew )
    {
        // Copy the existing Pcbnew options
        // REVIEW: This also copies the current subscription list of the options
        // to the new options. This is probably not what is intended, but because
        // this widget doesn't change the options it should be OK.
        gal_opts = std::make_unique<KIGFX::GAL_DISPLAY_OPTIONS>( pcbnew->GetGalDisplayOptions() );
    }
    else
    {
        // Make and populate a new one from config
        gal_opts = std::make_unique<KIGFX::GAL_DISPLAY_OPTIONS>();

        gal_opts->ReadConfig( *commonCfg, *cfg, wxString( PCB_EDIT_FRAME_NAME ), aParent );
    }

#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays so default to OpenGL
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE)
                        cfg->ReadLong( CanvasTypeKeyBase, EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE)
                        cfg->ReadLong( CanvasTypeKeyBase, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );
#endif

    auto panel = new FOOTPRINT_PREVIEW_PANEL( aKiway, aParent, std::move( gal_opts ), canvasType );

    if( pcbnew )
    {
        panel->GetGAL()->SetGridVisibility( pcbnew->IsGridVisible() );
        panel->GetGAL()->SetGridSize( VECTOR2D( pcbnew->GetScreen()->GetGridSize() ) );

        // Grid color (among other things):
        KIGFX::PAINTER* pcbnew_painter = pcbnew->GetCanvas()->GetView()->GetPainter();
        panel->GetView()->GetPainter()->ApplySettings( pcbnew_painter->GetSettings() );
    }
    else
    {
        btemp = cfg->ReadBool( wxString( PCB_EDIT_FRAME_NAME ) + ShowGridEntryKeyword, true );
        panel->GetGAL()->SetGridVisibility( btemp );

        // Read grid size:
        std::unique_ptr<PCB_SCREEN> temp_screen = std::make_unique<PCB_SCREEN>( wxSize() );
        cfg->Read( wxString( PCB_EDIT_FRAME_NAME ) + LastGridSizeIdKeyword, &itemp, 0L );
        temp_screen->SetGrid( itemp + ID_POPUP_GRID_LEVEL_1000 );
        panel->GetGAL()->SetGridSize( VECTOR2D( temp_screen->GetGridSize() ) );

        // Read grid color:
        msg = cfg->Read( wxString( PCB_EDIT_FRAME_NAME ) + GridColorEntryKeyword, wxT( "NONE" ) );
        ctemp.SetFromWxString( msg );
        panel->GetGAL()->SetGridColor( ctemp );
    }

    return panel;
}
