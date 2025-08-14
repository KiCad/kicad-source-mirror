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

#include <wx/dcclient.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/listbox.h>
#include <wx/dataview.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/srchctrl.h>
#include <wx/stc/stc.h>
#include <wx/scrolbar.h>
#include <wx/grid.h>
#include <widgets/ui_common.h>

#include <algorithm>
#include <dialog_shim.h>
#include <pgm_base.h>
#include <wx/settings.h>
#include <settings/common_settings.h>
#include <bitmaps/bitmap_types.h>
#include <string_utils.h>
#include <wx/hyperlink.h>


const wxString KIUI::s_FocusStealableInputName = wxS( "KI_NOFOCUS" );


int KIUI::GetStdMargin()
{
    // This is the value used in (most) wxFB dialogs
    return 5;
}


SEVERITY SeverityFromString( const wxString& aSeverity )
{
    if( aSeverity == wxT( "warning" ) )
        return RPT_SEVERITY_WARNING;
    else if( aSeverity == wxT( "ignore" ) )
        return RPT_SEVERITY_IGNORE;
    else
        return RPT_SEVERITY_ERROR;
}


wxString SeverityToString( const SEVERITY& aSeverity )
{
    if( aSeverity == RPT_SEVERITY_IGNORE )
        return wxT( "ignore" );
    else if( aSeverity == RPT_SEVERITY_WARNING )
        return wxT( "warning" );
    else
        return wxT( "error" );
}


wxSize KIUI::GetTextSize( const wxString& aSingleLine, wxWindow* aWindow )
{
    wxCoord width;
    wxCoord height;

    {
        wxClientDC dc( aWindow );
        dc.SetFont( aWindow->GetFont() );
        dc.GetTextExtent( aSingleLine, &width, &height );
    }

    return wxSize( width, height );
}


wxFont KIUI::GetMonospacedUIFont()
{
    static int guiFontSize = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();

    wxFont font( guiFontSize, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

#ifdef __WXMAC__
    // https://trac.wxwidgets.org/ticket/19210
    if( font.GetFaceName().IsEmpty() )
        font.SetFaceName( wxS( "Menlo" ) );
#endif

    return font;
}


wxFont getGUIFont( wxWindow* aWindow, int aRelativeSize )
{
    wxFont font = aWindow->GetFont();

    font.SetPointSize( font.GetPointSize() + aRelativeSize );

    if( Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Appearance.apply_icon_scale_to_fonts )
        font.SetPointSize( KiROUND( KiIconScale( aWindow ) * font.GetPointSize() / 4.0 ) );

#ifdef __WXMAC__
    // https://trac.wxwidgets.org/ticket/19210
    if( font.GetFaceName().IsEmpty() )
        font.SetFaceName( wxS( "San Francisco" ) );

    // OSX 10.1 .. 10.9: Lucida Grande
    // OSX 10.10:        Helvetica Neue
    // OSX 10.11 .. :    San Francisco
#endif

    return font;
}


wxFont KIUI::GetStatusFont( wxWindow* aWindow )
{
#ifdef __WXMAC__
    int scale = -2;
#else
    int scale = 0;
#endif

    return getGUIFont( aWindow, scale );
}


wxFont KIUI::GetDockedPaneFont( wxWindow* aWindow )
{
#ifdef __WXMAC__
    int scale = -1;
#else
    int scale = 0;
#endif

    return getGUIFont( aWindow, scale );
}


wxFont KIUI::GetInfoFont( wxWindow* aWindow )
{
    return getGUIFont( aWindow, -1 );
}


wxFont KIUI::GetSmallInfoFont( wxWindow* aWindow )
{
    return getGUIFont( aWindow, - 2 );
}


wxFont KIUI::GetControlFont( wxWindow* aWindow )
{
    return getGUIFont( aWindow, 0 );
}


bool KIUI::EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString )
{
    wxWindow* window = aCtrl->GetParent();

    if( !window )
        window = aCtrl;

    wxString ctrlText;

    if( !aString )
    {
        ctrlText = aCtrl->GetValue();
        aString  = &ctrlText;
    }

    wxSize textz = GetTextSize( *aString, window );
    wxSize ctrlz = aCtrl->GetSize();

    if( ctrlz.GetWidth() < textz.GetWidth() + 10 )
    {
        ctrlz.SetWidth( textz.GetWidth() + 10 );
        aCtrl->SetSizeHints( ctrlz );
        return true;
    }

    return false;
}


