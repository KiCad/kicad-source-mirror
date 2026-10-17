/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DB_LIB_PREVIEW_PANEL_H
#define DB_LIB_PREVIEW_PANEL_H

#include <map>
#include <memory>
#include <vector>

#include <wx/panel.h>

#include <class_draw_panel_gal.h>
#include <lib_tree_model_adapter.h>
#include <settings/app_settings.h>
#include <widgets/wx_debounced_action.h>

class EDA_BASE_FRAME;
class FOOTPRINT_PREVIEW_WIDGET;
class KIWAY;
class LIB_SYMBOL;
class LIB_TREE;
class SYMBOL_PREVIEW_WIDGET;
class wxChoice;
class wxStaticText;


/**
 * Shows symbols that are not in any library table the way the symbol chooser does: a searchable
 * library tree, symbol and footprint previews, and the details pane.  Used to preview a database
 * library while its settings are being edited.
 */
class DB_LIB_PREVIEW_PANEL : public wxPanel
{
public:
    /**
     * @param aFrame owns the tree's tool dispatcher; may be null
     * @param aKiway enables the footprint preview; may be null
     */
    DB_LIB_PREVIEW_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame, KIWAY* aKiway,
                          EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    ~DB_LIB_PREVIEW_PANEL() override;

    /**
     * Replaces the previewed library, keeping the selected symbol when it still exists.
     * Every symbol's LIB_ID nickname is set to aLibNickname so tree lookups resolve.
     */
    void SetSymbols( const wxString& aLibNickname, std::vector<std::unique_ptr<LIB_SYMBOL>> aSymbols );

    /// Shows a one-line status below the library tree
    void SetStatus( const wxString& aText );

    void SelectSymbol( const wxString& aSymbolName );

private:
    void onSymbolSelected( wxCommandEvent& aEvent );
    void onFootprintSelected( wxCommandEvent& aEvent );

    void showSymbol( const LIB_SYMBOL* aSymbol, int aUnit );
    void showFootprint( const wxString& aFootprint );

    const LIB_SYMBOL* findSymbol( const LIB_ID& aLibId ) const;

    APP_SETTINGS_BASE::LIB_TREE             m_treeSettings;
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;
    LIB_TREE*                               m_tree;
    SYMBOL_PREVIEW_WIDGET*                  m_symbolPreview;
    wxChoice*                               m_footprintChoice;
    FOOTPRINT_PREVIEW_WIDGET*               m_footprintPreview;
    wxStaticText*                           m_status;
    KIWAY*                                  m_kiway;

    /// Polls the footprint library load, which DisplayFootprint() needs finished
    WX_DEBOUNCED_ACTION                     m_footprintLoadRetry;

    wxString                                         m_nickname;
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_symbols;
};

#endif // DB_LIB_PREVIEW_PANEL_H
