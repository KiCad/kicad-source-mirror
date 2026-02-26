/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_CUSTOM_RULE_PANEL_H
#define DRC_RE_CUSTOM_RULE_PANEL_H

#include <memory>

#include <wx/tipwin.h>
#include <wx/panel.h>
#include <wx/stc/stc.h>

#include "drc_re_content_panel_base.h"
#include "drc_re_custom_rule_constraint_data.h"

class SCINTILLA_TRICKS;
class wxButton;

/**
 * Simple panel used for editing custom rule text.  The panel consists of a
 * single wxStyledTextCtrl allowing free-form rule entry.
 */
class DRC_RE_CUSTOM_RULE_PANEL : public wxPanel, public DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    DRC_RE_CUSTOM_RULE_PANEL( wxWindow* aParent, std::shared_ptr<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA> aConstraintData );
    ~DRC_RE_CUSTOM_RULE_PANEL() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;
    void     UpdateRuleName( const wxString& aName );

private:
    void onScintillaCharAdded( wxStyledTextEvent& aEvent );
    void onCheckSyntax( wxCommandEvent& aEvent );

    std::shared_ptr<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA> m_constraintData;
    wxStyledTextCtrl*                                   m_textCtrl;
    wxButton*                                           m_checkSyntaxBtn;

#if wxCHECK_VERSION( 3, 3, 2 )
    wxTipWindow::Ref m_tipWindow;
#else
    wxTipWindow* m_tipWindow = nullptr;
#endif

    std::unique_ptr<SCINTILLA_TRICKS>                   m_scintillaTricks;
};

#endif // DRC_RE_CUSTOM_RULE_PANEL_H
