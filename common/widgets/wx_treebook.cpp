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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/wx_treebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <dialog_shim.h>
#include <scoped_set_reset.h>

#include <utility>


class LAZY_PAGE : public wxPanel
{
public:
    LAZY_PAGE( wxWindow* aParent, std::function<wxWindow*( wxWindow* aParent )> aLazyCtor ) :
            wxPanel( aParent, wxID_ANY ),
            m_lazyCtor( std::move( aLazyCtor ) ),
            m_mainSizer( nullptr ),
            m_contents( nullptr ),
            m_active( false )
    {
        m_mainSizer = new wxBoxSizer( wxVERTICAL );
        SetSizer( m_mainSizer );
    }

    wxWindow* Resolve()
    {
        build();
        m_active = true;
        return m_contents;
    }

    wxWindow* ResolveForSearch()
    {
        if( !m_contents )
        {
            build();

            // Some panels finish building their controls in Show(), e.g. colour swatches
            m_contents->Show( true );
        }

        return m_contents;
    }

    wxWindow* Contents() const { return m_active ? m_contents : nullptr; }

    bool Validate() override { return !m_active || wxPanel::Validate(); }

    bool TransferDataFromWindow() override { return !m_active || wxPanel::TransferDataFromWindow(); }

    bool Show( bool show ) override
    {
        if( show )
            Resolve();

        // m_contents has been created as a child window of LAZY_PAGE, and has been added to
        // LAZY_PAGE's m_mainSizer.  So wxPanel::Show() should call m_contents' Show() method,
        // whether overridden or not.  Only it doesn't, so we call it directly here.
        if( show && m_contents )
            m_contents->Show( true );

        return wxPanel::Show( show );
    }

private:
    void build()
    {
        if( m_contents )
            return;

        m_contents = m_lazyCtor( this );
        m_mainSizer->Add( m_contents, 1, wxEXPAND, 5 );
        m_mainSizer->Layout();

        m_contents->TransferDataToWindow();

        if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
            dlg->SelectAllInTextCtrls( GetChildren() );
    }

    std::function<wxWindow*( wxWindow* aParent )> m_lazyCtor;

    wxSizer*   m_mainSizer;
    wxWindow*  m_contents;
    bool       m_active; ///< Shown or resolved for editing, so it takes part in validation
};


WX_TREEBOOK::WX_TREEBOOK( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                          const wxString& name ) :
        wxTreebook( parent, id, pos, size, style, name ),
        m_resolvingForSearch( false )
{
}


bool WX_TREEBOOK::AddLazyPage( std::function<wxWindow*( wxWindow* aParent )> aLazyCtor,
                               const wxString& text, bool bSelect, int imageId )
{
    return AddPage( new LAZY_PAGE( this, std::move( aLazyCtor ) ), text, bSelect, imageId );
}


bool WX_TREEBOOK::AddLazySubPage( std::function<wxWindow*( wxWindow* aParent )> aLazyCtor,
                                  const wxString& text, bool bSelect, int imageId )
{
    return AddSubPage( new LAZY_PAGE( this, std::move( aLazyCtor ) ), text, bSelect, imageId );
}


bool WX_TREEBOOK::InsertLazySubPage( size_t aPos, std::function<wxWindow*( wxWindow* aParent )> aLazyCtor,
                                     const wxString& text, bool bSelect, int imageId )
{
    return InsertSubPage( aPos, new LAZY_PAGE( this, std::move( aLazyCtor ) ), text, bSelect, imageId );
}


wxWindow* WX_TREEBOOK::ResolvePage( size_t aPage )
{
    wxWindow* page = GetPage( aPage );

    if( LAZY_PAGE* lazyPage = dynamic_cast<LAZY_PAGE*>( page ) )
        return m_resolvingForSearch ? lazyPage->ResolveForSearch() : lazyPage->Resolve();

    return page;
}


wxWindow* WX_TREEBOOK::ResolvePageForSearch( size_t aPage )
{
    // Pages resolved by another page's constructor must stay passive too.
    SCOPED_SET_RESET resolvingForSearch( m_resolvingForSearch, true );

    return ResolvePage( aPage );
}


wxWindow* WX_TREEBOOK::GetResolvedPage( size_t aPage )
{
    wxWindow* page = GetPage( aPage );

    if( LAZY_PAGE* lazyPage = dynamic_cast<LAZY_PAGE*>( page ) )
        return lazyPage->Contents();

    return page;
}
