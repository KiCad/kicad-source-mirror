/**
 * @file properties_frame.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef  _PROPERTIES_FRAME_H
#define  _PROPERTIES_FRAME_H

#include <wxstruct.h>
#include <pl_editor_frame.h>
#include <dialogs/properties_frame_base.h>

class WORKSHEET_DATAITEM;

/**
 * Class PROPERTIES_FRAME display properties of the current item.
 */

class PROPERTIES_FRAME : public PANEL_PROPERTIES_BASE
{
    PL_EDITOR_FRAME* m_parent;

public:
    PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent );
    ~PROPERTIES_FRAME();

    // Event functions
    void OnAcceptPrms( wxCommandEvent& event );
    void OnSetDefaultValues( wxCommandEvent& event );


    // Data transfert from general properties to widgets
    void CopyPrmsFromGeneralToPanel();

    // Data transfert from widgets to general properties
    bool CopyPrmsFromPanelToGeneral();

    // Data transfert from item to widgets in properties frame
    void CopyPrmsFromItemToPanel( WORKSHEET_DATAITEM* aItem );

    // Data transfert from widgets in properties frame to item
    bool CopyPrmsFromPanelToItem( WORKSHEET_DATAITEM* aItem );

    wxSize GetMinSize() const;
};

#endif /* _PROPERTIES_FRAME_H */