wxString KIUI::EllipsizeStatusText( wxWindow* aWindow, const wxString& aString )
{
    wxString msg = UnescapeString( aString );

    msg.Replace( wxT( "\n" ), wxT( " " ) );
    msg.Replace( wxT( "\r" ), wxT( " " ) );
    msg.Replace( wxT( "\t" ), wxT( " " ) );

    wxClientDC dc( aWindow );
    int        statusWidth = aWindow->GetSize().GetWidth();

    // 30% of the first 800 pixels plus 60% of the remaining width
    int textWidth = std::min( statusWidth, 800 ) * 0.3 + std::max( statusWidth - 800, 0 ) * 0.6;

    return wxControl::Ellipsize( msg, dc, wxELLIPSIZE_END, textWidth );
}


wxString KIUI::EllipsizeMenuText( const wxString& aString )
{
    wxString msg = UnescapeString( aString );

    msg.Replace( wxT( "\n" ), wxT( " " ) );
    msg.Replace( wxT( "\r" ), wxT( " " ) );
    msg.Replace( wxT( "\t" ), wxT( " " ) );

    if( msg.Length() > 36 )
        msg = msg.Left( 34 ) + wxT( "..." );

    return msg;
}


void KIUI::SelectReferenceNumber( wxTextEntry* aTextEntry )
{
    wxString ref = aTextEntry->GetValue();

    if( ref.find_first_of( '?' ) != ref.npos )
    {
        aTextEntry->SetSelection( ref.find_first_of( '?' ), ref.find_last_of( '?' ) + 1 );
    }
    else if( ref.find_first_of( '*' ) != ref.npos )
    {
        aTextEntry->SetSelection( ref.find_first_of( '*' ), ref.find_last_of( '*' ) + 1 );
    }
    else
    {
        wxString num = ref;

        while( !num.IsEmpty() && ( !isdigit( num.Last() ) || !isdigit( num.GetChar( 0 ) ) ) )
        {
            // Trim non-digit from end
            if( !isdigit( num.Last() ) )
                num.RemoveLast();

            // Trim non-digit from the start
            if( !num.IsEmpty() && !isdigit( num.GetChar( 0 ) ) )
                num = num.Right( num.Length() - 1 );
        }

        aTextEntry->SetSelection( ref.Find( num ), ref.Find( num ) + num.Length() );

        if( num.IsEmpty() )
            aTextEntry->SetSelection( -1, -1 );
    }
}


