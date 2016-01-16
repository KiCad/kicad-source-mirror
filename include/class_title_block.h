/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#ifndef CLASS_TITLE_BLOCK_H_
#define CLASS_TITLE_BLOCK_H_

#include <wx/string.h>


class OUTPUTFORMATTER;
struct IO_ERROR;


/**
 * Class TITLE_BLOCK
 * holds the information shown in the lower right corner of a plot, printout, or
 * editing view.
 *
 * @author Dick Hollenbeck
 */
class TITLE_BLOCK
{
    // Texts are stored in wxArraystring.
    // textsIdx gives the index of known texts in
    // this array
    enum textsIdx
    {
        titleIdx = 0,
        dateIdx,
        revisionIdx,
        companyIdx,
        m_commentIdx
    };

public:

    TITLE_BLOCK() {};
    virtual ~TITLE_BLOCK() {};      // a virtual dtor seems needed to build
                                    // python lib without warning

    void SetTitle( const wxString& aTitle )
    {
        setTbText( titleIdx, aTitle );
    }

    const wxString& GetTitle() const
    {
        return getTbText( titleIdx );
    }

    /**
     * Function SetDate
     * sets the date field, and defaults to the current time and date.
     */
    void SetDate( const wxString& aDate )
    {
        setTbText( dateIdx, aDate );
    }

    const wxString& GetDate() const
    {
        return getTbText( dateIdx );
    }

    void SetRevision( const wxString& aRevision )
    {
        setTbText( revisionIdx, aRevision );
    }

    const wxString& GetRevision() const
    {
        return getTbText( revisionIdx );
    }

    void SetCompany( const wxString& aCompany )
    {
        setTbText( companyIdx, aCompany );
    }

    const wxString& GetCompany() const
    {
        return getTbText( companyIdx );
    }

    void SetComment(  int aIdx, const wxString& aComment )
    {
        aIdx += m_commentIdx;
        return setTbText( aIdx, aComment );
    }

    const wxString& GetComment( int aIdx ) const
    {
        aIdx += m_commentIdx;
        return getTbText( aIdx );
    }

    // Only for old code compatibility. Will be removed later
    void SetComment1( const wxString& aComment ) { SetComment( 0, aComment ); }
    void SetComment2( const wxString& aComment ) { SetComment( 1, aComment ); }
    void SetComment3( const wxString& aComment ) { SetComment( 2, aComment ); }
    void SetComment4( const wxString& aComment ) { SetComment( 3, aComment ); }
    const wxString& GetComment1( ) const { return GetComment( 0 ); }
    const wxString& GetComment2( ) const { return GetComment( 1 ); }
    const wxString& GetComment3( ) const { return GetComment( 2 ); }
    const wxString& GetComment4( ) const { return GetComment( 3 ); }


    void Clear()
    {
        m_tbTexts.Clear();
    }

    /**
     * Function Format
     * outputs the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
        throw( IO_ERROR );

private:
    wxArrayString m_tbTexts;

    void setTbText( int aIdx, const wxString& aText )
    {
        if( (int)m_tbTexts.GetCount() <= aIdx )
            m_tbTexts.Add( wxEmptyString, aIdx + 1 - m_tbTexts.GetCount() );
        m_tbTexts[aIdx] = aText;
    }

    const wxString& getTbText( int aIdx ) const
    {
        static const wxString m_emptytext;

        if( (int)m_tbTexts.GetCount() > aIdx )
            return m_tbTexts[aIdx];
        else
            return m_emptytext;
    }
};

#endif // CLASS_TITLE_BLOCK_H_
