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

/**
 * @file easypc_classes_library.h
 * @brief Symbols, components and gates, embedded in designs or the single object of a library item stream.
 *
 * Symbols and schematic and board components carry their own format, which every object whose parent chain reaches
 * them gates on.
 */

#ifndef EASYPC_CLASSES_LIBRARY_H
#define EASYPC_CLASSES_LIBRARY_H

#include <memory>
#include <vector>

#include <wx/string.h>

#include <io/easypc/easypc_classes_geometry.h>


namespace EASYPC
{

/// The item's own format, returned by the self-versioned classes' ItemFormat
struct SELF_VERSIONED
{
    int Format = NEWEST_ITEM_FORMAT;
};


/// Footprints and schematic symbols alike
struct SOURCE_SYMBOL : SOURCE_DESIGN_ITEM, SELF_VERSIONED
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override { return Format; }

    wxString Name;
    OBJECT*  Shapes = nullptr; ///< free copper list
    OBJECT*  Pads = nullptr;   ///< free pad list
    OBJECT*  Texts = nullptr;  ///< free text list
    OBJECT*  ValuePositions = nullptr;
    int32_t  OriginX = 0;
    int32_t  OriginY = 0;
    int32_t  Timestamp = 0;
    wxString Library;

    /// The symbol name origin of formats below 10003, which becomes the only value position
    std::unique_ptr<OBJECT>                NameOrigin;
    std::unique_ptr<SOURCE_DESIGN_LIST>    DerivedValuePositions;
    std::unique_ptr<SOURCE_VALUE_POSITION> DerivedNamePosition;
};


/// Pin name and pin number definitions
struct SOURCE_PIN_NAME : SOURCE_TEXT_POSITION
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    bool Visible = true;
};


/// Placed pin names and numbers
struct SOURCE_PIN_NAME_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Definition = nullptr; ///< the pin name or number instantiated
    bool    Visible = true;
};


/// A schematic symbol and its pin names
struct SOURCE_GATE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString              Symbol;
    std::vector<wxString> PinNames; ///< terminal t is the symbol pad numbered t + 1
    wxString              Library;
};


/// An array: owner tag, then a counted list of objects
struct SOURCE_DESIGN_ARRAY : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT*              Parent = nullptr;
    std::vector<OBJECT*> Items;
};


/// The items of a design list or array, empty for anything else
const std::vector<OBJECT*>& ListItems( const OBJECT* aList );


/// The items of ListItems that are a T, in order
template <typename T>
std::vector<T*> TypedItems( const OBJECT* aList )
{
    std::vector<T*> out;

    for( OBJECT* obj : ListItems( aList ) )
    {
        if( T* typed = dynamic_cast<T*>( obj ) )
            out.push_back( typed );
    }

    return out;
}


/// One gate's terminals to footprint pads
struct SOURCE_GATE_MAP : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT*              Pins = nullptr; ///< gate pin array, format above 8999
    std::vector<int32_t> PcbSymbolPins;  ///< older formats: pad number per terminal, -1 unmapped
};


struct SOURCE_GATE_PIN : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    bool IsNoConnect() const { return PinType & 0x1; }

    wxString PcbSymbolPin = wxS( "-1" ); ///< pad number, "-1", or a pad list
    wxString PinNumber;
    wxString NetName; ///< default net, or a net class in parentheses
    uint32_t PinType = 0;
};


struct SOURCE_SCM_COMPONENT : SOURCE_DESIGN_ITEM, SELF_VERSIONED
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override { return Format; }

    wxString Name;
    wxString Package;
    OBJECT*  Gates = nullptr;
    wxString DefaultReference;
    wxString Description;
    wxString Library;
    uint32_t SuppressionFlags = 0;
};


/// The package half of a component
struct SOURCE_PCB_COMPONENT : SOURCE_DESIGN_ITEM, SELF_VERSIONED
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override { return Format; }

    wxString Package;
    wxString Symbol;             ///< footprint name
    OBJECT*  GateMaps = nullptr; ///< one per schematic gate in order
    wxString Library;
};


struct SOURCE_COMPONENT : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    SOURCE_PCB_COMPONENT* Board = nullptr;
    SOURCE_SCM_COMPONENT* Schematic = nullptr;
};


/**
 * Expand a gate pin's component pin list by Easy-PC's pin list syntax: items separated by `,` or `+`, numeric
 * ranges `5-9`, and an optional `display=` prefix that only changes what is shown.
 */
std::vector<wxString> ExpandPinNumberList( const wxString& aList, wxString* aDisplay = nullptr );


/// Whether a gate pin's pad reference names several pads ("-1", the unmapped marker, does not)
bool IsPadList( const wxString& aPcbSymbolPin );

} // namespace EASYPC

#endif // EASYPC_CLASSES_LIBRARY_H
