/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015  CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
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

#include "pns_tune_status_popup.h"
#include "pns_router.h"
#include "pns_meander_placer.h"

PNS_TUNE_STATUS_POPUP::PNS_TUNE_STATUS_POPUP( EDA_DRAW_FRAME* aParent ) :
    STATUS_TEXT_POPUP( aParent )
{
    m_panel->SetBackgroundColour( wxColour( 64, 64, 64 ) );
}


void PNS_TUNE_STATUS_POPUP::UpdateStatus( PNS::ROUTER* aRouter )
{
    PNS::MEANDER_PLACER_BASE* placer = dynamic_cast<PNS::MEANDER_PLACER_BASE*>( aRouter->Placer() );

    if( !placer )
        return;

    SetText( placer->TuningInfo() );

    switch( placer->TuningStatus() )
    {
    case PNS::MEANDER_PLACER::TUNED:
        SetTextColor( wxColour( 0, 255, 0 ) );
        break;
    case PNS::MEANDER_PLACER::TOO_SHORT:
        SetTextColor( wxColour( 255, 128, 128 ) );
        break;
    case PNS::MEANDER_PLACER::TOO_LONG:
        SetTextColor( wxColour( 128, 128, 255 ) );
        break;
    }
}

