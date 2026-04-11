/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <magic_enum.hpp>
#include <import_export.h>
#include <qa_utils/api_test_utils.h>

#include <api/api_enums.h>
#include <api/schematic/schematic_types.pb.h>
#include <lib_symbol.h>

#include <sch_label.h>
#include <sch_sheet_pin.h>


BOOST_AUTO_TEST_SUITE( ApiSchEnums )

using namespace kiapi::schematic;

BOOST_AUTO_TEST_CASE( SchematicLabelShape )
{
    testEnums<LABEL_FLAG_SHAPE, types::SchematicLabelShape>();
}

BOOST_AUTO_TEST_CASE( SchematicLabelSpinStyle )
{
    testEnums<SPIN_STYLE::SPIN, types::SchematicLabelSpinStyle>();
}

BOOST_AUTO_TEST_CASE( SheetSide )
{
    testEnums<SHEET_SIDE, types::SheetSide>( true );
}

BOOST_AUTO_TEST_CASE( SchematicSymbolType )
{
    testEnums<LIBRENTRYOPTIONS, types::SchematicSymbolType>();
}

BOOST_AUTO_TEST_CASE( SchematicSymbolOrientation )
{
    testEnums<SYMBOL_ORIENTATION_PROP, types::SchematicSymbolOrientation>();
}

BOOST_AUTO_TEST_CASE( SchematicPinOrientation )
{
    testEnums<PIN_ORIENTATION, types::SchematicPinOrientation>( true );
}

BOOST_AUTO_TEST_CASE( SchematicPinShape )
{
    testEnums<GRAPHIC_PINSHAPE, types::SchematicPinShape>( true );
}

BOOST_AUTO_TEST_SUITE_END()
