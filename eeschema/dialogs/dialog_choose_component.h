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
class LIB_ALIAS;
class LIB_PART;
class wxTreeItemId;

class DIALOG_CHOOSE_COMPONENT : public DIALOG_CHOOSE_COMPONENT_BASE
{
public:
    /**
     * Create dialog to choose component.
     *
     * @param aParent          Parent window.
     * @param aTitle           Dialog title.
     * @param aSearchContainer The tree selection search container. Needs to be pre-populated
     *                         This dialog does not take over ownership of this object.
     * @param aDeMorganConvert preferred deMorgan conversion (TODO: should happen in dialog)
     */
    DIALOG_CHOOSE_COMPONENT( wxWindow* aParent, const wxString& aTitle,
                             COMPONENT_TREE_SEARCH_CONTAINER* aSearchContainer,
                             int aDeMorganConvert );
    virtual ~DIALOG_CHOOSE_COMPONENT();

    /** Function GetSelectedAlias
     * To be called after this dialog returns from ShowModal().
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the alias that has been selected, or NULL if there is none.
     */
    LIB_ALIAS* GetSelectedAlias( int* aUnit ) const;

    /** Function IsExternalBrowserSelected
     *
     * @return true, iff the user pressed the thumbnail view of the component to
     *               launch the component browser.
     */
    bool IsExternalBrowserSelected() const { return m_external_browser_requested; }

protected:
    virtual void OnSearchBoxChange( wxCommandEvent& aEvent );
    virtual void OnSearchBoxEnter( wxCommandEvent& aEvent );
    virtual void OnInterceptSearchBoxKey( wxKeyEvent& aEvent );

    virtual void OnTreeSelect( wxTreeEvent& aEvent );
    virtual void OnDoubleClickTreeActivation( wxTreeEvent& aEvent );
    virtual void OnInterceptTreeEnter( wxKeyEvent& aEvent );
    virtual void OnTreeMouseUp( wxMouseEvent& aMouseEvent );

    virtual void OnStartComponentBrowser( wxMouseEvent& aEvent );
    virtual void OnHandlePreviewRepaint( wxPaintEvent& aRepaintEvent );

private:
    bool updateSelection();
    void selectIfValid( const wxTreeItemId& aTreeId );
    void renderPreview( LIB_PART*      aComponent, int aUnit );

    COMPONENT_TREE_SEARCH_CONTAINER* const m_search_container;
    const int m_deMorganConvert;
    bool m_external_browser_requested;
    bool m_received_doubleclick_in_tree;
};

#endif /* DIALOG_CHOOSE_COMPONENT_H */
