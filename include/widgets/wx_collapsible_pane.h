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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_WX_COLLAPSIBLE_PANE_H
#define KICAD_WX_COLLAPSIBLE_PANE_H

#include <wx/control.h>
#include <wx/containr.h>
#include <wx/statbox.h> // needed to provide a hint that wx libraries instantiated
                        // wxNavigationEnabled<wxControl>


wxDECLARE_EVENT( WX_COLLAPSIBLE_PANE_HEADER_CHANGED, wxCommandEvent );
wxDECLARE_EVENT( WX_COLLAPSIBLE_PANE_CHANGED, wxCommandEvent );

/**
 * A header control for #WX_COLLAPSIBLE_PANE.
 *
 * Looks like a static text with a unicode arrow prepended to show the state.
 * Takes the same space as a static text.  This is similar to the wxCollapsiblePane on GTK.
 */
class WX_COLLAPSIBLE_PANE_HEADER : public wxControl
{
public:
    WX_COLLAPSIBLE_PANE_HEADER()
    {
        init();
    }

    WX_COLLAPSIBLE_PANE_HEADER( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                                const wxPoint& aPos = wxDefaultPosition,
                                const wxSize& aSize = wxDefaultSize, long aStyle = wxBORDER_NONE,
                                const wxValidator& aValidator = wxDefaultValidator,
                                const wxString& aName = wxT( "COLLAPSIBLE_PANE_HEADER" ) )
    {
        init();

        Create( aParent, aId, aLabel, aPos, aSize, aStyle, aValidator, aName );
    }

    bool Create( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                 const wxPoint& aPos = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize, long aStyle = wxBORDER_NONE,
                 const wxValidator& aValidator = wxDefaultValidator,
                 const wxString& aName = wxT( "COLLAPSIBLE_PANE_HEADER" ) );

    void SetCollapsed( bool aCollapsed = true );

    bool IsCollapsed() const
    {
        return m_collapsed;
    }

protected:

    wxSize DoGetBestClientSize() const override;

private:
    wxString m_label;
    bool     m_collapsed;
    bool     m_inWindow;

    void init();

    void onPaint( wxPaintEvent& aEvent );

    void onFocus( wxFocusEvent& aEvent );

    void onEnterWindow( wxMouseEvent& aEvent );

    void onLeaveWindow( wxMouseEvent& aEvent );

    void onLeftUp( wxMouseEvent& aEvent );

    void onChar( wxKeyEvent& aEvent );

    void doSetCollapsed( bool aCollapsed );

    void drawArrow( wxDC& aDC, wxRect aRect, bool aIsActive );
};


/**
 * A better wxCollapsiblePane that
 * - Looks the same on all platforms.
 * - Doesn't have the same sizer bugs.
 * - Uses proper window colors.
 */
class WX_COLLAPSIBLE_PANE : public wxNavigationEnabled<wxControl>
{
public:
    WX_COLLAPSIBLE_PANE()
    {
        init();
    }

    WX_COLLAPSIBLE_PANE( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                         const wxPoint& aPos = wxDefaultPosition,
                         const wxSize& aSize = wxDefaultSize, long aStyle = wxBORDER_NONE,
                         const wxValidator& aValidator = wxDefaultValidator,
                         const wxString& aName = wxT( "COLLAPSIBLE_PANE_HEADER" ) )
    {
        init();

        Create( aParent, aId, aLabel, aPos, aSize, aStyle, aValidator, aName );
    }

    ~WX_COLLAPSIBLE_PANE();

    bool Create( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                 const wxPoint& aPos = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize, long aStyle = wxBORDER_NONE,
                 const wxValidator& aValidator = wxDefaultValidator,
                 const wxString& aName = wxT( "COLLAPSIBLE_PANE_HEADER" ) );

    void Collapse( bool aCollapse = true );

    void Expand()
    {
        Collapse( false );
    }

    bool IsCollapsed() const;

    bool IsExpanded() const { return !IsCollapsed(); }

    wxWindow* GetPane()
    {
        return m_pane;
    }

    wxString GetLabel() const override
    {
        return m_header->GetLabel();
    }

    void SetLabel( const wxString& aLabel ) override;

    bool SetBackgroundColour( const wxColour& aColor ) override;

    bool InformFirstDirection( int aDirection, int aSize, int aAvailableOtherDir ) override;

    wxSize DoGetBestClientSize() const override;

    bool Layout() override;

private:
    wxWindow*                   m_pane;
    wxSizer*                    m_sizer;
    WX_COLLAPSIBLE_PANE_HEADER* m_header;

    void init();

    int getBorder() const;

    void onSize( wxSizeEvent& aEvent );

    void onHeaderClicked( wxCommandEvent& aEvent );
};

#endif // KICAD_WX_COLLAPSIBLE_PANE_H
