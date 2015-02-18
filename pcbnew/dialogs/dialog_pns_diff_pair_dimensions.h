/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014-2015  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * Push and Shove diff pair dimensions (gap) settings dialog.
 */

#ifndef __dialog_diff_pair_dimensions_settings__
#define __dialog_diff_pair_dimensions_settings__

#include <wx_unit_binder.h>

#include "dialog_pns_diff_pair_dimensions_base.h"

class PNS_SIZES_SETTINGS;

class DIALOG_PNS_DIFF_PAIR_DIMENSIONS : public DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE
{
	public:
		DIALOG_PNS_DIFF_PAIR_DIMENSIONS( wxWindow* aParent, PNS_SIZES_SETTINGS& aSizes );

        virtual void OnClose( wxCloseEvent& aEvent );
        virtual void OnOkClick( wxCommandEvent& aEvent );
        virtual void OnCancelClick( wxCommandEvent& aEvent );
		virtual void OnViaTraceGapEqualCheck( wxCommandEvent& event );
	

	private:
		void updateCheckbox( );

		WX_UNIT_BINDER m_traceWidth;
		WX_UNIT_BINDER m_traceGap;
		WX_UNIT_BINDER m_viaGap;

		PNS_SIZES_SETTINGS& m_sizes;
};

#endif // __dialog_pns_settings__
