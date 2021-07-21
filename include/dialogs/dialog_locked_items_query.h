/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_LOCKED_ITEMS_QUERY_H
#define DIALOG_LOCKED_ITEMS_QUERY_H

#include <dialog_locked_items_query_base.h>


class DIALOG_LOCKED_ITEMS_QUERY : public DIALOG_LOCKED_ITEMS_QUERY_BASE
{
public:
    /// This has no dependencies on calling wxFrame derivative, such as PCB_BASE_FRAME.
    DIALOG_LOCKED_ITEMS_QUERY( wxWindow* aParent, int aLockedItemCount );

    int ShowModal() override;

private:
    void onOverrideLocks( wxCommandEvent& event ) override;
};

#endif // DIALOG_LOCKED_ITEMS_QUERY_H
