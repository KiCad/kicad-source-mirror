/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EESCHEMA_CONFIG_H
#define EESCHEMA_CONFIG_H

#include <config_params.h>

extern const wxChar RescueNeverShowEntry[];
extern const wxChar AutoplaceFieldsEntry[];
extern const wxChar AutoplaceJustifyEntry[];
extern const wxChar AutoplaceAlignEntry[];
extern const wxChar LibIconScaleEntry[];
extern const wxChar SchIconScaleEntry[];

class TEMPLATES;


class PARAM_CFG_FIELDNAMES : public PARAM_CFG
{
protected:
    TEMPLATES* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_FIELDNAMES( TEMPLATES* ptparam, const wxChar* group = nullptr );

    void ReadParam( wxConfigBase* aConfig ) const override;
    void SaveParam( wxConfigBase* aConfig ) const override;
};



#endif      // EESCHEMA_CONFIG_H
