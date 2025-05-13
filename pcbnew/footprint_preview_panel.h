/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <map>
#include <deque>
#include <functional>

#include <pcb_draw_panel_gal.h>
#include <gal/color4d.h>
#include <gal/gal_display_options.h>
#include <lib_id.h>
#include <kiway_player.h>
#include <optional>

#include <widgets/footprint_preview_widget.h>

class FOOTPRINT;
class KIWAY;
class IO_MGR;
class BOARD;
class FOOTPRINT;


/**
 * Panel that renders a single footprint via Cairo GAL, meant to be exported
 * through Kiface.
 */
class FOOTPRINT_PREVIEW_PANEL : public PCB_DRAW_PANEL_GAL,
                                public KIWAY_HOLDER,
                                public FOOTPRINT_PREVIEW_PANEL_BASE
{
public:

    virtual ~FOOTPRINT_PREVIEW_PANEL( );

    void SetUserUnits( EDA_UNITS aUnits ) override { m_userUnits = aUnits; }
    void SetPinFunctions( const std::map<wxString, wxString>& aPinFunctions ) override
    {
        m_pinFunctions = aPinFunctions;
    }

    bool DisplayFootprint( const LIB_ID& aFPID ) override;
    void DisplayFootprints( std::shared_ptr<FOOTPRINT> aFootprintA,
                            std::shared_ptr<FOOTPRINT> aFootprintB ) override;

    const KIGFX::COLOR4D& GetBackgroundColor() const override;
    const KIGFX::COLOR4D& GetForegroundColor() const override;

    EDA_DRAW_PANEL_GAL* GetCanvas() override { return this; };
    BOARD* GetBoard() override { return m_dummyBoard.get(); }

    void RefreshAll() override;

    static FOOTPRINT_PREVIEW_PANEL* New( KIWAY* aKiway, wxWindow* aParent,
                                         UNITS_PROVIDER* aUnitsProvider );
    FOOTPRINT* GetCurrentFootprint() const { return m_currentFootprint.get(); }

    /* Delete loaded footprints and clear view
     * Must be called before deleting the panel
     */
    void ClearViewAndData();

private:
    /**
     * Create a new panel
     *
     * @param aKiway the connected KIWAY
     * @param aParent the owning WX window
     * @param aOpts the GAL options (ownership is assumed)
     * @param aGalType the displayed GAL type
     */
    FOOTPRINT_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent, UNITS_PROVIDER* aUnitsProvider,
                             std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aOpts, GAL_TYPE aGalType );

    void renderFootprint( std::shared_ptr<FOOTPRINT> aFootprint );

    void fitToCurrentFootprint();

private:
    std::unique_ptr<BOARD>                      m_dummyBoard;
    std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> m_displayOptions;
    EDA_UNITS                                   m_userUnits;
    std::map<wxString, wxString>                m_pinFunctions;
    std::shared_ptr<FOOTPRINT>                  m_currentFootprint;
    std::shared_ptr<FOOTPRINT>                  m_otherFootprint;
};

#endif
