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

#pragma once

#include <dialogs/dialog_router_save_test_case_base.h>

#include "router/pns_logger.h"

class PCB_BASE_EDIT_FRAME;

class DIALOG_ROUTER_SAVE_TEST_CASE : public DIALOG_ROUTER_SAVE_TEST_CASE_BASE
{
public:
    DIALOG_ROUTER_SAVE_TEST_CASE( PCB_BASE_EDIT_FRAME* aParent, const wxString& aTestCaseDir );
    ~DIALOG_ROUTER_SAVE_TEST_CASE();

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    PNS::LOGGER::TEST_CASE_TYPE getTestCaseType() const { return m_testCaseType; }
    const wxString& getTestCaseName() const { return m_testCaseName; }

private:
    wxString m_testCaseDir;
    wxString m_testCaseName;
    PNS::LOGGER::TEST_CASE_TYPE m_testCaseType;
};

