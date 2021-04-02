/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/ui.h>

#include <wx/choice.h>
#include <wx/nonownedwnd.h>
#include <wx/settings.h>
#include <wx/window.h>

#include <gtk/gtk.h>


bool KIPLATFORM::UI::IsDarkTheme()
{
    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );

    // Weighted W3C formula
    double brightness = ( bg.Red() / 255.0 ) * 0.299 +
                        ( bg.Green() / 255.0 ) * 0.587 +
                        ( bg.Blue() / 255.0 ) * 0.117;

    return brightness < 0.5;
}


void KIPLATFORM::UI::ForceFocus( wxWindow* aWindow )
{
    aWindow->SetFocus();
}


bool KIPLATFORM::UI::IsWindowActive( wxWindow* aWindow )
{
    GtkWindow* window = GTK_WINDOW( aWindow->GetHandle() );

    if( window )
        return gtk_window_is_active( window );

    // We shouldn't really ever reach this point
    return false;
}


void KIPLATFORM::UI::ReparentQuasiModal( wxNonOwnedWindow* aWindow )
{
    // Not needed on this platform
}


void KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( wxWindow *aWindow )
{
    // Not needed on this platform
}


bool KIPLATFORM::UI::IsStockCursorOk( wxStockCursor aCursor )
{
    switch( aCursor )
    {
    case wxCURSOR_BULLSEYE:
    case wxCURSOR_HAND:
    case wxCURSOR_ARROW:
        return true;
    default:
        return false;
    }
}


void KIPLATFORM::UI::EllipsizeChoiceBox( wxChoice* aChoice )
{
    // This function is based on the code inside the function post_process_ui in gtkfilechooserwidget.c
    GList* cells = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( aChoice->m_widget ) );

    if( !cells )
        return;

    GtkCellRenderer* cell = (GtkCellRenderer*) cells->data;

    if( !cell )
        return;

    g_object_set( G_OBJECT( cell ), "ellipsize", PANGO_ELLIPSIZE_END, NULL );

    // Only the list of cells must be freed, the renderer isn't ours to free
    g_list_free( cells );
}


double KIPLATFORM::UI::GetSystemScaleFactor( const wxWindow* aWindow )
{
    double val = 1.0;

    GtkWidget* widget = static_cast<GtkWidget*>( aWindow->GetHandle() );

    if( widget && gtk_check_version( 3, 10, 0 ) == NULL )
        val = gtk_widget_get_scale_factor( widget );

    return val;
}
