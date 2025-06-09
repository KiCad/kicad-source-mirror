/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sheet_synchronization_item.h"
#include "bitmaps/bitmap_types.h"
#include "sync_sheet_pin/sheet_synchronization_item.h"
#include "sync_sheet_pin/sync_sheet_pin_preference.h"
#include <sch_label.h>
#include <sch_sheet_pin.h>
#include <bitmaps/bitmaps_list.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <wx/bitmap.h>
#include <wx/image.h>


SCH_HIERLABEL_SYNCHRONIZATION_ITEM::SCH_HIERLABEL_SYNCHRONIZATION_ITEM( SCH_HIERLABEL* aLabel,
                                                                        SCH_SHEET*     aSheet ) :
        m_label( aLabel )
{
}


wxString SCH_HIERLABEL_SYNCHRONIZATION_ITEM::GetName() const
{
    return m_label->GetShownText( true );
}


int SCH_HIERLABEL_SYNCHRONIZATION_ITEM::GetShape() const
{
    return m_label->GetShape();
}


wxBitmap& SCH_HIERLABEL_SYNCHRONIZATION_ITEM::GetBitmap() const
{
    static wxBitmap bitMap =
            KiBitmap( BITMAPS::add_hierarchical_label, SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT );
    return bitMap;
}


SCH_ITEM* SCH_HIERLABEL_SYNCHRONIZATION_ITEM::GetItem() const
{
    return m_label;
}


SHEET_SYNCHRONIZATION_ITEM_KIND SCH_HIERLABEL_SYNCHRONIZATION_ITEM::GetKind() const
{
    return SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL;
}


SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::SCH_SHEET_PIN_SYNCHRONIZATION_ITEM( SCH_SHEET_PIN* aPin,
                                                                        SCH_SHEET*     aSheet ) :
        m_pin( aPin )
{
}


wxString SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::GetName() const
{
    return m_pin->GetShownText( true );
}


int SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::GetShape() const
{
    return m_pin->GetShape();
}


wxBitmap& SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::GetBitmap() const
{
    static wxBitmap bitMap =
            KiBitmap( BITMAPS::add_hierar_pin, SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT );
    return bitMap;
}


SCH_ITEM* SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::GetItem() const
{
    return m_pin;
}


SHEET_SYNCHRONIZATION_ITEM_KIND SCH_SHEET_PIN_SYNCHRONIZATION_ITEM::GetKind() const
{
    return SHEET_SYNCHRONIZATION_ITEM_KIND::SHEET_PIN;
}


ASSOCIATED_SCH_LABEL_PIN::ASSOCIATED_SCH_LABEL_PIN( SCH_HIERLABEL* aLabel, SCH_SHEET_PIN* aPin ) :
        m_label( aLabel ), m_pin( aPin )
{
}


ASSOCIATED_SCH_LABEL_PIN::ASSOCIATED_SCH_LABEL_PIN( SCH_HIERLABEL_SYNCHRONIZATION_ITEM* aLabel,
                                                    SCH_SHEET_PIN_SYNCHRONIZATION_ITEM* aPin ) :
        ASSOCIATED_SCH_LABEL_PIN( aLabel->GetLabel(), aPin->GetPin() )
{
}


wxString ASSOCIATED_SCH_LABEL_PIN::GetName() const
{
    return m_label->GetShownText( true );
}


int ASSOCIATED_SCH_LABEL_PIN::GetShape() const
{
    return m_label->GetShape();
}


wxBitmap& ASSOCIATED_SCH_LABEL_PIN::GetBitmap() const
{
    static auto label_and_pin_icon = ( []{
        wxBitmap left =  KiBitmap( BITMAPS::add_hierar_pin,
                                   SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT );
        wxBitmap right =
                KiBitmap( BITMAPS::add_hierarchical_label,
                          SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT );
        wxImage img( wxSize{ SYNC_SHEET_PIN_PREFERENCE::NORMAL_WIDTH * 2,
                             SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT } );
        img.Paste( left.ConvertToImage(), 0, 0 );
        img.Paste( right.ConvertToImage(), SYNC_SHEET_PIN_PREFERENCE::NORMAL_WIDTH, 0 );
        return wxBitmap( img );
    } )();

    return label_and_pin_icon;
}


SCH_ITEM* ASSOCIATED_SCH_LABEL_PIN::GetItem() const
{
    return nullptr;
}


SHEET_SYNCHRONIZATION_ITEM_KIND ASSOCIATED_SCH_LABEL_PIN::GetKind() const
{
    return SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL_AND_SHEET_PIN;
}