bool KIUI::IsInputControlFocused( wxWindow* aFocus )
{
    if( aFocus == nullptr )
        aFocus = wxWindow::FindFocus();

    if( !aFocus )
        return false;

    // These widgets are never considered focused
    if( aFocus->GetName() == s_FocusStealableInputName )
        return false;

    wxTextEntry*      textEntry = dynamic_cast<wxTextEntry*>( aFocus );
    wxStyledTextCtrl* styledText = dynamic_cast<wxStyledTextCtrl*>( aFocus );
    wxListBox*        listBox = dynamic_cast<wxListBox*>( aFocus );
    wxSearchCtrl*     searchCtrl = dynamic_cast<wxSearchCtrl*>( aFocus );
    wxCheckBox*       checkboxCtrl = dynamic_cast<wxCheckBox*>( aFocus );
    wxChoice*         choiceCtrl = dynamic_cast<wxChoice*>( aFocus );
    wxRadioButton*    radioBtn = dynamic_cast<wxRadioButton*>( aFocus );
    wxSpinCtrl*       spinCtrl = dynamic_cast<wxSpinCtrl*>( aFocus );
    wxSpinCtrlDouble* spinDblCtrl = dynamic_cast<wxSpinCtrlDouble*>( aFocus );
    wxSlider*         sliderCtl = dynamic_cast<wxSlider*>( aFocus );

    // Data view control is annoying, the focus is on a "wxDataViewCtrlMainWindow" class that
    // is not formally exported via the header.
    wxDataViewCtrl* dataViewCtrl = nullptr;

    wxWindow* parent = aFocus->GetParent();

    if( parent )
        dataViewCtrl = dynamic_cast<wxDataViewCtrl*>( parent );

    return ( textEntry || styledText || listBox || searchCtrl || checkboxCtrl || choiceCtrl
                || radioBtn || spinCtrl || spinDblCtrl || sliderCtl || dataViewCtrl );
}


bool KIUI::IsInputControlEditable( wxWindow* aFocus )
{
    wxTextEntry*      textEntry = dynamic_cast<wxTextEntry*>( aFocus );
    wxStyledTextCtrl* styledText = dynamic_cast<wxStyledTextCtrl*>( aFocus );
    wxSearchCtrl*     searchCtrl = dynamic_cast<wxSearchCtrl*>( aFocus );

    if( textEntry )
        return textEntry->IsEditable();
    else if( styledText )
        return styledText->IsEditable();
    else if( searchCtrl )
        return searchCtrl->IsEditable();

    // Must return true if we can't determine the state, intentionally true for non inputs as well.
    return true;
}


bool KIUI::IsModalDialogFocused()
{
    return !Pgm().m_ModalDialogs.empty();
}


void KIUI::Disable( wxWindow* aWindow )
{
    wxScrollBar*      scrollBar = dynamic_cast<wxScrollBar*>( aWindow );
    wxHyperlinkCtrl*  hyperlink = dynamic_cast<wxHyperlinkCtrl*>( aWindow );
    wxGrid*           grid = dynamic_cast<wxGrid*>( aWindow );
    wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aWindow );
    wxControl*        control = dynamic_cast<wxControl*>( aWindow );

    if( scrollBar || hyperlink )
    {
        // Leave navigation controls active
    }
    else if( grid )
    {
        for( int row = 0; row < grid->GetNumberRows(); ++row )
        {
            for( int col = 0; col < grid->GetNumberCols(); ++col )
                grid->SetReadOnly( row, col );
        }
    }
    else if( scintilla )
    {
        scintilla->SetReadOnly( true );
    }
    else if( control )
    {
        control->Disable();
    }
    else
    {
        for( wxWindow* child : aWindow->GetChildren() )
            Disable( child );
    }
}


void KIUI::AddBitmapToMenuItem( wxMenuItem* aMenu, const wxBitmapBundle& aImage )
{
    // Retrieve the global application show icon option:
    bool useImagesInMenus = Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Appearance.use_icons_in_menus;

    wxItemKind menu_type = aMenu->GetKind();

    if( useImagesInMenus && menu_type != wxITEM_CHECK && menu_type != wxITEM_RADIO )
        aMenu->SetBitmap( aImage );
}


wxMenuItem* KIUI::AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                               const wxBitmapBundle& aImage, wxItemKind aType )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, wxEmptyString, aType );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* KIUI::AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                               const wxString& aHelpText, const wxBitmapBundle& aImage,
                               wxItemKind aType )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, aHelpText, aType );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* KIUI::AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId, const wxString& aText,
                               const wxBitmapBundle& aImage )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText );
    item->SetSubMenu( aSubMenu );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* KIUI::AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId, const wxString& aText,
                               const wxString& aHelpText, const wxBitmapBundle& aImage )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, aHelpText );
    item->SetSubMenu( aSubMenu );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}
