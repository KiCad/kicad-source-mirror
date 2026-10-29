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

#pragma once

#include <mdw/client/mdw_bom.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_eco.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_review.h>
#include <mdw_bom_compare.h>
#include <widgets/mdw_change_orders.h>

#include <wx/panel.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class MDW_SOURCE;
class WX_GRID;
class wxButton;
class wxCheckBox;
class wxGridEvent;
class wxGridRangeSelectEvent;
class wxHyperlinkCtrl;
class wxHyperlinkEvent;
class wxStaticText;


/**
 * The Symbol Fields Table's read-only Workspace page: the design, grouped as the Edit page
 * groups it, compared with the project's BOM in its workspace source.
 */
class PANEL_SYMBOL_FIELDS_WORKSPACE : public wxPanel
{
public:
    using DESIGN_PROVIDER = std::function<std::vector<std::vector<MDW_DESIGN_SYMBOL>>()>;
    using CROSS_PROBE = std::function<void( const std::vector<wxString>& aSymbolPaths )>;

    /**
     * aProjectState names the project item and the revision the design is based on, empty when unknown.
     * aLink is why it names no item, when it names none.
     */
    PANEL_SYMBOL_FIELDS_WORKSPACE( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                                   const MDW_PROJECT_STATE& aProjectState, const MDW_RESULT& aLink,
                                   const wxString& aProjectName, DESIGN_PROVIDER aDesign, CROSS_PROBE aCrossProbe );

    ~PANEL_SYMBOL_FIELDS_WORKSPACE() override;

    /// Read the workspace BOM the first time the page is shown; later calls only recompare
    void Activate();

    /// The schematic or the Edit page's grouping changed; recompare without a round trip
    void DesignChanged();

    /// The table is closing, so stop every request and stop reading or probing the design
    void Detach();

private:
    enum class STATE
    {
        LOADING,
        LOADED,
        EMPTY,
        UNLINKED,
        OFFLINE,
        DENIED,
        FAILED
    };

    void fetch();

    /// The design's open change orders and review state, shown above the comparison when known
    void     fetchActivity();
    void     fetchEcoActivity();
    void     showActivity();
    void     wrapActivity();
    void     onSize( wxSizeEvent& aEvent );
    void     onActivityLink( wxHyperlinkEvent& aEvent );
    void     onBom( const MDW_RESULT& aResult, const MDW_BOM_DATA& aBom );
    void     resolveParts();
    void     compare();
    void     showState( STATE aState, const wxString& aDetail = wxEmptyString );
    void     fillGrid();

    void onRefresh( wxCommandEvent& aEvent );
    void onRetry( wxHyperlinkEvent& aEvent );
    void onDifferencesOnly( wxCommandEvent& aEvent );
    void onSelectCell( wxGridEvent& aEvent );
    void onRangeSelected( wxGridRangeSelectEvent& aEvent );
    void onUserSelecting( wxGridEvent& aEvent );

    /// Probe every selected row, or the cursor row when none is selected
    void probeSelection();

    std::shared_ptr<MDW_SOURCE> m_source;
    MDW_PROJECT_STATE           m_projectState;
    MDW_RESULT                  m_link;
    wxString                    m_projectName;
    MDW_ITEM_REF                m_project;
    DESIGN_PROVIDER             m_design;
    CROSS_PROBE                 m_crossProbe;

    std::map<wxString, std::vector<MDW_ITEM_REF>> m_candidates; ///< By member, as ResolveParts answered
    std::optional<MDW_BOM_DATA>                   m_bom;
    std::vector<std::vector<MDW_DESIGN_SYMBOL>>   m_designGroups;
    std::vector<MDW_BOM_ROW>                      m_rows;
    std::vector<size_t>                           m_shownRows;  ///< Grid row to m_rows index
    wxDateTime                                    m_checked;
    bool                                          m_activated = false;
    bool                                          m_userSelecting = false; ///< A click or key is moving the selection
    STATE                                         m_state = STATE::LOADING;

    MDW_OP m_op;
    MDW_OP m_ecoOp;
    MDW_OP m_reviewOp;

    wxString m_ecoNote;
    wxString m_reviewNote;
    wxString m_activityText; ///< Unwrapped, since wrapping rewrites the label

    wxStaticText*    m_header;
    wxStaticText*    m_activity;
    wxHyperlinkCtrl* m_activityLink;
    wxButton*        m_refresh;
    wxCheckBox*      m_differencesOnly;
    wxStaticText*    m_summary;
    wxStaticText*    m_message;
    wxHyperlinkCtrl* m_retry;
    WX_GRID*         m_grid;
};
