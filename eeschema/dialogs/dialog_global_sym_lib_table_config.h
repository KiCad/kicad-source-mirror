/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_H_
#define _DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_H_

#include <dialogs/dialog_global_lib_table_config.h>


class DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG : public DIALOG_GLOBAL_LIB_TABLE_CONFIG
{
public:
    DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG( wxWindow* aParent );
    virtual ~DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG();

    bool TransferDataFromWindow() override;

    virtual wxFileName GetGlobalTableFileName() override;
};

#endif  // _DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_H_
