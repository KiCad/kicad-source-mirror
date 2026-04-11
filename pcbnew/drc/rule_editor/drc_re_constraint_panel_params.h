/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef DRC_RE_CONSTRAINT_PANEL_PARAMS_H_
#define DRC_RE_CONSTRAINT_PANEL_PARAMS_H_

#include <wx/string.h>

#include "drc_rule_editor_enums.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_re_bool_input_constraint_data.h"


class DRC_RE_CONSTRAINT_PANEL_PARAMS
{
public:
    DRC_RE_CONSTRAINT_PANEL_PARAMS() {}

    DRC_RE_CONSTRAINT_PANEL_PARAMS( const wxString& aConstraintTitle,
                                    const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType ) :
            m_constraintTitle( aConstraintTitle ), m_constraintType( aConstraintType )
    {
    }

    wxString m_constraintTitle;
    DRC_RULE_EDITOR_CONSTRAINT_NAME m_constraintType;
};


class DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS : public DRC_RE_CONSTRAINT_PANEL_PARAMS
{
public:

    DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS() : DRC_RE_CONSTRAINT_PANEL_PARAMS()
    {
    }

    DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS( const wxString& aConstraintTitle,
                                            const std::shared_ptr<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>& aConstraintData,
                                            const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType ) :
            DRC_RE_CONSTRAINT_PANEL_PARAMS( aConstraintTitle, aConstraintType ),
            m_constraintData( aConstraintData )
    {
    }

    DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS( const wxString& aConstraintTitle,
                                            const std::shared_ptr<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>& aConstraintData,
                                            const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType,
                                            const wxString& aCustomLabelText ) :
            DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS( aConstraintTitle, aConstraintData, aConstraintType )
    {
        m_customLabelText = aCustomLabelText;
    }


    void SetInputIsCount( bool aInputIsCount ) { m_isCountInput = aInputIsCount; }

    wxString m_customLabelText;
    bool m_isCountInput = false;
    std::shared_ptr<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA> m_constraintData;
};


class DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS : public DRC_RE_CONSTRAINT_PANEL_PARAMS
{
public:
    DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS() : DRC_RE_CONSTRAINT_PANEL_PARAMS() {}

    DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS( const wxString&  aConstraintTitle,
            const std::shared_ptr<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA>& aConstraintData,
            const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType ) :
            DRC_RE_CONSTRAINT_PANEL_PARAMS( aConstraintTitle, aConstraintType ),
            m_constraintData( aConstraintData )
    {
    }

    DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS( const wxString& aConstraintTitle,
            const std::shared_ptr<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA>& aConstraintData,
            const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType,
            const wxString& aCustomLabelText ) :
            DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS( aConstraintTitle, aConstraintData,
                                                       aConstraintType )
    {
        m_customLabelText = aCustomLabelText;
    }

    wxString  m_customLabelText;
    std::shared_ptr<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA> m_constraintData;
};

#endif // DRC_RE_CONSTRAINT_PANEL_PARAMS_H_