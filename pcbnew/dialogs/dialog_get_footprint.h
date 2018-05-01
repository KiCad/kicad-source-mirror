/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#ifndef __INCLUDE_DIALOG_GET_COMPONENT_H__
#define __INCLUDE_DIALOG_GET_COMPONENT_H__

#include "dialog_get_footprint_base.h"

class PCB_BASE_FRAME;

void AddHistoryComponentName( const wxString& Name );


/* Dialog frame to choose a component name */
class DIALOG_GET_FOOTPRINT : public DIALOG_GET_FOOTPRINT_BASE
{
private:
    PCB_BASE_FRAME* m_frame;
    wxString        m_Text;
    bool            m_selectionIsKeyword;
    bool            m_selectByBrowser;

public:
    // Constructor and destructor
    DIALOG_GET_FOOTPRINT( PCB_BASE_FRAME* parent, bool aShowBrowseButton );
    ~DIALOG_GET_FOOTPRINT() override {};

    /**
     * Function GetComponentName
     * @return the selection (name or keyword)
     */
    wxString GetComponentName();

    /**
     * Function IsKeyword
     * @return true if the returned string is a keyword
     */
    bool IsKeyword()
    {
        return m_selectionIsKeyword;
    }

    /**
     * Function SelectByBrowser
     * @return true if the footprint browser should be shown to select the footprint
     */
    bool SelectByBrowser()
    {
        return m_selectByBrowser;
    }

    void SetComponentName( const wxString& name );

private:
    void OnHistoryClick( wxCommandEvent& aEvent ) override;
    void Accept( wxCommandEvent& aEvent ) override;
};


#endif /* __INCLUDE_DIALOG_GET_COMPONENT_H__ */
