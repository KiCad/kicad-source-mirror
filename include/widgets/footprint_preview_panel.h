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

#ifndef __FOOTPRINT_PREVIEW_PANEL_H
#define __FOOTPRINT_PREVIEW_PANEL_H

#include <wx/wx.h>

#include <map>
#include <deque>

#include <pcb_draw_panel_gal.h>
#include <gal/gal_display_options.h>
#include <lib_id.h>
#include <kiway_player.h>
#include <boost/optional.hpp>

class MODULE;
class KIWAY;
class IO_MGR;
class BOARD;
class wxStaticText;

enum FOOTPRINT_STATUS {
    FPS_NOT_FOUND = 0,
    FPS_READY = 1,
    FPS_LOADING = 2
};

class FOOTPRINT_PREVIEW_PANEL : public PCB_DRAW_PANEL_GAL, public KIWAY_HOLDER
{
public:


    struct CACHE_ENTRY {
        LIB_ID fpid;
        MODULE *module;
        FOOTPRINT_STATUS status;
    };

    virtual ~FOOTPRINT_PREVIEW_PANEL( );

    virtual CACHE_ENTRY CacheFootprint ( const LIB_ID& aFPID );
    virtual void DisplayFootprint ( const LIB_ID& aFPID );

    /**
     * Link to a label for displaying error and status messages. Can be NULL to
     * unlink.  This does not take ownership of the wx object.
     *
     * TODO if we ever require wx >= 3.1: use wxActivityIndicator for "Loading"
     * status
     */
    virtual void LinkErrorLabel( wxStaticText* aLabel );

    /**
     * Set a sizer to be automatically hidden when the status text is cleared.
     * This is useful to have a status message take the place of the footprint display
     * instead of displaying next to it.
     */
    virtual void SetHideSizer( wxSizer* aSizer );

    /**
     * Set the contents of the status label, and display it if autohide is enabled.
     */
    virtual void SetStatusText( wxString const & aText );

    /**
     * Clear the contents of the status label, and hide it if autohide is enabled.
     */
    virtual void ClearStatus();


#ifdef PCBNEW
    static FOOTPRINT_PREVIEW_PANEL* New( KIWAY* aKiway, wxWindow* aParent );
#endif

    /**
     * Get a preview panel via Kiway and add it to a blank wxPanel. May return
     * NULL in the event of a kiway error.
     *
     * @param aKiway - an active Kiway instance
     * @param aPanel - a blank panel to receive the previewer
     * @param aStatus - if true, also add indicator elements to display status and errors.
     */
    static FOOTPRINT_PREVIEW_PANEL* InstallOnPanel( KIWAY& aKiway, wxPanel* aPanel, bool aStatus );

protected:

    class LOADER_THREAD : public wxThread
    {
        public:
            LOADER_THREAD ( FOOTPRINT_PREVIEW_PANEL *aParent );
            ~LOADER_THREAD ();

            /**
             * Threadsafe accessor to retrieve an entry from the cache.
             */
            boost::optional<CACHE_ENTRY> GetFromCache( LIB_ID const & aFPID );

            /**
             * Threadsafe accessor to push an entry to the queue to be loaded.
             * Also adds a placeholder to the cache and returns it.
             */
            CACHE_ENTRY AddToQueue( LIB_ID const & aEntry );

        protected:
            void* Entry() override;
            FOOTPRINT_PREVIEW_PANEL *m_parent;

            /**
             * Threadsafe accessor to pop from the loader queue. Returns a
             * cache entry or an empty option if there is none.
             */
            boost::optional<CACHE_ENTRY> PopFromQueue();

            /**
             * Threadsafe accessor to add an entry to the cache. Puts it into
             * both the footprint cache and the loader queue.
             */
            void AddToCache( CACHE_ENTRY const & aEntry );

        private:
            /* DO NOT access directly unless you are an accessor function.
             * USE THE ACCESSOR FUNCTIONS.
             */
            std::deque<CACHE_ENTRY> m_loaderQueue;
            std::map<LIB_ID, CACHE_ENTRY> m_cachedFootprints;
            wxMutex m_loaderLock;

    };

    FOOTPRINT_PREVIEW_PANEL(
            KIWAY* aKiway, wxWindow* aParent,
            KIGFX::GAL_DISPLAY_OPTIONS& aOpts, GAL_TYPE aGalType );

private:
    void OnPaint( wxPaintEvent& event );
    void OnLoaderThreadUpdate( wxCommandEvent& aEvent );

    void renderFootprint( MODULE *module );

    std::unique_ptr<LOADER_THREAD> m_loader;

    std::unique_ptr<BOARD> m_dummyBoard;

    LIB_ID m_currentFPID;
    bool m_footprintDisplayed;

    wxStaticText* m_label;
    wxSizer* m_hidesizer;
};

#endif
