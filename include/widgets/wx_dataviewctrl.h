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

#ifndef WX_DATAVIEWCTRL_H_
#define WX_DATAVIEWCTRL_H_

#include <functional>
#include <utility>

#include <wx/dataview.h>

/**
 * A wxDataViewModelNotifier that runs a callback whenever the model removes items or resets.
 *
 * WX_DATAVIEWCTRL installs one so a deferred EnsureVisible (see
 * WX_DATAVIEWCTRL::CancelPendingEnsureVisible) drops automatically as the model frees items,
 * rather than relying on every model-mutation site to cancel it by hand.
 */
class WX_ENSURE_VISIBLE_CANCELLER : public wxDataViewModelNotifier
{
public:
    explicit WX_ENSURE_VISIBLE_CANCELLER( std::function<void()> aOnItemsRemoved ) :
            m_onItemsRemoved( std::move( aOnItemsRemoved ) )
    {
    }

    bool ItemAdded( const wxDataViewItem&, const wxDataViewItem& ) override { return true; }
    bool ItemChanged( const wxDataViewItem& ) override { return true; }
    bool ValueChanged( const wxDataViewItem&, unsigned int ) override { return true; }
    void Resort() override {}

    bool ItemDeleted( const wxDataViewItem&, const wxDataViewItem& ) override
    {
        m_onItemsRemoved();
        return true;
    }

    bool ItemsDeleted( const wxDataViewItem&, const wxDataViewItemArray& ) override
    {
        m_onItemsRemoved();
        return true;
    }

    // BeforeReset fires while items are still live; Cleared covers AfterReset and direct clears.
    bool BeforeReset() override
    {
        m_onItemsRemoved();
        return true;
    }

    bool Cleared() override
    {
        m_onItemsRemoved();
        return true;
    }

private:
    std::function<void()> m_onItemsRemoved;
};


/**
 * Extension of the wxDataViewCtrl to include some helper functions for working with items.
 *
 * These should probably be sent upstream, since they may be useful to others, but for now
 * just extend the class with them ourselves.
 */
class WX_DATAVIEWCTRL : public wxDataViewCtrl
{
public:
    // Just take all constructors
    using wxDataViewCtrl::wxDataViewCtrl;

    ~WX_DATAVIEWCTRL() override;

    /**
     * Install a WX_ENSURE_VISIBLE_CANCELLER on @p aModel so deferred scrolls are dropped
     * automatically when it frees items, then associate it as usual.
     */
    bool AssociateModel( wxDataViewModel* aModel ) override;

    /**
     * Get the previous item in list order.
     *
     * @param aItem a valid item in the control's model.
     * @return the item before aItem, or an invalid item if aItem is at the top.
     */
    wxDataViewItem GetPrevItem( wxDataViewItem const& aItem );

    /**
     * Get the next item in list order.
     *
     * @param aItem a valid item in the control's model.
     * @return the item after aItem, or an invalid item if aItem is at the bottom.
     */
    wxDataViewItem GetNextItem( wxDataViewItem const& aItem );

    /**
     * Get the previous sibling of an item.
     *
     * @param aItem a valid item in the control's model.
     * @return the sibling before aItem, or an invalid item if aItem has no siblings before it.
     */
    wxDataViewItem GetPrevSibling( wxDataViewItem const& aItem );

    /**
     * Get the next sibling of an item.
     *
     * @param aItem a valid item in the control's model.
     * @return the sibling after aItem, or an invalid item if aItem has no siblings after it.
     */
    wxDataViewItem GetNextSibling( wxDataViewItem const& aItem );

    void DoSetToolTipText( const wxString &tip ) override {}

    void ExpandAll();
    void CollapseAll();

    /**
     * Cancel any pending deferred EnsureVisible request.
     *
     * On GTK, wxDataViewCtrl::EnsureVisible() records the target item and re-scrolls to it
     * during the next idle cycle (in OnInternalIdle).  If the model is rebuilt and the
     * underlying item is freed before that idle runs, OnInternalIdle dereferences the stale
     * pointer in ExpandAncestors and crashes.  The installed WX_ENSURE_VISIBLE_CANCELLER invokes
     * this automatically when the model frees items, so call sites normally do not need to.  This
     * is a no-op on platforms that do not defer the scroll.
     */
    void CancelPendingEnsureVisible();

private:
    // Owned by the associated model; null when no model is associated.
    wxDataViewModelNotifier* m_ensureVisibleCanceller = nullptr;
};


/**
 * Scroll @p aItem into view only when @p aWidget is currently enabled.
 *
 * On GTK, queueing scroll_to_path on a tree whose host frame has been disabled by a
 * quasimodal child dialog races with concurrent model rebuilds and crashes
 * GtkTreeView::validate_visible_area on the next frame-clock tick.  The user can't see
 * the scroll while the dialog is up anyway.  wxWindow::IsEnabled() cascades through
 * WINDOW_DISABLER on the parent frame, so this guard naturally suppresses the racy
 * scroll for any quasimodal child.
 */
inline void EnsureVisibleIfEnabled( wxDataViewCtrl* aWidget, const wxDataViewItem& aItem )
{
    if( aItem.IsOk() && aWidget->IsEnabled() )
        aWidget->EnsureVisible( aItem );
}

#endif // WX_DATAVIEWCTRL_H_
