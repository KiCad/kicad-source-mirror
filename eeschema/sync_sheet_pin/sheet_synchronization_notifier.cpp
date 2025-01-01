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

#include "sheet_synchronization_notifier.h"
#include "panel_sync_sheet_pins.h"
#include "sheet_synchronization_model.h"

SHEET_SYNCHRONIZATION_NOTIFIER::SHEET_SYNCHRONIZATION_NOTIFIER(
        SHEET_SYNCHRONIZATION_MODEL* aOwner ) :
        m_owner( aOwner )
{
}


SHEET_FILE_CHANGE_NOTIFIER::SHEET_FILE_CHANGE_NOTIFIER( SHEET_SYNCHRONIZATION_MODEL* aOwner,
                                                        PANEL_SYNC_SHEET_PINS*       aPanel ) :
        SHEET_SYNCHRONIZATION_NOTIFIER( aOwner ),
        m_panel( aPanel )
{
}


bool SHEET_FILE_CHANGE_NOTIFIER::ShouldIgnore() const
{
    return false;
}


void SHEET_FILE_CHANGE_NOTIFIER::Sync()
{
    m_panel->UpdateForms();
}
