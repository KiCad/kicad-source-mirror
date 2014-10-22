/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * This file is part of the common libary.
 * @file  dialog_get_component.h
 */

#ifndef __INCLUDE_DIALOG_GET_COMPONENT_H__
#define __INCLUDE_DIALOG_GET_COMPONENT_H__

#include <../common/dialogs/dialog_get_component_base.h>

void     AddHistoryComponentName( wxArrayString& HistoryList,
                                  const wxString& Name );

/* Dialog frame to choose a component name */
class DIALOG_GET_COMPONENT : public DIALOG_GET_COMPONENT_BASE
{
private:
    bool        m_auxToolSelector;
    wxString    m_Text;
    bool        m_selectionIsKeyword;

public:
    bool        m_GetExtraFunction;

public:
    // Constructor and destructor
    DIALOG_GET_COMPONENT( EDA_DRAW_FRAME* parent,
                          wxArrayString& HistoryList, const wxString& Title,
                          bool show_extra_tool );
    ~DIALOG_GET_COMPONENT() {};

    /**
     * Function GetComponentName
     * @return the selection (name or keyword)
     */
    wxString GetComponentName( void );

    /**
     * Function IsKeyword
     * @return true if the returned string is a keyword
     */
    bool     IsKeyword( void )
    {
        return m_selectionIsKeyword;
    }

    void     SetComponentName( const wxString& name );

private:
    void     initDialog( wxArrayString& aHistoryList );
    void     OnCancel( wxCommandEvent& event );
    void     Accept( wxCommandEvent& event );
    void     GetExtraSelection( wxCommandEvent& event );
};


#endif /* __INCLUDE_DIALOG_GET_COMPONENT_H__ */
