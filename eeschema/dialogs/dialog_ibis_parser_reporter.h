/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _DIALOG_IBIS_PARSER_REPORTER_H_
#define _DIALOG_IBIS_PARSER_REPORTER_H_

#include <dialog_ibis_parser_reporter_base.h>


class SCH_EDIT_FRAME;

class DIALOG_IBIS_PARSER_REPORTER : public DIALOG_IBIS_PARSER_REPORTER_BASE
{
public:
    DIALOG_IBIS_PARSER_REPORTER( wxWindow* aParent );
    ~DIALOG_IBIS_PARSER_REPORTER();

private:
    void updateData();

    bool TransferDataToWindow() override;
    void OnCloseClick( wxCommandEvent& event ) override { Close(); };

    wxWindow* m_frame;
};

#endif
