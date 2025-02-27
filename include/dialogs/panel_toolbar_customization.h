/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

 #ifndef PANEL_TOOLBAR_CUSTOMIZATION_H_
 #define PANEL_TOOLBAR_CUSTOMIZATION_H_

#include <dialogs/panel_toolbar_customization_base.h>

#include <tool/action_toolbar.h>

#include <wx/bmpbndl.h>

class wxImageList;

class APP_SETTINGS_BASE;
class TOOL_ACTION;
class TOOLBAR_SETTINGS;

class PANEL_TOOLBAR_CUSTOMIZATION : public PANEL_TOOLBAR_CUSTOMIZATION_BASE
{
public:
    PANEL_TOOLBAR_CUSTOMIZATION( wxWindow* aParent, APP_SETTINGS_BASE* aCfg, TOOLBAR_SETTINGS* aTbSettings,
                                 std::vector<TOOL_ACTION*> aTools,
                                 std::vector<ACTION_TOOLBAR_CONTROL*> aControls );

    ~PANEL_TOOLBAR_CUSTOMIZATION();

    void ResetPanel() override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

protected:
    void parseToolbarTree( TOOLBAR_CONFIGURATION& aToolbar );

    void populateToolbarTree( const TOOLBAR_CONFIGURATION& aToolbar );

    void populateActions( const std::map<std::string, TOOL_ACTION*>& aTools,
                          const std::map<std::string, ACTION_TOOLBAR_CONTROL*>& aControls );

    void enableCustomControls( bool enable );

    void onGroupPress( wxCommandEvent& aEvent );
    void onSpacerPress( wxCommandEvent& aEvent );
    void onSeparatorPress( wxCommandEvent& aEvent );

    // From the base class
    void onCustomizeTbCb( wxCommandEvent& event ) override;
    void onToolDelete( wxCommandEvent& event ) override;
    void onToolMoveUp( wxCommandEvent& event ) override;
    void onToolMoveDown( wxCommandEvent& event ) override;
    void onBtnAddAction( wxCommandEvent& event ) override;
    void onTreeBeginLabelEdit( wxTreeEvent& event ) override;
    void onTreeEndLabelEdit( wxTreeEvent& event ) override;

protected:
    wxImageList*               m_actionImageList;
    wxVector<wxBitmapBundle>   m_actionImageBundleVector;
    std::map<std::string, int> m_actionImageListMap;

    APP_SETTINGS_BASE* m_appSettings;
    TOOLBAR_SETTINGS*  m_tbSettings;

    std::map<std::string, TOOL_ACTION*>               m_availableTools;
    std::map<std::string, ACTION_TOOLBAR_CONTROL*>    m_availableControls;
};

 #endif /* PANEL_TOOLBAR_CUSTOMIZATION_H_ */
