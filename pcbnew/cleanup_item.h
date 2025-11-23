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

#ifndef CLEANUP_ITEM_H
#define CLEANUP_ITEM_H

#include <drc/drc_item.h>

class PCB_BASE_FRAME;


enum CLEANUP_RC_CODE {
    CLEANUP_FIRST = DRCE_LAST + 1,
    CLEANUP_SHORTING_TRACK = CLEANUP_FIRST,
    CLEANUP_SHORTING_VIA,
    CLEANUP_REDUNDANT_VIA,
    CLEANUP_DUPLICATE_TRACK,
    CLEANUP_MERGE_TRACKS,
    CLEANUP_DANGLING_TRACK,
    CLEANUP_DANGLING_VIA,
    CLEANUP_ZERO_LENGTH_TRACK,
    CLEANUP_TRACK_IN_PAD,
    CLEANUP_NULL_GRAPHIC,
    CLEANUP_DUPLICATE_GRAPHIC,
    CLEANUP_LINES_TO_RECT,
    CLEANUP_MERGE_PAD
};

class CLEANUP_ITEM : public RC_ITEM
{
public:
    CLEANUP_ITEM( int aErrorCode );

    /**
     * Return the string form of a drc error code.
     */
    wxString GetErrorText( int aErrorCode = -1, bool aTranslate = true ) const;

private:
    wxString m_errorMessage;
};


/**
 * An implementation of the interface named RC_ITEMS_PROVIDER which uses a vector
 * of pointers to CLEANUP_ITEMs to fulfill the interface.  No ownership is taken of the
 * vector.
 */
class VECTOR_CLEANUP_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
public:
    VECTOR_CLEANUP_ITEMS_PROVIDER( std::vector<std::shared_ptr<CLEANUP_ITEM> >* aList ) :
            m_sourceVector( aList )
    {
    }

    void SetSeverities( int aSeverities ) override
    {
    }

    int GetSeverities() const override
    {
        return 0;
    }

    int  GetCount( int aSeverity = -1 ) const override
    {
        return m_sourceVector->size();
    }

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override
    {
        return m_sourceVector->at( aIndex );
    }

    std::shared_ptr<CLEANUP_ITEM> GetCleanupItem( int aIndex )
    {
        return m_sourceVector->at( aIndex );
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        if( aDeep )
        {
            auto item = m_sourceVector->at( aIndex );
            m_sourceVector->erase( m_sourceVector->begin() + aIndex );
        }
    }

private:
    std::vector<std::shared_ptr<CLEANUP_ITEM> >* m_sourceVector;     // owns its CLEANUP_ITEMs
};



#endif      // CLEANUP_ITEM_H
