/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

class FOOTPRINT_PREVIEW_PANEL;
class COMPONENT_TREE_SEARCH_CONTAINER;
class LIB_ALIAS;
class LIB_PART;
class wxTreeListItem;
class SCH_BASE_FRAME;

class DIALOG_CHOOSE_COMPONENT : public DIALOG_CHOOSE_COMPONENT_BASE
{
    SCH_BASE_FRAME* m_parent;
    COMPONENT_TREE_SEARCH_CONTAINER* const m_search_container;
    int             m_deMorganConvert;
    bool            m_external_browser_requested;
    bool            m_received_doubleclick_in_tree;

public:
    /**
     * Create dialog to choose component.
     *
     * @param aParent          a SCH_BASE_FRAME parent window.
     * @param aTitle           Dialog title.
     * @param aSearchContainer The tree selection search container. Needs to be pre-populated
     *                         This dialog does not take over ownership of this object.
     * @param aDeMorganConvert preferred deMorgan conversion (TODO: should happen in dialog)
     */
    DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                             COMPONENT_TREE_SEARCH_CONTAINER* const aSearchContainer,
                             int aDeMorganConvert );
    virtual ~DIALOG_CHOOSE_COMPONENT();
    void OnInitDialog( wxInitDialogEvent& event ) override;

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
    virtual void OnSearchBoxChange( wxCommandEvent& aEvent ) override;
    virtual void OnSearchBoxEnter( wxCommandEvent& aEvent ) override;
    virtual void OnInterceptSearchBoxKey( wxKeyEvent& aEvent ) override;

    virtual void OnTreeSelect( wxTreeListEvent& aEvent ) override;
    virtual void OnDoubleClickTreeActivation( wxTreeListEvent& aEvent ) override;
    virtual void OnInterceptTreeEnter( wxKeyEvent& aEvent ) override;
    virtual void OnTreeMouseUp( wxMouseEvent& aMouseEvent ) override;

    virtual void OnStartComponentBrowser( wxMouseEvent& aEvent ) override;
    virtual void OnHandlePreviewRepaint( wxPaintEvent& aRepaintEvent ) override;
    virtual void OnDatasheetClick( wxHtmlLinkEvent& aEvent ) override;

private:
    bool updateSelection();
    void selectIfValid( const wxTreeListItem& aTreeId );
    void renderPreview( LIB_PART*      aComponent, int aUnit );

    void updateFootprint();

    FOOTPRINT_PREVIEW_PANEL* m_footprintPreviewPanel;
};

#endif /* DIALOG_CHOOSE_COMPONENT_H */
