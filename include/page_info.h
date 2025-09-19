/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#pragma once

#include <map>
#include <kicommon.h>
#include <wx/string.h>
#include <math/vector2d.h>

/// Min and max page sizes for clamping, in mils.
#define MIN_PAGE_SIZE_MILS          1000
#define MAX_PAGE_SIZE_PCBNEW_MILS   48000
#define MAX_PAGE_SIZE_EESCHEMA_MILS 120000

/// Min and max page sizes for clamping, in mm.
#define MIN_PAGE_SIZE_MM            25.4
#define MAX_PAGE_SIZE_PCBNEW_MM     48000*.0254
#define MAX_PAGE_SIZE_EESCHEMA_MM   120000*.0254

class OUTPUTFORMATTER;

/*
 * @brief Standard paper sizes nicknames
 * Do not rename entires as these names are saved to file and parsed back
 */
enum class PAGE_SIZE_TYPE
{
    A5,
    A4,
    A3,
    A2,
    A1,
    A0,
    A,
    B,
    C,
    D,
    E,
    GERBER,
    USLetter,
    USLegal,
    USLedger,
    User
};

/**
 * Describe the page size and margins of a paper page on which to eventually print or plot.
 *
 * Paper sizes are often described in inches.  Here paper is described in 1/1000th of an
 * inch (mils).  For convenience there are some read only accessors for internal units
 * which is a compile time calculation, not runtime.
 *
 * @author Dick Hollenbeck
 */
class KICOMMON_API PAGE_INFO
{
public:
    PAGE_INFO( PAGE_SIZE_TYPE aType = PAGE_SIZE_TYPE::A3, bool IsPortrait = false );

    // paper size names which are part of the public API, pass to SetType() or
    // above constructor.


    /**
     * Set the name of the page type and also the sizes and margins commonly associated with
     * that type name.
     *
     * @param aStandardPageDescriptionName is a wxString constant giving one of:
     * "A5" "A4" "A3" "A2" "A1" "A0" "A" "B" "C" "D" "E" "GERBER", "USLetter", "USLegal",
     * "USLedger", or "User".  If "User" then the width and height are custom,
     * and will be set according to <b>previous</b> calls to
     * static PAGE_INFO::SetUserWidthMils() and
     * static PAGE_INFO::SetUserHeightMils();
     * @param aIsPortrait Set to true to set page orientation to portrait mode.
     * @return true if @a aStandarePageDescription was a recognized type.
     */
    bool                  SetType( PAGE_SIZE_TYPE aPageSize, bool aIsPortrait = false );
    bool                  SetType( const wxString& aPageSize, bool aIsPortrait = false );
    const PAGE_SIZE_TYPE& GetType() const { return m_type; }
    wxString       GetTypeAsString() const;

    const wxString& GetPageFormatDescription() const { return m_description;  }

    /**
     * @return True if the object has the default page settings which are A3, landscape.
     */
    bool IsDefault() const { return m_type == PAGE_SIZE_TYPE::A3 && !m_portrait; }

    /**
     * @return true if the type is Custom.
     */
    bool IsCustom() const;

    /**
     * Rotate the paper page 90 degrees.
     *
     * This PAGE_INFO may either be in portrait or landscape mode.  Use this function to
     * change from one mode to the other mode.
     *
     * @param aIsPortrait if true and not already in portrait mode, will change this
     *                    PAGE_INFO to portrait mode.  Or if false and not already in
     *                    landscape mode, will change this PAGE_INFO to landscape mode.
     */
    void SetPortrait( bool aIsPortrait );
    bool IsPortrait() const { return m_portrait; }

    /**
     * @return ws' style printing orientation (wxPORTRAIT or wxLANDSCAPE).
     */
    wxPrintOrientation  GetWxOrientation() const { return IsPortrait() ? wxPORTRAIT : wxLANDSCAPE; }

    /**
     * @return wxPrintData's style paper id associated with page type name.
     */
    wxPaperSize GetPaperId() const { return m_paper_id; }

    void SetWidthMM(  double aWidthInMM ) { SetWidthMils( aWidthInMM * 1000 / 25.4 ); }
    void SetWidthMils(  double aWidthInMils );
    double GetWidthMils() const { return m_size.x; }
    double GetWidthMM() const { return m_size.x * 25.4 / 1000; }

    void SetHeightMM( double aHeightInMM ) { SetHeightMils( aHeightInMM * 1000 / 25.4 ); }
    void SetHeightMils( double aHeightInMils );
    double GetHeightMils() const { return m_size.y; }
    double GetHeightMM() const { return m_size.y * 25.4 / 1000; }

    const VECTOR2D& GetSizeMils() const { return m_size; }

    /**
     * Gets the page width in IU
     *
     * @param aIUScale The IU scale, this is most likely always going to be IU_PER_MILS
     * variable being passed. Note, this constexpr variable changes depending
     * on application, hence why it is passed.
     */
    int GetWidthIU( double aIUScale ) const { return aIUScale * GetWidthMils(); }

    /**
     * Gets the page height in IU
     *
     * @param aIUScale The IU scale, this is most likely always going to be IU_PER_MILS
     * variable being passed. Note, this constexpr variable changes depending
     * on application, hence why it is passed.
     */
    int GetHeightIU( double aIUScale ) const { return aIUScale * GetHeightMils(); }

    /**
     * Gets the page size in internal units
     *
     * @param aIUScale The IU scale, this is most likely always going to be IU_PER_MILS
     * variable being passed. Note, this constexpr variable changes depending
     * on application, hence why it is passed.
     */
    const VECTOR2D GetSizeIU( double aIUScale ) const
    {
        return VECTOR2D( GetWidthIU( aIUScale ), GetHeightIU( aIUScale ) );
    }

    /**
     * Set the width of Custom page in mils for any custom page constructed or made via
     * SetType() after making this call.
     */
    static void SetCustomWidthMils( double aWidthInMils );

    /**
     * Set the height of Custom page in mils for any custom page constructed or made via
     * SetType() after making this call.
     */
    static void SetCustomHeightMils( double aHeightInMils );

    /**
     * @return custom paper width in mils.
     */
    static double GetCustomWidthMils() { return s_user_width; }

    /**
     * @return custom paper height in mils.
     */
    static double GetCustomHeightMils() { return s_user_height; }

    /**
     * Output the page class to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @throw IO_ERROR on write error.
     */
    void Format( OUTPUTFORMATTER* aFormatter ) const;

    static const std::vector<PAGE_INFO>& GetPageFormatsList();

protected:
    // only the class implementation(s) may use this constructor
    PAGE_INFO( const VECTOR2D& aSizeMils, const PAGE_SIZE_TYPE& aType, wxPaperSize aPaperId,
               const wxString& aDescription = wxEmptyString );

    void    updatePortrait();

private:
    static std::vector<PAGE_INFO> standardPageSizes;

    // all dimensions here are in mils

    PAGE_SIZE_TYPE m_type;             ///< paper type: A4, A3, etc.
    VECTOR2D       m_size;             ///< mils

    bool           m_portrait;         ///< true if portrait, false if landscape

    wxPaperSize    m_paper_id;         ///< wx' style paper id.
    wxString       m_description;      ///< more human friendly description of page size

    static double s_user_height;
    static double s_user_width;
};
