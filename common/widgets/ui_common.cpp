/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/listbox.h>
#include <wx/dataview.h>
#include <wx/stc/stc.h>
#include <widgets/ui_common.h>

#include <algorithm>
#include <dialog_shim.h>
#include <pgm_base.h>
#include <wx/settings.h>

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
        font.SetFaceName( "Menlo" );
#endif

    return font;
}


wxFont KIUI::GetInfoFont()
{
    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    font.SetSymbolicSize( wxFONTSIZE_SMALL );

#ifdef __WXMAC__
    // https://trac.wxwidgets.org/ticket/19210
    if( font.GetFaceName().IsEmpty() )
        font.SetFaceName( "Lucida Grande" );
#endif

    return font;
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


bool KIUI::IsInputControlFocused()
{
    wxWindow*         focus        = wxWindow::FindFocus();
    wxTextEntry*      textEntry    = dynamic_cast<wxTextEntry*>( focus );
    wxStyledTextCtrl* styledText   = dynamic_cast<wxStyledTextCtrl*>( focus );
    wxListBox*        listBox      = dynamic_cast<wxListBox*>( focus );
    wxDataViewCtrl*   dataViewCtrl = dynamic_cast<wxDataViewCtrl*>( focus );

    return ( textEntry || styledText || listBox || dataViewCtrl );
}


bool KIUI::IsModalDialogFocused()
{
    return Pgm().m_ModalDialogCount > 0;
}