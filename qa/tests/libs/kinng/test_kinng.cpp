/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <kinng.h>

#include <import_export.h>
#include <api/common/envelope.pb.h>

BOOST_AUTO_TEST_SUITE( KiNNG )

BOOST_AUTO_TEST_CASE( CreateIPCResponder )
{
    KINNG_REQUEST_SERVER server( wxFileName::CreateTempFileName( "test-kinng" ).ToStdString() );
}

BOOST_AUTO_TEST_SUITE_END()
