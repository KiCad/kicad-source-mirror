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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <map>
#include <wx/string.h>
#include <wx/translation.h>
#include "pcb_track.h"


class VIA_PROTECTION_UI_MIXIN
{
protected:
    enum class IPC4761_SURFACE
    {
        FROM_BOARD = 0,
        NONE = 1,
        FRONT = 2,
        BACK = 3,
        BOTH = 4,
        CUSTOM = 5
    };

    enum class IPC4761_DRILL
    {
        FROM_BOARD = 0,
        NOT_SET = 1,
        SET = 2
    };

    enum class IPC4761_PRESET
    {
        FROM_BOARD = 0,
        NONE = 1,
        IA = 2,
        IB = 3,
        IA_INVERTED = 4,
        IIA = 5,
        IIB = 6,
        IIA_INVERTED = 7,
        IIIA = 8,
        IIIB = 9,
        IIIA_INVERTED = 10,
        IVA = 11,
        IVB = 12,
        IVA_INVERTED = 13,
        V = 14,
        VIA = 15,
        VIB = 16,
        VIA_INVERTED = 17,
        VII = 18,
        CUSTOM = 19,
        END = 20
    };

    struct IPC4761_CONFIGURATION
    {
        IPC4761_SURFACE tent =  IPC4761_SURFACE::NONE;
        IPC4761_SURFACE cover = IPC4761_SURFACE::NONE;
        IPC4761_SURFACE plug =  IPC4761_SURFACE::NONE;
        IPC4761_DRILL   fill =  IPC4761_DRILL::NOT_SET;
        IPC4761_DRILL   cap =   IPC4761_DRILL::NOT_SET;

        bool operator==( const IPC4761_CONFIGURATION& other ) const;
    };

