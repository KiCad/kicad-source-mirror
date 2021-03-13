/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __dialog_lib_edit_draw_item__
#define __dialog_lib_edit_draw_item__


class LIB_ITEM;
class SYMBOL_EDIT_FRAME;


#include <dialog_lib_edit_draw_item_base.h>
#include <widgets/unit_binder.h>

/**
 * Dialog to edit library component graphic items.
 */
class DIALOG_LIB_EDIT_DRAW_ITEM : public DIALOG_LIB_EDIT_DRAW_ITEM_BASE
{
public:
    /** Constructor */
    DIALOG_LIB_EDIT_DRAW_ITEM( SYMBOL_EDIT_FRAME* parent, LIB_ITEM* aItem );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool GetApplyToAllConversions();
    bool GetApplyToAllUnits();

private:
    SYMBOL_EDIT_FRAME* m_frame;
    LIB_ITEM*          m_item;
    UNIT_BINDER        m_lineWidth;
};

#endif // __dialog_lib_edit_draw_item__
