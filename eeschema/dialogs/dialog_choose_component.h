/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014 KiCad Developers, see change_log.txt for contributors.
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
#ifndef DIALOG_CHOOSE_COMPONENT_H
#define DIALOG_CHOOSE_COMPONENT_H

#include <dialog_choose_component_base.h>

class COMPONENT_TREE_SEARCH_CONTAINER;
class LIB_COMPONENT;
class wxTreeItemId;

class DIALOG_CHOOSE_COMPONENT : public DIALOG_CHOOSE_COMPONENT_BASE
{
public:
    DIALOG_CHOOSE_COMPONENT( wxWindow* aParent, const wxString& aTitle,
                             COMPONENT_TREE_SEARCH_CONTAINER* aSearch_container,
                             int aDeMorganConvert );

    /** Function GetSelectedAliasName
     * To be called after this dialog returns from ShowModal().
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the alias that has been selected, or an empty string if there is none.
     */
    wxString GetSelectedAliasName( int* aUnit ) const;

    /** Function IsExternalBrowserSelected
     *
     * @return true, iff the browser pressed the browsing button.
     */
    bool IsExternalBrowserSelected() const { return m_external_browser_requested; }

protected:
    virtual void OnSearchBoxChange( wxCommandEvent& aEvent );
    virtual void OnSearchBoxEnter( wxCommandEvent& aEvent );
    virtual void OnInterceptSearchBoxKey( wxKeyEvent& aEvent );

    virtual void OnTreeSelect( wxTreeEvent& aEvent );
    virtual void OnDoubleClickTreeSelect( wxTreeEvent& aEvent );
    virtual void OnTreeMouseUp( wxMouseEvent& aMouseEvent );

    virtual void OnStartComponentBrowser( wxMouseEvent& aEvent );
    virtual void OnHandlePreviewRepaint( wxPaintEvent& aRepaintEvent );

private:
    bool updateSelection();
    void selectIfValid( const wxTreeItemId& aTreeId );
    void renderPreview( LIB_COMPONENT* aComponent, int aUnit );

    COMPONENT_TREE_SEARCH_CONTAINER* const m_search_container;
    const int m_deMorganConvert;
    bool m_external_browser_requested;
    bool m_received_doubleclick_in_tree;
};

#endif /* DIALOG_CHOOSE_COMPONENT_H */
