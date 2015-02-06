/**
 * @file dialog_edit_component_in_lib.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_EDIT_COMPONENT_IN_LIB_H_
#define _DIALOG_EDIT_COMPONENT_IN_LIB_H_

#include <dialog_edit_component_in_lib_base.h>


class DIALOG_EDIT_COMPONENT_IN_LIBRARY: public DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE
{
    static int m_lastOpenedPage;    // To remember the last notebook selection

public:
    LIB_EDIT_FRAME* m_Parent;
    bool m_RecreateToolbar;
    int m_AliasLocation;

public:
    /// Constructors
    DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* parent);
    ~DIALOG_EDIT_COMPONENT_IN_LIBRARY();

private:
    void initDlg();
    void InitPanelDoc();
    void InitBasicPanel();
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick(wxCommandEvent& event);
    void DeleteAllAliasOfPart(wxCommandEvent& event);
    void DeleteAliasOfPart(wxCommandEvent& event);
    void AddAliasOfPart(wxCommandEvent& event);
    bool ChangeNbUnitsPerPackage(int newUnit);
    bool SetUnsetConvert();
    void CopyDocFromRootToAlias(wxCommandEvent& event);
    void BrowseAndSelectDocFile(wxCommandEvent& event);

    void DeleteAllFootprintFilter(wxCommandEvent& event);
    void DeleteOneFootprintFilter(wxCommandEvent& event);
    void AddFootprintFilter(wxCommandEvent& event);
	void EditOneFootprintFilter( wxCommandEvent& event );
};

#endif
    // _DIALOG_EDIT_COMPONENT_IN_LIB_H_
