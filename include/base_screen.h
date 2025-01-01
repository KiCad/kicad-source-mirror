/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file base_screen.h
 * @brief BASE_SCREEN class implementation.
 */

#ifndef  BASE_SCREEN_H
#define  BASE_SCREEN_H

#include <eda_item.h>


/**
 * Handles how to draw a screen (a board, a schematic ...)
 */
class BASE_SCREEN : public EDA_ITEM
{
public:
    BASE_SCREEN( EDA_ITEM* aParent, KICAD_T aType = SCREEN_T );

    BASE_SCREEN( const VECTOR2I& aPageSizeIU, KICAD_T aType = SCREEN_T ) :
            BASE_SCREEN( nullptr, aType )
    {
        InitDataPoints( aPageSizeIU );
    }

    BASE_SCREEN( KICAD_T aType = SCREEN_T ) :
            BASE_SCREEN( nullptr, aType )
    {}

    ~BASE_SCREEN() override { }

    void InitDataPoints( const VECTOR2I& aPageSizeInternalUnits );

    void SetContentModified( bool aModified = true )    { m_flagModified = aModified; }
    bool IsContentModified() const                      { return m_flagModified; }

    /**
     * Return the class name.
     *
     * @return wxString
     */
    virtual wxString GetClass() const override
    {
        return wxT( "BASE_SCREEN" );
    }

    int GetPageCount() const { return m_pageCount; }
    void SetPageCount( int aPageCount );

    int GetVirtualPageNumber() const { return m_virtualPageNumber; }
    void SetVirtualPageNumber( int aPageNumber ) { m_virtualPageNumber = aPageNumber; }

    const wxString& GetPageNumber() const;
    void SetPageNumber( const wxString& aPageNumber ) { m_pageNumber = aPageNumber; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static wxString m_DrawingSheetFileName;  ///< the name of the drawing sheet file, or empty
                                             ///< to use the default drawing sheet

    VECTOR2I    m_DrawOrg;          ///< offsets for drawing the circuit on the screen

    VECTOR2D    m_LocalOrigin;      ///< Relative Screen cursor coordinate (on grid)
                                    ///< in user units. (coordinates from last reset position)

    VECTOR2I    m_StartVisu;        ///< Coordinates in drawing units of the current
                                    ///< view position (upper left corner of device)

    bool        m_Center;           ///< Center on screen.  If true (0.0) is centered on screen
                                    ///< coordinates can be < 0 and > 0 except for schematics.
                                    ///< false: when coordinates can only be >= 0 (schematics).

    VECTOR2D    m_ScrollCenter;     ///< Current scroll center point in logical units.

protected:
    /**
     * The number of #BASE_SCREEN objects in this design.
     *
     * This currently only has meaning for #SCH_SCREEN objects because #PCB_SCREEN object
     * are limited to a single file.  The count is virtual because #SCH_SCREEN objects can be
     * used more than once so the screen (page) count can be more than the number of screen
     * objects.
     */
    int         m_pageCount;

    /**
     * An integer based page number used for printing a range of pages.
     *
     * This page number is set before printing and plotting because page numbering does not
     * reflect the actual page number in complex hiearachies in #SCH_SCREEN objects.
     */
    int         m_virtualPageNumber;

    /**
     * A user defined string page number used for printing and plotting.
     *
     * This currently only has meaning for #SCH_SCREEN objects because #PCB_SCREEN object
     * are limited to a single file.  This must be set before displaying, printing, or
     * plotting the current sheet.  If empty, the #m_virtualPageNumber value is converted
     * to a string.
     */
    wxString    m_pageNumber;

private:
    bool        m_flagModified;     ///< Indicates current drawing has been modified.
};

#endif  // BASE_SCREEN_H
