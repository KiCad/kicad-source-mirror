/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef DIALOG_EESCHEMA_PAGE_SETTINGS_H
#define DIALOG_EESCHEMA_PAGE_SETTINGS_H

#include <dialogs/dialog_page_settings.h>


class DIALOG_EESCHEMA_PAGE_SETTINGS : public DIALOG_PAGES_SETTINGS
{
public:
    DIALOG_EESCHEMA_PAGE_SETTINGS( EDA_DRAW_FRAME* aParent, wxSize aMaxUserSizeMils );
    virtual ~DIALOG_EESCHEMA_PAGE_SETTINGS();

private:
    void onTransferDataToWindow() override;
    bool onSavePageSettings() override;
};

#endif // DIALOG_PCBNEW_PAGE_SETTINGS
