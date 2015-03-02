/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sheetlab.cpp
 * @brief Create and edit the SCH_SHEET_PIN items.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <schframe.h>
#include <base_units.h>

#include <general.h>
#include <sch_sheet.h>
#include <dialog_helpers.h>

#include <dialogs/dialog_sch_edit_sheet_pin.h>


int SCH_EDIT_FRAME::m_lastSheetPinType = NET_INPUT;
wxSize SCH_EDIT_FRAME::m_lastSheetPinTextSize( -1, -1 );
wxPoint SCH_EDIT_FRAME::m_lastSheetPinPosition;

const wxSize &SCH_EDIT_FRAME::GetLastSheetPinTextSize()
{
    // Delayed initialization (need the preferences to be loaded)
    if( m_lastSheetPinTextSize.x == -1 )
    {
        m_lastSheetPinTextSize.x = GetDefaultTextSize();
        m_lastSheetPinTextSize.y = GetDefaultTextSize();
    }
    return m_lastSheetPinTextSize;
}

int SCH_EDIT_FRAME::EditSheetPin( SCH_SHEET_PIN* aSheetPin, wxDC* aDC )
{
    if( aSheetPin == NULL )
        return wxID_CANCEL;

    DIALOG_SCH_EDIT_SHEET_PIN dlg( this );

    dlg.SetLabelName( aSheetPin->GetText() );
    dlg.SetTextHeight( StringFromValue( g_UserUnit, aSheetPin->GetSize().y ) );
    dlg.SetTextHeightUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetTextWidth( StringFromValue( g_UserUnit, aSheetPin->GetSize().x ) );
    dlg.SetTextWidthUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetConnectionType( aSheetPin->GetShape() );

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier versions for
     * the flex grid sizer in wxGTK that prevents the last column from being sized
     * correctly.  It doesn't cause any problems on win32 so it doesn't need to wrapped
     * in ugly #ifdef __WXGTK__ #endif.
     */
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxID_CANCEL;

    if( aDC )
        aSheetPin->Draw( m_canvas, aDC, wxPoint( 0, 0 ), g_XorMode );

    if( !aSheetPin->IsNew() )
    {
        SaveCopyInUndoList( (SCH_ITEM*) aSheetPin->GetParent(), UR_CHANGED );
        GetScreen()->SetCurItem( NULL );
    }

    aSheetPin->SetText( dlg.GetLabelName() );
    aSheetPin->SetSize( wxSize( ValueFromString( g_UserUnit, dlg.GetTextHeight() ),
                                ValueFromString( g_UserUnit, dlg.GetTextWidth() ) ) );
    aSheetPin->SetShape( dlg.GetConnectionType() );

    if( aDC )
        aSheetPin->Draw( m_canvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    return wxID_OK;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::CreateSheetPin( SCH_SHEET* aSheet, wxDC* aDC )
{
    wxString       line;
    SCH_SHEET_PIN* sheetPin;

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), line );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetSize( GetLastSheetPinTextSize() );
    sheetPin->SetShape( m_lastSheetPinType );

    int response = EditSheetPin( sheetPin, NULL );

    if( sheetPin->GetText().IsEmpty() || (response == wxID_CANCEL) )
    {
        delete sheetPin;
        return NULL;
    }

    m_lastSheetPinType = sheetPin->GetShape();
    m_lastSheetPinTextSize = sheetPin->GetSize();

    sheetPin->SetPosition( GetCrossHairPosition() );
    sheetPin->Draw( m_canvas, aDC, wxPoint( 0, 0 ), g_XorMode );
    PrepareMoveItem( (SCH_ITEM*) sheetPin, aDC );

    OnModify();
    return sheetPin;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::ImportSheetPin( SCH_SHEET* aSheet, wxDC* aDC )
{
    EDA_ITEM*      item;
    SCH_SHEET_PIN* sheetPin;
    SCH_HIERLABEL* label = NULL;

    if( !aSheet->GetScreen() )
        return NULL;

    item = aSheet->GetScreen()->GetDrawItems();

    for( ; item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_HIERARCHICAL_LABEL_T )
            continue;

        label = (SCH_HIERLABEL*) item;

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !aSheet->HasPin( label->GetText() ) )
            break;

        label = NULL;
    }

    if( label == NULL )
    {
        DisplayInfoMessage( this, _( "No new hierarchical labels found." ) );
        return NULL;
    }

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), label->GetText() );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetSize( GetLastSheetPinTextSize() );
    m_lastSheetPinType = label->GetShape();
    sheetPin->SetShape( label->GetShape() );
    sheetPin->SetPosition( GetCrossHairPosition() );

    sheetPin->Draw( m_canvas, aDC, wxPoint( 0, 0 ), g_XorMode );
    PrepareMoveItem( (SCH_ITEM*) sheetPin, aDC );

    return sheetPin;
}
