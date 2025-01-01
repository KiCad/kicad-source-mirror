/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <dialog_bom_base.h>
#include <vector>

class SCH_EDIT_FRAME;
class BOM_GENERATOR_HANDLER;
class HTML_MESSAGE_BOX;

// BOM "plugins" are not actually plugins. They are external tools
// (scripts or executables) called by this dialog.
typedef std::vector<std::unique_ptr<BOM_GENERATOR_HANDLER>> BOM_GENERATOR_ARRAY;

// The main dialog frame to run scripts to build bom
class DIALOG_BOM : public DIALOG_BOM_BASE
{
private:
    SCH_EDIT_FRAME*     m_parent;
    BOM_GENERATOR_ARRAY m_generators;
    bool                m_initialized;

    HTML_MESSAGE_BOX*   m_helpWindow;

public:
    DIALOG_BOM( SCH_EDIT_FRAME* parent );
    ~DIALOG_BOM();

private:
    void OnGeneratorSelected( wxCommandEvent& event ) override;
    void OnRunGenerator( wxCommandEvent& event ) override;
    void OnHelp( wxCommandEvent& event ) override;
    void OnAddGenerator( wxCommandEvent& event ) override;
    void OnRemoveGenerator( wxCommandEvent& event ) override;
    void OnEditGenerator( wxCommandEvent& event ) override;
    void OnCommandLineEdited( wxCommandEvent& event ) override;
    void OnNameEdited( wxCommandEvent& event ) override;
    void OnShowConsoleChanged( wxCommandEvent& event ) override;
    void OnIdle( wxIdleEvent& event ) override;

    void pluginInit();
    void installGeneratorsList();
    BOM_GENERATOR_HANDLER* addGenerator( const wxString& aPath,
                                         const wxString& aName = wxEmptyString );
    bool pluginExists( const wxString& aName );

    BOM_GENERATOR_HANDLER* selectedGenerator();

    wxString chooseGenerator();
};
