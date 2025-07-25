/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_MANAGE_REPOSITORIES_H_
#define DIALOG_MANAGE_REPOSITORIES_H_

#include "dialog_manage_repositories_base.h"
#include "pcm.h"
#include <memory>
#include <vector>


/** Implementing DIALOG_MANAGE_REPOSITORIES_BASE */
class DIALOG_MANAGE_REPOSITORIES : public DIALOG_MANAGE_REPOSITORIES_BASE
{
protected:
    // Handlers for DIALOG_MANAGE_REPOSITORIES_BASE events.
    void OnRemoveButtonClicked( wxCommandEvent& event ) override;
    void OnMoveUpButtonClicked( wxCommandEvent& event ) override;
    void OnMoveDownButtonClicked( wxCommandEvent& event ) override;
    void OnGridCellClicked( wxGridEvent& event ) override;
    void OnSaveClicked( wxCommandEvent& event ) override;

public:
    /** Constructor */
    DIALOG_MANAGE_REPOSITORIES( wxWindow* parent, std::shared_ptr<PLUGIN_CONTENT_MANAGER> aPcm );

    ~DIALOG_MANAGE_REPOSITORIES();

    void SetData( const std::vector<std::pair<wxString, wxString>>& aData );

    void OnAdd( wxCommandEvent& event );

    void OnAddDefault( wxCommandEvent& event );

    std::vector<std::pair<wxString, wxString>> GetData();

private:
    void selectRow( int aRow );
    void setColumnWidths();
    void addRepository( const wxString& aUrl );
    int  findRow( int aCol, const wxString& aVal );

    std::shared_ptr<PLUGIN_CONTENT_MANAGER> m_pcm;
};

#endif // DIALOG_MANAGE_REPOSITORIES_H_
