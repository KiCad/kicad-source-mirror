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

#pragma once

#include "panel_fab_export_content_base.h"

#include <functional>
#include <optional>
#include <vector>

#include <dialogs/dialog_export_2581_bom.h>
#include <exporters/fab_model/fab_sections.h>

class BOARD;

enum class FAB_CONTENT_FORMAT
{
    IPC2581,
    ODBPP
};


class PANEL_FAB_EXPORT_CONTENT : public PANEL_FAB_EXPORT_CONTENT_BASE
{
public:
    PANEL_FAB_EXPORT_CONTENT( wxWindow* aParent, wxWindowID aId = wxID_ANY, const wxPoint& aPos = wxDefaultPosition,
                              const wxSize& aSize = wxDefaultSize, long aStyle = wxTAB_TRAVERSAL,
                              const wxString& aName = wxEmptyString );

    void Configure( FAB_CONTENT_FORMAT aFormat, BOARD* aBoard );
    void SetContentChanged( std::function<void()> aCallback ) { m_contentChanged = std::move( aCallback ); }

    IPC2581::MODE                  GetDataSet() const;
    void                           SetDataSet( IPC2581::MODE aMode );
    wxString                       GetNetNamePolicy() const;
    void                           SetNetNamePolicy( const wxString& aPolicy );
    wxString                       GetRefDesPolicy() const;
    void                           SetRefDesPolicy( const wxString& aPolicy );
    const std::optional<wxString>& GetSectionKey() const { return m_sectionKey; }
    void                           SetSectionKey( const std::optional<wxString>& aKey );
    std::vector<wxString>          GetVariantNames() const;
    void                           SetVariantNames( const std::vector<wxString>& aNames );
    bool                           IsCombinedVariantOutput() const { return m_variantOutput->GetSelection() == 1; }
    void                           SetCombinedVariantOutput( bool aCombined );
    const IPC2581_BOM_FIELDS&      GetBomFields() const { return m_bomFields; }
    void                           SetBomFields( const IPC2581_BOM_FIELDS& aFields ) { m_bomFields = aFields; }

    IPC2581::SECTION_SET ResolvedSections() const;

private:
    void onDataSetChange( wxCommandEvent& aEvent ) override;
    void onVariantChange( wxCommandEvent& aEvent ) override;
    void onVariantOutputChange( wxCommandEvent& aEvent ) override;
    void onCustomizeClick( wxCommandEvent& aEvent ) override;
    void onBomFieldsClick( wxCommandEvent& aEvent ) override;
    /// Set the Includes line and BOM Fields button from the function mode
    void updateSummary();
    void updateVariantOutput();
    void changed();

    FAB_CONTENT_FORMAT         m_format = FAB_CONTENT_FORMAT::IPC2581;
    BOARD*                     m_board = nullptr;
    IPC2581_BOM_FIELDS         m_bomFields;
    std::vector<IPC2581::MODE> m_modes;
    std::vector<wxString>      m_selectedVariants;
    /// Set after Customize  An empty IPC-2581 key is a true empty selection
    std::optional<wxString>    m_sectionKey;
    std::function<void()>      m_contentChanged;
    int                        m_selectedChoice = 0;
};
