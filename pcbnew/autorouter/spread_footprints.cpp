/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file spread_footprints.cpp
 * @brief functions to spread footprints on free areas outside a board.
 * this is usefull after reading a netlist, when new footprints are loaded
 * and stacked at 0,0 coordinate.
 * Often, spread them on a free area near the board being edited make more easy
 * their selection.
 */

#include <algorithm>
#include <boost/foreach.hpp>

#include <fctsys.h>
#include <convert_to_biu.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_module.h>

#include <rect_placement/rect_placement.h>

struct TSubRect : public CRectPlacement::TRect
{
    int n;      // Original index of this subrect, before sorting

    TSubRect() : TRect(),
        n( 0 )
    {
    }

    TSubRect( int _w, int _h, int _n ) :
        TRect( 0, 0, _w, _h ), n( _n ) { }
};

typedef std::vector<TSubRect> CSubRectArray;

// Use 0.01 mm units to calculate placement, to avoid long calculation time
const int scale = (int)(0.01 * IU_PER_MM);

// Populates a list of rectangles, from a list of modules
void fillRectList( CSubRectArray& vecSubRects, std::vector <MODULE*>& aModuleList )
{
    vecSubRects.clear();

    for( unsigned ii = 0; ii < aModuleList.size(); ii++ )
    {
        EDA_RECT fpBox = aModuleList[ii]->GetBoundingBox();
        TSubRect fpRect( fpBox.GetWidth()/scale, fpBox.GetHeight()/scale, ii );
        vecSubRects.push_back( fpRect );
    }
}

// Populates a list of rectangles, from a list of EDA_RECT
void fillRectList( CSubRectArray& vecSubRects, std::vector <EDA_RECT>& aRectList )
{
    vecSubRects.clear();

    for( unsigned ii = 0; ii < aRectList.size(); ii++ )
    {
        EDA_RECT& rect = aRectList[ii];
        TSubRect fpRect( rect.GetWidth()/scale, rect.GetHeight()/scale, ii );
        vecSubRects.push_back( fpRect );
    }
}



// Spread a list of rectangles inside a placement area
void spreadRectangles( CRectPlacement& aPlacementArea,
                       CSubRectArray& vecSubRects,
                       int areaSizeX, int areaSizeY )
{
    areaSizeX/= scale;
    areaSizeY/= scale;

    // Sort the subRects based on dimensions, larger dimension goes first.
    std::sort( vecSubRects.begin(), vecSubRects.end(), CRectPlacement::TRect::Greater );

    // gives the initial size to the area
    aPlacementArea.Init( areaSizeX, areaSizeY );

    // Add all subrects
    CSubRectArray::iterator it;
    for( it = vecSubRects.begin(); it != vecSubRects.end(); )
    {
        CRectPlacement::TRect r( 0, 0, it->w, it->h );

        bool bPlaced = aPlacementArea.AddAtEmptySpotAutoGrow( &r, areaSizeX, areaSizeY );

        if( !bPlaced )   // No room to place the rectangle: enlarge area and retry
        {
            areaSizeX = ceil(areaSizeX * 1.1);
            areaSizeY = ceil(areaSizeY * 1.1);
            aPlacementArea.Init( areaSizeX, areaSizeY );
            it = vecSubRects.begin();
            continue;
        }

        // When correctly placed in a placement area, the coords are returned in r.x and r.y
        // Store them.
        it->x   = r.x;
        it->y   = r.y;

        it++;
    }
}


void moveFootprintsInArea( CRectPlacement& aPlacementArea,
                           std::vector <MODULE*>& aModuleList,
                           EDA_RECT& aFreeArea,
                           bool aFindAreaOnly )
{
    CSubRectArray   vecSubRects;

    fillRectList( vecSubRects, aModuleList );
    spreadRectangles( aPlacementArea, vecSubRects,
                      aFreeArea.GetWidth(), aFreeArea.GetHeight() );

    if( aFindAreaOnly )
        return;

    for( unsigned it = 0; it < vecSubRects.size(); ++it )
    {
        wxPoint pos( vecSubRects[it].x, vecSubRects[it].y );
        pos.x *= scale;
        pos.y *= scale;

        MODULE * module = aModuleList[vecSubRects[it].n];

        EDA_RECT fpBBox = module->GetBoundingBox();
        wxPoint mod_pos = pos + ( module->GetPosition() - fpBBox.GetOrigin() )
                          + aFreeArea.GetOrigin();

        module->Move( mod_pos - module->GetPosition() );
    }
}

static bool sortFootprintsbySheetPath( MODULE* ref, MODULE* compare );

/* Function to move components in a rectangular area format 4 / 3,
 * starting from the mouse cursor
 * The components with the FIXED status set are not moved
 */
