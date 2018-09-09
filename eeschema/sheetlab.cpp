/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>

#include <sch_sheet.h>
#include <dialog_helpers.h>

#include <dialogs/dialog_sch_edit_sheet_pin.h>


PINSHEETLABEL_SHAPE SCH_EDIT_FRAME::m_lastSheetPinType = NET_INPUT;
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


int SCH_EDIT_FRAME::EditSheetPin( SCH_SHEET_PIN* aSheetPin, bool aRedraw )
{
    if( aSheetPin == NULL )
        return wxID_CANCEL;

    DIALOG_SCH_EDIT_SHEET_PIN dlg( this, aSheetPin );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxID_CANCEL;

    if( aRedraw )
        RefreshItem( aSheetPin );

    return wxID_OK;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::CreateSheetPin( SCH_SHEET* aSheet )
{
    wxString       line;
    SCH_SHEET_PIN* sheetPin;

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), line );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( GetLastSheetPinTextSize() );
    sheetPin->SetShape( m_lastSheetPinType );

    int response = EditSheetPin( sheetPin, false );

    if( sheetPin->GetText().IsEmpty() || (response == wxID_CANCEL) )
    {
        delete sheetPin;
        return NULL;
    }

    m_lastSheetPinType = sheetPin->GetShape();
    m_lastSheetPinTextSize = sheetPin->GetTextSize();

    sheetPin->SetPosition( GetCrossHairPosition() );
    PrepareMoveItem( sheetPin );

    OnModify();
    return sheetPin;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::ImportSheetPin( SCH_SHEET* aSheet )
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
    sheetPin->SetTextSize( GetLastSheetPinTextSize() );
    m_lastSheetPinType = label->GetShape();
    sheetPin->SetShape( label->GetShape() );
    sheetPin->SetPosition( GetCrossHairPosition() );

    PrepareMoveItem( sheetPin );

    return sheetPin;
}
