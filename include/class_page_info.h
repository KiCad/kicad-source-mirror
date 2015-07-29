/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_page_info.h
 */

#ifndef CLASS_PAGE_INFO_H_
#define CLASS_PAGE_INFO_H_

#include <wx/wx.h>

#include <richio.h>         // for OUTPUTFORMATTER and IO_ERROR
#include <base_units.h>     // for IU_PER_MILS


/**
 * Class PAGE_INFO
 * describes the page size and margins of a paper page on which to
 * eventually print or plot.  Paper sizes are often described in inches.
 * Here paper is described in 1/1000th of an inch (mils).  For convenience
 * there are some read only accessors for internal units (IU), which is a compile
 * time calculation, not runtime.
 *
 * @author Dick Hollenbeck
 */
class PAGE_INFO
{
public:

    PAGE_INFO( const wxString& aType = PAGE_INFO::A3, bool IsPortrait = false );

    // paper size names which are part of the public API, pass to SetType() or
    // above constructor.

    // these were once wxStrings, but it caused static construction sequence problems:
    static const wxChar A4[];
    static const wxChar A3[];
    static const wxChar A2[];
    static const wxChar A1[];
    static const wxChar A0[];
    static const wxChar A[];
    static const wxChar B[];
    static const wxChar C[];
    static const wxChar D[];
    static const wxChar E[];
    static const wxChar GERBER[];
    static const wxChar USLetter[];
    static const wxChar USLegal[];
    static const wxChar USLedger[];
    static const wxChar Custom[];     ///< "User" defined page type


    /**
     * Function SetType
     * sets the name of the page type and also the sizes and margins
     * commonly associated with that type name.
     *
     * @param aStandardPageDescriptionName is a wxString constant giving one of:
     * "A4" "A3" "A2" "A1" "A0" "A" "B" "C" "D" "E" "GERBER", "USLetter", "USLegal",
     * "USLedger", or "User".  If "User" then the width and height are custom,
     * and will be set according to <b>previous</b> calls to
     * static PAGE_INFO::SetUserWidthMils() and
     * static PAGE_INFO::SetUserHeightMils();
     * @param IsPortrait Set to true to set page orientation to portrait mode.
     *
     * @return bool - true if @a aStandarePageDescription was a recognized type.
     */
    bool SetType( const wxString& aStandardPageDescriptionName, bool IsPortrait = false );
    const wxString& GetType() const { return m_type; }

    /**
     * Function IsDefault
     * @return True if the object has the default page settings which are A3, landscape.
     */
    bool IsDefault() const { return m_type == PAGE_INFO::A3 && !m_portrait; }

    /**
     * Function IsCustom
     * returns true if the type is Custom
     */
    bool IsCustom() const;

    /**
     * Function SetPortrait
     * will rotate the paper page 90 degrees.  This PAGE_INFO may either be in
     * portrait or landscape mode.  Use this function to change from one to the
     * other mode.
     * @param isPortrait if true and not already in portrait mode, will change
     *  this PAGE_INFO to portrait mode.  Or if false and not already in landscape mode,
     *  will change this PAGE_INFO to landscape mode.
     */
    void SetPortrait( bool isPortrait );
    bool IsPortrait() const { return m_portrait; }

    /**
     * Function GetWxOrientation.
     * @return ws' style printing orientation (wxPORTRAIT or wxLANDSCAPE).
     */
    wxPrintOrientation  GetWxOrientation() const { return IsPortrait() ? wxPORTRAIT : wxLANDSCAPE; }

    /**
     * Function GetPaperId
     * @return wxPaperSize - wxPrintData's style paper id associated with
     * page type name.
     */
    wxPaperSize GetPaperId() const { return m_paper_id; }

    void SetWidthMils(  int aWidthInMils );
    int GetWidthMils() const { return m_size.x; }

    void SetHeightMils( int aHeightInMils );
    int GetHeightMils() const { return m_size.y; }

    const wxSize& GetSizeMils() const { return m_size; }

    // Accessors returning "Internal Units (IU)".  IUs are mils in EESCHEMA,
    // and either deci-mils or nanometers in PCBNew.
#if defined(PCBNEW) || defined(EESCHEMA) || defined(GERBVIEW) || defined(PL_EDITOR)
    int GetWidthIU() const  { return IU_PER_MILS * GetWidthMils();  }
    int GetHeightIU() const { return IU_PER_MILS * GetHeightMils(); }
    const wxSize GetSizeIU() const  { return wxSize( GetWidthIU(), GetHeightIU() ); }
#endif

    /**
     * Function SetCustomWidthMils
     * sets the width of Custom page in mils, for any custom page
     * constructed or made via SetType() after making this call.
     */
    static void SetCustomWidthMils( int aWidthInMils );

    /**
     * Function SetCustomHeightMils
     * sets the height of Custom page in mils, for any custom page
     * constructed or made via SetType() after making this call.
     */
    static void SetCustomHeightMils( int aHeightInMils );

    /**
     * Function GetCustomWidthMils.
     * @return int - custom paper width in mils.
     */
    static int GetCustomWidthMils() { return s_user_width; }

    /**
     * Function GetCustomHeightMils.
     * @return int - custom paper height in mils.
     */
    static int GetCustomHeightMils() { return s_user_height; }

    /**
     * Function GetStandardSizes
     * returns the standard page types, such as "A4", "A3", etc.
    static wxArrayString GetStandardSizes();
     */

    /**
     * Function Format
     * outputs the page class to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
        throw( IO_ERROR );

protected:
    // only the class implementation(s) may use this constructor
    PAGE_INFO( const wxSize& aSizeMils, const wxString& aName, wxPaperSize aPaperId );


private:

    // standard pre-defined sizes
    static const PAGE_INFO pageA4;
    static const PAGE_INFO pageA3;
    static const PAGE_INFO pageA2;
    static const PAGE_INFO pageA1;
    static const PAGE_INFO pageA0;
    static const PAGE_INFO pageA;
    static const PAGE_INFO pageB;
    static const PAGE_INFO pageC;
    static const PAGE_INFO pageD;
    static const PAGE_INFO pageE;
    static const PAGE_INFO pageGERBER;

    static const PAGE_INFO pageUSLetter;
    static const PAGE_INFO pageUSLegal;
    static const PAGE_INFO pageUSLedger;

    static const PAGE_INFO pageUser;

    // all dimensions here are in mils

    wxString    m_type;             ///< paper type: A4, A3, etc.
    wxSize      m_size;             ///< mils

/// Min and max page sizes for clamping.
#define MIN_PAGE_SIZE   4000
#define MAX_PAGE_SIZE   48000

    bool        m_portrait;         ///< true if portrait, false if landscape

    wxPaperSize m_paper_id;         ///< wx' style paper id.

    static int s_user_height;
    static int s_user_width;

    void    updatePortrait();

    void    setMargins();
};

#endif  // CLASS_PAGE_INFO_H_