    const std::map<IPC4761_PRESET, IPC4761_CONFIGURATION> m_IPC4761Presets =
    {
        { IPC4761_PRESET::FROM_BOARD, { .tent =  IPC4761_SURFACE::FROM_BOARD,
                                        .cover = IPC4761_SURFACE::FROM_BOARD,
                                        .plug =  IPC4761_SURFACE::FROM_BOARD,
                                        .fill =  IPC4761_DRILL::FROM_BOARD,
                                        .cap =   IPC4761_DRILL::FROM_BOARD } },
        { IPC4761_PRESET::NONE, {} },
        { IPC4761_PRESET::IA, { .tent = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IB, { .tent = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IA_INVERTED, { .tent = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IIA, { .tent = IPC4761_SURFACE::FRONT, .cover = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IIB, { .tent = IPC4761_SURFACE::BOTH, .cover = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IIA_INVERTED, { .tent = IPC4761_SURFACE::BACK, .cover = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IIIA, { .plug = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IIIB, { .plug = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IIIA_INVERTED, { .plug = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::IVA, { .tent = IPC4761_SURFACE::FRONT, .plug = IPC4761_SURFACE::FRONT } },
        { IPC4761_PRESET::IVB, { .tent = IPC4761_SURFACE::BOTH, .plug = IPC4761_SURFACE::BOTH } },
        { IPC4761_PRESET::IVA_INVERTED, { .tent = IPC4761_SURFACE::BACK, .plug = IPC4761_SURFACE::BACK } },
        { IPC4761_PRESET::V, { .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIA, { .tent = IPC4761_SURFACE::FRONT, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIB, { .tent = IPC4761_SURFACE::BOTH, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VIA_INVERTED, { .tent = IPC4761_SURFACE::BACK, .fill = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::VII, { .fill = IPC4761_DRILL::SET, .cap = IPC4761_DRILL::SET } },
        { IPC4761_PRESET::CUSTOM, {} },
        { IPC4761_PRESET::END, {} }
    };

    const std::map<IPC4761_PRESET, wxString> m_IPC4761Names =
    {
        { IPC4761_PRESET::FROM_BOARD,    _( "From rules" ) },
        { IPC4761_PRESET::NONE,          _( "None" ) },
        { IPC4761_PRESET::IA,            _( "Type I-a (tented top)" ) },
        { IPC4761_PRESET::IB,            _( "Type I-b (tented both sides)" ) },
        { IPC4761_PRESET::IA_INVERTED,   _( "Type I-a (tented bottom)" ) },
        { IPC4761_PRESET::IIA,           _( "Type II-a (covered and tented top)" ) },
        { IPC4761_PRESET::IIB,           _( "Type II-b (covered and tented both sides)" ) },
        { IPC4761_PRESET::IIA_INVERTED,  _( "Type II-a (covered and tented bottom)" ) },
        { IPC4761_PRESET::IIIA,          _( "Type III-a (plugged top)" ) },
        { IPC4761_PRESET::IIIB,          _( "Type III-b (plugged both sides)" ) },
        { IPC4761_PRESET::IIIA_INVERTED, _( "Type III-a (plugged bottom)" ) },
        { IPC4761_PRESET::IVA,           _( "Type IV-a (plugged and tented top)" ) },
        { IPC4761_PRESET::IVB,           _( "Type IV-b (plugged and tented both sides)" ) },
        { IPC4761_PRESET::IVA_INVERTED,  _( "Type IV-a (plugged and tented bottom)" ) },
        { IPC4761_PRESET::V,             _( "Type V (filled )" ) },
        { IPC4761_PRESET::VIA,           _( "Type VI-a (filled and tented top)" ) },
        { IPC4761_PRESET::VIB,           _( "Type VI-b (filled and tented both sides)" ) },
        { IPC4761_PRESET::VIA_INVERTED,  _( "Type VI-a (filled and tented bottom)" ) },
        { IPC4761_PRESET::VII,           _( "Type VII (filled and capped)" ) },
        { IPC4761_PRESET::CUSTOM,        _( "Custom" ) },
        { IPC4761_PRESET::END,           _( "End" ) }
    };

    IPC4761_SURFACE getProtectionSurface( const std::optional<bool>& front, const std::optional<bool>& back )
    {
        IPC4761_SURFACE value = IPC4761_SURFACE::CUSTOM;

        if( !front.has_value() )
            value = IPC4761_SURFACE::FROM_BOARD;
        else if( front.value() )
            value = IPC4761_SURFACE::FRONT;
        else
            value = IPC4761_SURFACE::NONE;

        if( !back.has_value() )
        {
            if( value == IPC4761_SURFACE::FROM_BOARD )
                return IPC4761_SURFACE::FROM_BOARD;
        }
        else if( back.value() )
        {
            if( value == IPC4761_SURFACE::FRONT )
                return IPC4761_SURFACE::BOTH;
            else if( value == IPC4761_SURFACE::NONE )
                return IPC4761_SURFACE::BACK;
        }
        else
        {
            if( value == IPC4761_SURFACE::FRONT )
                return IPC4761_SURFACE::FRONT;
            else if( value == IPC4761_SURFACE::NONE )
                return IPC4761_SURFACE::NONE;
        }

        return IPC4761_SURFACE::CUSTOM;
    };

    IPC4761_DRILL getProtectionDrill( const std::optional<bool>& drill )
    {
        if( !drill.has_value() )
            return IPC4761_DRILL::FROM_BOARD;
        if( drill.value() )
            return IPC4761_DRILL::SET;

        return IPC4761_DRILL::NOT_SET;
    };

    IPC4761_PRESET getViaConfiguration( const PCB_VIA* aVia )
    {
        IPC4761_CONFIGURATION config;
        config.tent = getProtectionSurface( aVia->Padstack().FrontOuterLayers().has_solder_mask,
                                            aVia->Padstack().BackOuterLayers().has_solder_mask );

        config.cover = getProtectionSurface( aVia->Padstack().FrontOuterLayers().has_covering,
                                             aVia->Padstack().BackOuterLayers().has_covering );

        config.plug = getProtectionSurface( aVia->Padstack().FrontOuterLayers().has_plugging,
                                            aVia->Padstack().BackOuterLayers().has_plugging );

        config.cap = getProtectionDrill( aVia->Padstack().Drill().is_capped );

        config.fill = getProtectionDrill( aVia->Padstack().Drill().is_filled );

        for( const auto& [preset, configuration] : m_IPC4761Presets )
        {
            if( configuration == config )
                return preset;
        }

        return IPC4761_PRESET::CUSTOM;
    };

    void setSurfaceProtection( std::optional<bool>& aFront, std::optional<bool>& aBack,
                               IPC4761_SURFACE aProtection )
    {
        switch( aProtection )
        {
        case IPC4761_SURFACE::FROM_BOARD:
            aFront.reset();
            aBack.reset();
            break;
        case IPC4761_SURFACE::NONE:
            aFront = false;
            aBack = false;
            break;
        case IPC4761_SURFACE::FRONT:
            aFront = true;
            aBack = false;
            break;
        case IPC4761_SURFACE::BACK:
            aFront = false;
            aBack = true;
            break;
        case IPC4761_SURFACE::BOTH:
            aFront = true;
            aBack = true;
            break;
        case IPC4761_SURFACE::CUSTOM:
            return;
        }
    };

    void setDrillProtection( std::optional<bool>& aDrill, IPC4761_DRILL aProtection )
    {
        switch( aProtection )
        {
        case IPC4761_DRILL::FROM_BOARD: aDrill.reset(); break;
        case IPC4761_DRILL::NOT_SET:    aDrill = false; break;
        case IPC4761_DRILL::SET:        aDrill = true;  break;
        }
    };

    void setViaConfiguration( PCB_VIA* aVia, const IPC4761_PRESET& aPreset )
    {
        if( aPreset < IPC4761_PRESET::CUSTOM ) // Do not change custom feaure list.
        {
            const IPC4761_CONFIGURATION config = m_IPC4761Presets.at( aPreset );

            setSurfaceProtection( aVia->Padstack().FrontOuterLayers().has_solder_mask,
                                  aVia->Padstack().BackOuterLayers().has_solder_mask,
                                  config.tent );

            setSurfaceProtection( aVia->Padstack().FrontOuterLayers().has_plugging,
                                  aVia->Padstack().BackOuterLayers().has_plugging,
                                  config.plug );

            setSurfaceProtection( aVia->Padstack().FrontOuterLayers().has_covering,
                                  aVia->Padstack().BackOuterLayers().has_covering,
                                  config.cover );

            setDrillProtection( aVia->Padstack().Drill().is_filled, config.fill );

            setDrillProtection( aVia->Padstack().Drill().is_capped, config.cap );
        }
    }
};
