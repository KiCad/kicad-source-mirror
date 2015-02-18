/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_TUNE_STATUS_POPUP_H_
#define __PNS_TUNE_STATUS_POPUP_H_

#include <wx_status_popup.h>

class PNS_ROUTER;

class PNS_TUNE_STATUS_POPUP : public WX_STATUS_POPUP
{
public:
   	PNS_TUNE_STATUS_POPUP ( PCB_EDIT_FRAME *parent );
     ~PNS_TUNE_STATUS_POPUP();

    
    void Update( PNS_ROUTER *aRouter );

private:
	wxStaticText *m_statusLine;
};

#endif /* __PNS_TUNE_STATUS_POPUP_H_*/
