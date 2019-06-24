/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __LENGTH_TUNER_TOOL_H
#define __LENGTH_TUNER_TOOL_H

#include "pns_tool_base.h"
#include "pns_meander.h"

class PNS_TUNE_STATUS_POPUP;

class APIEXPORT LENGTH_TUNER_TOOL : public PNS::TOOL_BASE
{
public:
    LENGTH_TUNER_TOOL();
    ~LENGTH_TUNER_TOOL();

    void Reset( RESET_REASON aReason ) override;

    int MainLoop( const TOOL_EVENT& aEvent );

    void setTransitions() override;

private:
    void performTuning();
    void updateStatusPopup( PNS_TUNE_STATUS_POPUP& aPopup );

    int routerOptionsDialog( const TOOL_EVENT& aEvent );
    int meanderSettingsDialog( const TOOL_EVENT& aEvent );

    PNS::MEANDER_SETTINGS m_savedMeanderSettings;
};

#endif
