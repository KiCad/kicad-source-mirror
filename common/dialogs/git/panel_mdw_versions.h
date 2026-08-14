/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef PANEL_MDW_VERSIONS_H
#define PANEL_MDW_VERSIONS_H

#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_compare_doc.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_revision.h>

#include <wx/datetime.h>
#include <wx/panel.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

class EDA_BASE_FRAME;
class GIT_DIFF_WORKSPACE;
class MDW_SOURCE;
class wxButton;
class wxListCtrl;


/**
 * The versions of one item, newest first, and a Compare button that opens two of them in the diff viewer. It reads
 * them from an MDW_SOURCE, so a design in the workspace and a part in a library list and download the same way.
 */
class PANEL_MDW_VERSIONS : public wxPanel
{
public:
    PANEL_MDW_VERSIONS( wxWindow* aParent, wxWindowID aId = wxID_ANY, const wxPoint& aPos = wxDefaultPosition,
                        const wxSize& aSize = wxDefaultSize, long aStyle = wxTAB_TRAVERSAL );

    /**
     * List the versions of aItem on aSource, ending any read or download of the item shown before.
     *
     * The file at aCheckoutPath is listed above the versions as the Working copy when it differs from them. A source
     * that lists local changes under aLocalRoot says so, read once and kept until RefreshChanges(). One that cannot
     * is asked to download its newest version, once per version and save of the file, to compare it with the file.
     *
     * @param aLocalRoot is empty when the source has no working tree.
     * @param aFrame owns the diff viewer Compare opens, and must outlive the panel.
     * @param aMemberName is the symbol or footprint when the item is a library member, which the working copy is
     *                    cut down to and the versions are named by. It is empty for a file.
     */
    void SetItem( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem, const wxString& aLocalRoot,
                  const wxString& aCheckoutPath, EDA_BASE_FRAME* aFrame, const wxString& aMemberName );

    /// Read the source's local changes again, after a listing or a check in made them stale.
    void RefreshChanges();

    /// Forget the item and end any read or download of it.
    void Clear();

private:
    enum class CHANGES
    {
        UNREAD,
        READING,
        KNOWN,
        UNSUPPORTED
    };

    /// Whether the checkout file differed from a version when it was last saved at a time
    struct NEWEST
    {
        wxString   revisionId;
        wxDateTime modified;
        bool       differs = false;
    };

    /// One side of a comparison, the working copy when revisionId is empty
    struct SIDE
    {
        wxString revisionId;
        wxString label;
        wxString path;
    };

    void readHistory();
    void readChanges();
    void forgetChanges();
    void applyWorkingCopy();

    /// True when the checkout file differs from the newest version, which is downloaded once to find out
    bool differsFromNewest();

    void fillList();
    bool hasWorkingCopy() const { return !m_workingCopy.IsEmpty(); }
    SIDE sideAt( long aRow ) const;

    MDW_COMPARE_DOC compareDocument() const;

    /// The rows Compare opens for the selected ones, older first
    std::optional<std::pair<long, long>> comparePair() const;

    void updateCompare();
    void onCompare();

    /// Write the working copy to aPath, as the one member when the item is a symbol of a library
    bool saveWorkingCopy( const wxString& aPath ) const;

    /// Put aSide at its path, then call aDone with the status
    MDW_OP fetch( const SIDE& aSide, std::function<void( const MDW_RESULT& )> aDone );

    wxListCtrl* m_list;
    wxButton*   m_compareButton;

    std::shared_ptr<MDW_SOURCE>         m_source;
    MDW_ITEM_REF                        m_item;
    wxString                            m_localRoot;
    wxString                            m_checkoutPath;
    wxString                            m_memberName;
    wxString                            m_workingCopy; ///< m_checkoutPath when it differs from the versions
    std::set<wxString>                  m_modified;
    CHANGES                             m_changesState = CHANGES::UNREAD;
    EDA_BASE_FRAME*                     m_frame = nullptr;
    std::vector<MDW_REVISION>           m_revisions;
    bool                                m_olderUnread = false; ///< A later page failed or the page cap was reached
    std::vector<wxString>               m_rowKeys; ///< Revision id of each list row, empty for the working copy
    bool                                m_comparing = false;
    MDW_OP                              m_historyOp;
    MDW_OP                              m_changesOp;
    MDW_OP                              m_olderOp;
    MDW_OP                              m_newestOp;
    std::shared_ptr<GIT_DIFF_WORKSPACE> m_newestScratch;
    wxString                            m_newestFor;
    std::map<wxString, NEWEST>          m_newest;
    MDW_OP                              m_newerOp;
};

#endif // PANEL_MDW_VERSIONS_H
