/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#ifndef PANEL_SETUP_RULES_H
#define PANEL_SETUP_RULES_H

#include <wx/regex.h>
#include <panel_setup_rules_base.h>

class DRC;
class PAGED_DIALOG;
class PCB_EDIT_FRAME;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;


class PANEL_SETUP_RULES : public PANEL_SETUP_RULES_BASE
{
public:
    PANEL_SETUP_RULES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_RULES( ) override;

    void ImportSettingsFrom( BOARD* aBoard );

private:
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    void OnSyntaxHelp( wxHyperlinkEvent& aEvent ) override;
    void OnCompile( wxCommandEvent& event ) override;
    void OnErrorLinkClicked( wxHtmlLinkEvent& event ) override;
    void onCharHook( wxKeyEvent& aEvent );
    void OnContextMenu( wxMouseEvent& event ) override;

    void checkPlausibility( const std::vector<std::shared_ptr<DRC_RULE>>& aRules );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME*   m_frame;
    SCINTILLA_TRICKS* m_scintillaTricks;
    wxString          m_originalText;

    wxRegEx           m_netClassRegex;
    wxRegEx           m_netNameRegex;
    wxRegEx           m_typeRegex;
    wxRegEx           m_viaTypeRegex;
    wxRegEx           m_padTypeRegex;
    wxRegEx           m_pinTypeRegex;
    wxRegEx           m_fabPropRegex;
    wxRegEx           m_shapeRegex;
    wxRegEx           m_padShapeRegex;
    wxRegEx           m_padConnectionsRegex;
    wxRegEx           m_zoneConnStyleRegex;
    wxRegEx           m_lineStyleRegex;
    wxRegEx           m_hJustRegex;
    wxRegEx           m_vJustRegex;

    HTML_MESSAGE_BOX* m_helpWindow;
};

#endif //PANEL_SETUP_RULES_H
