/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _PANEL_GLOBAL_LIB_TABLE_CONFIG_H_
#define _PANEL_GLOBAL_LIB_TABLE_CONFIG_H_

#include "panel_global_lib_table_config_base.h"
#include <memory>

#include <kiway.h>
#include <wx/filename.h>

class PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL
{
public:
    enum class TABLE_MODE
    {
        EMPTY,
        DEFAULT,
        CUSTOM
    };

    TABLE_MODE m_tableMode;
    wxString   m_customTablePath;
    wxString   m_defaultTablePath;
};


class PANEL_GLOBAL_LIB_TABLE_CONFIG : public PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE
{
public:
    PANEL_GLOBAL_LIB_TABLE_CONFIG( wxWindow* aParent, const wxString& aTableName,
                                   std::shared_ptr<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL> aModel,
                                   const KIWAY::FACE_T                  aFaceType );
    virtual ~PANEL_GLOBAL_LIB_TABLE_CONFIG();

    virtual wxFileName GetGlobalTableFileName() = 0;

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;

protected:
    virtual void onUpdateFilePicker( wxUpdateUIEvent& aEvent ) override;
    virtual void onUpdateDefaultSelection( wxUpdateUIEvent& aEvent ) override;

    wxString m_tableName;
    bool m_defaultFileFound;
    KIWAY::FACE_T m_faceType;
    std::shared_ptr<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL> m_model;
};

#endif  // _DIALOG_GLOBAL_LIB_TABLE_CONFIG_H_
