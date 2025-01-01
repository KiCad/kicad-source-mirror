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

#ifndef KICAD_PROPERTY_VALIDATOR_H
#define KICAD_PROPERTY_VALIDATOR_H

#include <functional>
#include <optional>

#include <wx/any.h>
#include <wx/string.h>

class EDA_ITEM;
class UNITS_PROVIDER;

/// Represents an error returned by a validator and contains enough data to format an error message
class VALIDATION_ERROR
{
public:
    virtual ~VALIDATION_ERROR() = default;

    virtual wxString Format( UNITS_PROVIDER* aUnits ) const = 0;
};

// TODO: This is a bit of code smell; maybe create a wrapper class to hide the ptr
/// Null optional means validation succeeded
using VALIDATOR_RESULT = std::optional<std::unique_ptr<VALIDATION_ERROR>>;

/**
* A property validator function takes in the data type of the owning property, and returns a
* VALIDATOR_RESULT that will be empty (null) if validation succeeded, and contain an error message
* otherwise.
*/
using PROPERTY_VALIDATOR_FN = std::function<VALIDATOR_RESULT( const wxAny&&, EDA_ITEM* aItem )>;

#endif //KICAD_PROPERTY_VALIDATOR_H
