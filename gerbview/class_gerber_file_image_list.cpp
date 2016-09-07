/**
 * @file class_gerber_file_image.cpp
 * a GERBER class handle for a given layer info about used D_CODES and how the layer is drawn
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <fctsys.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <class_X2_gerber_attributes.h>

#include <map>


// The global image list:
GERBER_FILE_IMAGE_LIST s_GERBER_List;


// GERBER_FILE_IMAGE_LIST is a helper class to handle a list of GERBER_FILE_IMAGE files
GERBER_FILE_IMAGE_LIST::GERBER_FILE_IMAGE_LIST()
{
    m_GERBER_List.reserve( GERBER_DRAWLAYERS_COUNT );

    for( unsigned layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
        m_GERBER_List.push_back( NULL );
}


GERBER_FILE_IMAGE_LIST::~GERBER_FILE_IMAGE_LIST()
{
    DeleteAllImages();
}


GERBER_FILE_IMAGE_LIST& GERBER_FILE_IMAGE_LIST::GetImagesList()
{
    return s_GERBER_List;
}


GERBER_FILE_IMAGE* GERBER_FILE_IMAGE_LIST::GetGbrImage( int aIdx )
{
    if( (unsigned)aIdx < m_GERBER_List.size() )
        return m_GERBER_List[aIdx];

    return NULL;
}

/* creates a new, empty GERBER_FILE_IMAGE* at index aIdx
 * or at the first free location if aIdx < 0
 * aIdx = the index of graphic layer to use, or -1 to uses the first free graphic layer
 * return the index actually used, or -1 if no room to add image
 */
int GERBER_FILE_IMAGE_LIST::AddGbrImage( GERBER_FILE_IMAGE* aGbrImage, int aIdx )
{
    int idx = aIdx;

    if( idx < 0 )
    {
        for( idx = 0; idx < (int)m_GERBER_List.size(); idx++ )
        {
            if( m_GERBER_List[idx] == NULL )
                break;
        }
    }

    if( idx >= (int)m_GERBER_List.size() )
        return -1;  // No room

    m_GERBER_List[idx] = aGbrImage;

    return idx;
}


void GERBER_FILE_IMAGE_LIST::DeleteAllImages()
{
    for( unsigned idx = 0; idx < m_GERBER_List.size(); ++idx )
        DeleteImage( idx );
}


void GERBER_FILE_IMAGE_LIST::DeleteImage( int aIdx )
{
    // Ensure the index is valid:
    if( aIdx < 0 || aIdx >= int( m_GERBER_List.size() ) )
        return;

    // delete image aIdx
    GERBER_FILE_IMAGE* gbr_image = GetGbrImage( aIdx );

    delete gbr_image;
    m_GERBER_List[ aIdx ] = NULL;
}

// Build a name for image aIdx which can be used in layers manager
const wxString GERBER_FILE_IMAGE_LIST::GetDisplayName( int aIdx, bool aNameOnly )
{
    wxString name;

    GERBER_FILE_IMAGE* gerber = NULL;

    if( aIdx >= 0 && aIdx < (int)m_GERBER_List.size() )
        gerber = m_GERBER_List[aIdx];

    // if a file is loaded, build the name:
    // <id> <short filename> <X2 FileFunction info> if a X2 FileFunction info is found
    // or (if no FileFunction info)
    // <id> <short filename> *
    if( gerber )
    {
        wxFileName fn( gerber->m_FileName );
        wxString filename = fn.GetFullName();

        // if the filename is too long, display a shortened name:
        const int maxlen = 30;

        if( filename.Length() > maxlen )
        {
            wxString shortenedfn = filename.Left(2) + "..." + filename.Right(maxlen-5);
            filename = shortenedfn;
        }

        if( gerber->m_FileFunction )
        {
            if( gerber->m_FileFunction->IsCopper() )
            {
                name.Printf( "%s (%s, %s, %s)",
                             filename.GetData(),
                             GetChars( gerber->m_FileFunction->GetFileType() ),
                             GetChars( gerber->m_FileFunction->GetBrdLayerId() ),
                             GetChars( gerber->m_FileFunction->GetBrdLayerSide() ) );
            }
            else
            {
                name.Printf( "%s (%s, %s)",
                             filename.GetData(),
                             GetChars( gerber->m_FileFunction->GetFileType() ),
                             GetChars( gerber->m_FileFunction->GetBrdLayerId() ) );
            }
        }
        else
            name = filename;

        if( aNameOnly )
            return name;

        wxString fullname;

        fullname.Printf( "%d ", aIdx + 1 );
        fullname << name;
        return fullname;
    }
    else
        name.Printf( _( "Graphic layer %d" ), aIdx + 1 );

    return name;
}



// Helper function, for std::sort.
// Sort loaded images by Z order priority, if they have the X2 FileFormat info
// returns true if the first argument (ref) is ordered before the second (test).
static bool sortZorder( const GERBER_FILE_IMAGE* const& ref, const GERBER_FILE_IMAGE* const& test )
{
    if( !ref && !test )
        return false;        // do not change order: no criteria to sort items

    if( !ref || !ref->m_InUse )
        return false;       // Not used: ref ordered after

    if( !test || !test->m_InUse )
        return true;        // Not used: ref ordered before

    if( !ref->m_FileFunction && !test->m_FileFunction )
        return false;        // do not change order: no criteria to sort items

    if( !ref->m_FileFunction )
        return false;

    if( !test->m_FileFunction )
        return true;

    if( ref->m_FileFunction->GetZOrder() != test->m_FileFunction->GetZOrder() )
        return ref->m_FileFunction->GetZOrder() > test->m_FileFunction->GetZOrder();

    return ref->m_FileFunction->GetZSubOrder() > test->m_FileFunction->GetZSubOrder();
}

void GERBER_FILE_IMAGE_LIST::SortImagesByZOrder()
{
    std::sort( m_GERBER_List.begin(), m_GERBER_List.end(), sortZorder );

    // The image order has changed.
    // Graphic layer numbering must be updated to match the widgets layer order

    // Store the old/new graphic layer info:
    std::map <int, int> tab_lyr;

    for( unsigned layer = 0; layer < m_GERBER_List.size(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = m_GERBER_List[layer];

        if( !gerber )
            continue;

        tab_lyr[gerber->m_GraphicLayer] = layer;
        gerber->m_GraphicLayer = layer ;
    }
}