void PCB_EDIT_FRAME::SpreadFootprints( std::vector<MODULE*>* aFootprints,
                                       bool aMoveFootprintsOutsideBoardOnly,
                                       bool aCheckForBoardEdges )
{
    EDA_RECT bbox = GetBoard()->ComputeBoundingBox( true );
    bool     edgesExist = ( bbox.GetWidth() || bbox.GetHeight() );
    // if aFootprintsOutsideBoardOnly is true, and if board outline exists,
    // wue have to filter footprints to move:
    bool outsideBrdFilter = aMoveFootprintsOutsideBoardOnly && edgesExist;

    // no edges exist
    if( aMoveFootprintsOutsideBoardOnly && !edgesExist )
    {
        DisplayError( this,
                      _( "Could not automatically place footprints. No board outlines detected." ) );
        return;
    }


    // Build candidate list
    // calculate also the area needed by these footprints
    std::vector <MODULE*> footprintList;

    BOOST_FOREACH( MODULE* footprint, *aFootprints )
    {
        footprint->CalculateBoundingBox();

        if( outsideBrdFilter )
        {
            if( bbox.Contains( footprint->GetPosition() ) )
                continue;
        }

        if( footprint->IsLocked() )
            continue;

        footprintList.push_back( footprint );
    }

    if( footprintList.empty() )
        return;

    // sort footprints by sheet path. we group them later by sheet
    sort( footprintList.begin(), footprintList.end(), sortFootprintsbySheetPath );

    // Undo command: init undo list
    PICKED_ITEMS_LIST  undoList;
    undoList.m_Status = UR_CHANGED;
    ITEM_PICKER        picker( NULL, UR_CHANGED );

    BOOST_FOREACH( MODULE* footprint, footprintList )
    {
        // Undo: add copy of the footprint to undo list
        picker.SetItem( footprint );
        picker.SetLink( footprint->Clone() );
        undoList.PushItem( picker );
    }

    // Extract and place footprints by sheet
    std::vector <MODULE*> footprintListBySheet;
    std::vector <EDA_RECT> placementSheetAreas;
    double subsurface;
    double placementsurface = 0.0;

    wxPoint placementAreaPosition = GetCrossHairPosition();

    // We sometimes do not want to move footprints inside an existing board.
    // move the placement area position outside the board bounding box
    // to the left of the board
    if( edgesExist && aCheckForBoardEdges )
    {
        if( placementAreaPosition.x < bbox.GetEnd().x &&
            placementAreaPosition.y < bbox.GetEnd().y )
        {
            placementAreaPosition.x = bbox.GetEnd().x;
            placementAreaPosition.y = bbox.GetOrigin().y;
        }
    }

    // The placement uses 2 passes:
    // the first pass creates the rectangular areas to place footprints
    // each sheet in schematic creates one rectangular area.
    // the second pass moves footprints inside these areas
    MODULE* footprint;
    for( int pass = 0; pass < 2; pass++ )
    {
        int subareaIdx = 0;
        footprintListBySheet.clear();
        subsurface = 0.0;

        for( unsigned ii = 0; ii < footprintList.size(); ii++ )
        {
            footprint = footprintList[ii];
            bool islastItem = false;

            if( ii == footprintList.size() - 1 ||
                ( footprintList[ii]->GetPath().BeforeLast( '/' ) !=
                  footprintList[ii+1]->GetPath().BeforeLast( '/' ) ) )
                islastItem = true;

            footprintListBySheet.push_back( footprint );
            subsurface += footprint->GetArea();

            if( islastItem )
            {
                // end of the footprint sublist relative to the same sheet path
                // calculate placement of the current sublist
                EDA_RECT freeArea;
                int Xsize_allowed = (int) ( sqrt( subsurface ) * 4.0 / 3.0 );
                int Ysize_allowed = (int) ( subsurface / Xsize_allowed );

                freeArea.SetWidth( Xsize_allowed );
                freeArea.SetHeight( Ysize_allowed );
                CRectPlacement placementArea;

                if( pass == 1 )
                {
                    wxPoint areapos = placementSheetAreas[subareaIdx].GetOrigin()
                                      + placementAreaPosition;
                    freeArea.SetOrigin( areapos );
                }

                bool findAreaOnly = pass == 0;
                moveFootprintsInArea( placementArea, footprintListBySheet,
                                      freeArea, findAreaOnly );

                if( pass == 0 )
                {
                    // Populate sheet placement areas list
                    EDA_RECT sub_area;
                    sub_area.SetWidth( placementArea.GetW()*scale );
                    sub_area.SetHeight( placementArea.GetH()*scale );
                    // Add a margin around the sheet placement area:
                    sub_area.Inflate( Millimeter2iu( 1.5 ) );

                    placementSheetAreas.push_back( sub_area );

                    placementsurface += (double) sub_area.GetWidth()*
                                        sub_area.GetHeight();
                }

                // Prepare buffers for next sheet
                subsurface  = 0.0;
                footprintListBySheet.clear();
                subareaIdx++;
            }
        }

        // End of pass:
        // At the end of the first pass, we have to find position of each sheet
        // placement area
        if( pass == 0 )
        {
            int Xsize_allowed = (int) ( sqrt( placementsurface ) * 4.0 / 3.0 );
            int Ysize_allowed = (int) ( placementsurface / Xsize_allowed );
            CRectPlacement placementArea;
            CSubRectArray  vecSubRects;

            fillRectList( vecSubRects, placementSheetAreas );
            spreadRectangles( placementArea, vecSubRects, Xsize_allowed, Ysize_allowed );

            for( unsigned it = 0; it < vecSubRects.size(); ++it )
            {
                TSubRect& srect = vecSubRects[it];
                wxPoint pos( srect.x*scale, srect.y*scale );
                wxSize size( srect.w*scale, srect.h*scale );
                placementSheetAreas[srect.n].SetOrigin( pos );
                placementSheetAreas[srect.n].SetSize( size );
            }
        }
    }   // End pass

    // Undo: commit list
    SaveCopyInUndoList( undoList, UR_CHANGED );
    OnModify();

    m_canvas->Refresh();
}


// Sort function, used to group footprints by sheet.
// Footprints are sorted by their sheet path.
// (the full sheet path restricted to the time stamp of the sheet itself,
// without the time stamp of the footprint ).
static bool sortFootprintsbySheetPath( MODULE* ref, MODULE* compare )
{
    if( ref->GetPath().Length() == compare->GetPath().Length() )
        return ref->GetPath().BeforeLast( '/' ).Cmp( compare->GetPath().BeforeLast( '/' ) ) < 0;

    return ref->GetPath().Length() < compare->GetPath().Length();
}
