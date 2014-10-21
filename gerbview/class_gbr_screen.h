/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gbr_screen.h
 */

#ifndef CLASS_GBR_SCREEN_H_
#define CLASS_GBR_SCREEN_H_


#include <base_units.h>
#include <class_base_screen.h>
#include <layers_id_colors_and_visibility.h>

#define ZOOM_FACTOR( x )       ( x * IU_PER_DECIMILS )


/* Handle info to display a board */
class GBR_SCREEN : public BASE_SCREEN
{
public:
    LAYER_NUM m_Active_Layer;
    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    GBR_SCREEN( const wxSize& aPageSizeIU );

    ~GBR_SCREEN();

    GBR_SCREEN* Next() const { return static_cast<GBR_SCREEN*>( Pnext ); }

//    void        SetNextZoom();
//    void        SetPreviousZoom();
//    void        SetLastZoom();

    virtual int MilsToIuScalar();

    /**
     * Function ClearUndoORRedoList
     * virtual pure in BASE_SCREEN, so it must be defined here
     */
    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );
};


#endif  // CLASS_GBR_SCREEN_H_
