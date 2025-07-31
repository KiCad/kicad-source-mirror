/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef ZONE_FILLER_TOOL_H
#define ZONE_FILLER_TOOL_H

#include <tools/pcb_tool_base.h>
#include <zone.h>


class PCB_EDIT_FRAME;
class PROGRESS_REPORTER;
class WX_PROGRESS_REPORTER;
class ZONE_FILLER;

#define ZONE_FILLER_TOOL_NAME "pcbnew.ZoneFiller"

/**
 * Handle actions specific to filling copper zones.
 */
class ZONE_FILLER_TOOL : public PCB_TOOL_BASE
{
public:
    ZONE_FILLER_TOOL();
    ~ZONE_FILLER_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    void CheckAllZones( wxWindow* aCaller, PROGRESS_REPORTER* aReporter = nullptr );
    void FillAllZones( wxWindow* aCaller, PROGRESS_REPORTER* aReporter = nullptr, bool aHeadless = false );

    int ZoneFill( const TOOL_EVENT& aEvent );
    int ZoneFillAll( const TOOL_EVENT& aEvent );
    int ZoneFillDirty( const TOOL_EVENT& aEvent );
    int ZoneUnfill( const TOOL_EVENT& aEvent );
    int ZoneUnfillAll( const TOOL_EVENT& aEvent );

    bool IsBusy() { return m_fillInProgress; }

    PROGRESS_REPORTER* GetProgressReporter();

    void DirtyZone( ZONE* aZone )
    {
        m_dirtyZoneIDs.insert( aZone->m_Uuid );
    }

    static bool IsZoneFillAction( const TOOL_EVENT* aEvent );

private:
    ///< Refocus on an idle event (used after the Progress Reporter messes up the focus).
    void singleShotRefocus( wxIdleEvent& );

    void rebuildConnectivity( bool aHeadless = false );
    void refresh();

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    std::unique_ptr<ZONE_FILLER> m_filler;
    bool                         m_fillInProgress;

    std::set<KIID>               m_dirtyZoneIDs;
};

#endif
