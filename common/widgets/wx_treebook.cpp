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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/wx_treebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <dialog_shim.h>


class LAZY_PAGE : public wxPanel
{
public:
    LAZY_PAGE( wxWindow* aParent, std::function<wxWindow*( wxWindow* aParent )> aLazyCtor ) :
            wxPanel( aParent, wxID_ANY ),
            m_lazyCtor( std::move( aLazyCtor ) ),
            m_mainSizer( nullptr ),
            m_contents( nullptr )
    {
        m_mainSizer = new wxBoxSizer( wxVERTICAL );
        SetSizer( m_mainSizer );
    }

    wxWindow* Resolve()
    {
        if( !m_contents )
        {
            m_contents = m_lazyCtor( this );
            m_mainSizer->Add( m_contents, 1, wxEXPAND, 5 );
            m_mainSizer->Layout();

            m_contents->TransferDataToWindow();

            if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
                dlg->SelectAllInTextCtrls( GetChildren() );
        }

        return m_contents;
    }

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
    std::function<wxWindow*( wxWindow* aParent )> m_lazyCtor;

    wxSizer*   m_mainSizer;
    wxWindow*  m_contents;
};


WX_TREEBOOK::WX_TREEBOOK( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                          long style, const wxString& name ) :
        wxTreebook( parent, id, pos, size, style, name )
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


wxWindow* WX_TREEBOOK::ResolvePage( size_t aPage )
{
    wxWindow* page = GetPage( aPage );

    if( LAZY_PAGE* lazyPage = dynamic_cast<LAZY_PAGE*>( page ) )
        return lazyPage->Resolve();

    return page;
}
