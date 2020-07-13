/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file base_screen.h
 * @brief BASE_SCREEN class implementation.
 */

#ifndef  BASE_SCREEN_H
#define  BASE_SCREEN_H

#include <eda_draw_frame.h>
#include <base_struct.h>
#include <common.h>


/**
 * BASE_SCREEN
 * handles how to draw a screen (a board, a schematic ...)
 */
class BASE_SCREEN : public EDA_ITEM
{
private:
    bool        m_FlagModified;     ///< Indicates current drawing has been modified.
    bool        m_FlagSave;         ///< Indicates automatic file save.

    /**
     * The cross hair position in logical (drawing) units.  The cross hair is not the cursor
     * position.  It is an addition indicator typically drawn on grid to indicate to the
     * user where the current action will be performed.
     */
    wxPoint     m_crossHairPosition;

public:
    static  wxString m_PageLayoutDescrFileName; ///< the name of the page layout descr file,
                                                ///< or emty to used the default pagelayout

    wxPoint     m_DrawOrg;          ///< offsets for drawing the circuit on the screen

    VECTOR2D    m_LocalOrigin;      ///< Relative Screen cursor coordinate (on grid)
                                    ///< in user units. (coordinates from last reset position)

    wxPoint     m_StartVisu;        /**< Coordinates in drawing units of the current
                                     * view position (upper left corner of device)
                                     */

    bool        m_Center;           /**< Center on screen.  If true (0.0) is centered
                                     * on screen coordinates can be < 0 and
                                     * > 0 except for schematics.
                                     * false: when coordinates can only be >= 0
                                     * Schematic */

    VECTOR2D    m_ScrollCenter;     ///< Current scroll center point in logical units.

    bool        m_Initialized;

    int         m_ScreenNumber;
    int         m_NumberOfScreens;

public:
    BASE_SCREEN( EDA_ITEM* aParent, KICAD_T aType = SCREEN_T );

    BASE_SCREEN( const wxSize& aPageSizeIU, KICAD_T aType = SCREEN_T ) :
            BASE_SCREEN( nullptr, aType )
    {
        InitDataPoints( aPageSizeIU );
    }

    BASE_SCREEN( KICAD_T aType = SCREEN_T ) :
            BASE_SCREEN( nullptr, aType )
    {}

    ~BASE_SCREEN() override { }

    void InitDataPoints( const wxSize& aPageSizeInternalUnits );


    void SetModify()        { m_FlagModified = true; }
    void ClrModify()        { m_FlagModified = false; }
    void SetSave()          { m_FlagSave = true; }
    void ClrSave()          { m_FlagSave = false; }
    bool IsModify() const   { return m_FlagModified; }
    bool IsSave() const     { return m_FlagSave; }

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const override
    {
        return wxT( "BASE_SCREEN" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif
};

#endif  // BASE_SCREEN_H
