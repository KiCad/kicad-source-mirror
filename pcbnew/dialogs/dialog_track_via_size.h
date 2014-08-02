/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

/**
 * Push and Shove router track width and via size dialog.
 */

#ifndef __dialog_track_via_size__
#define __dialog_track_via_size__

#include "dialog_track_via_size_base.h"

class PNS_ROUTING_SETTINGS;

/** Implementing DIALOG_TRACK_VIA_SIZE_BASE */
class DIALOG_TRACK_VIA_SIZE : public DIALOG_TRACK_VIA_SIZE_BASE
{
	public:
		/** Constructor */
		DIALOG_TRACK_VIA_SIZE( wxWindow* aParent, PNS_ROUTING_SETTINGS& aSettings );

    protected:
		// Routings settings that are modified by the dialog.
        PNS_ROUTING_SETTINGS& m_settings;

        ///> Checks if values given in the dialog are sensible.
        bool check();

        // Handlers for DIALOG_TRACK_VIA_SIZE_BASE events.
        void onClose( wxCloseEvent& aEvent );
        void onOkClick( wxCommandEvent& aEvent );
        void onCancelClick( wxCommandEvent& aEvent );
};

#endif // __dialog_track_via_size__
