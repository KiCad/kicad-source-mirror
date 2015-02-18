/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015  CERN
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

PNS_TUNE_STATUS_POPUP::PNS_TUNE_STATUS_POPUP ( PCB_EDIT_FRAME *parent ) : 
	WX_STATUS_POPUP ( parent )
{
	  m_panel->SetBackgroundColour( wxColour(64,64,64) );
	  m_statusLine = new wxStaticText( m_panel, wxID_ANY,
                          wxT("Status text 1\n") ) ;    
      m_topSizer->Add( m_statusLine, 1, wxALL | wxEXPAND, 5 );
      
      updateSize();
}

PNS_TUNE_STATUS_POPUP::~PNS_TUNE_STATUS_POPUP()
{

}

void PNS_TUNE_STATUS_POPUP::Update( PNS_ROUTER *aRouter )
{
    PNS_MEANDER_PLACER_BASE *placer = dynamic_cast <PNS_MEANDER_PLACER_BASE *> ( aRouter->Placer() );

    if(!placer)
        return;

    m_statusLine->SetLabel ( placer->TuningInfo() );

    wxColour color;

    switch ( placer->TuningStatus() )
    {
    	case PNS_MEANDER_PLACER::TUNED:
    		color = wxColour ( 0, 255, 0 );
    		break;
    	case PNS_MEANDER_PLACER::TOO_SHORT:
    		color = wxColour ( 255, 128, 128 );
    		break;
    	case PNS_MEANDER_PLACER::TOO_LONG:
    		color = wxColour ( 128, 128, 255 );
    		break;
    }

    m_statusLine->SetForegroundColour (color);

	updateSize();
}

