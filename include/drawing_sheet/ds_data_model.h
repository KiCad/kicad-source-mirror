/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef  DS_DATA_MODEL_H
#define  DS_DATA_MODEL_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <bitmap_base.h>

class DS_DATA_ITEM;
class PAGE_INFO;

/**
 * Handle the graphic items list to draw/plot the frame and title block.
 */
class DS_DATA_MODEL
{
public:
    DS_DATA_MODEL();

    ~DS_DATA_MODEL()
    {
        ClearList();
    }

    /**
     * Return the instance of DS_DATA_MODEL used in the application.
     */
    static DS_DATA_MODEL& GetTheInstance();

    /**
     * Set an alternate instance of #DS_DATA_MODEL.
     *
     * @param aLayout the alternate drawing sheet; if null restore the default drawing sheet.
     */
    static void SetAltInstance( DS_DATA_MODEL* aLayout = nullptr );

    int GetFileFormatVersionAtLoad() { return m_fileFormatVersionAtLoad; }
    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }

    double GetLeftMargin() { return m_leftMargin; }
    void SetLeftMargin( double aMargin ) { m_leftMargin = aMargin; }

    double GetRightMargin() { return m_rightMargin; }
    void SetRightMargin( double aMargin ) { m_rightMargin = aMargin; }

    double GetTopMargin() { return m_topMargin; }
    void SetTopMargin( double aMargin ) { m_topMargin = aMargin; }

    double GetBottomMargin() { return m_bottomMargin; }
    void SetBottomMargin( double aMargin ) { m_bottomMargin = aMargin; }

    void SetupDrawEnvironment( const PAGE_INFO& aPageInfo, double aMilsToIU );

    /**
     * In KiCad applications, a drawing sheet is needed
     * So if the list is empty, a default drawing sheet is loaded, the first time it is drawn.
     * However, in drawing sheet editor an empty list is acceptable.
     * AllowVoidList allows or not the empty list
     */
    void AllowVoidList( bool Allow ) { m_allowVoidList = Allow; }

    /**
     * @return true if an empty list is allowed
     */
    bool VoidListAllowed() { return m_allowVoidList; }

    /**
     * Erase the list of items.
     */
    void ClearList();

    /**
     * Save the description in a file.
     *
     * @param aFullFileName the filename of the file to created.
     */
    void Save( const wxString& aFullFileName );

    /**
     * Save the description in a buffer.
     *
     * @param aOutputString is a wxString to store the S expr string
     */
    void SaveInString( wxString* aOutputString );

    /**
     * Fill the given string with an S-expr serialization of the WS_DATA_ITEMs.
     */
    void SaveInString( std::vector<DS_DATA_ITEM*>& aItemsList, wxString* aOutputString );

    void Append( DS_DATA_ITEM* aItem );
    void Remove( DS_DATA_ITEM* aItem );

    /**
     * @return is the item from its index \a aIdx, or NULL if does not exist.
     */
    DS_DATA_ITEM* GetItem( unsigned aIdx ) const;

    /**
     * @return a reference to the items.
     */
    std::vector<DS_DATA_ITEM*>& GetItems() { return m_list; }

    /**
     * @return the item count.
     */
    unsigned GetCount() const { return m_list.size(); }

    void SetDefaultLayout();
    void SetEmptyLayout();

    /**
     * Return a string containing the empty layout shape.
     */
    static wxString EmptyLayout();

    /**
     * Return a string containing the empty layout shape.
     */
    static wxString DefaultLayout();

    /**
     * Populate the list with a custom layout or the default layout if no custom layout
     * is available.
     *
     * @param aFullFileName is the custom drawing sheet file. If empty, load the file defined by
     *                      KICAD_WKSFILE and if its not defined, the default internal drawing
     *                      sheet.
     * @param aMsg [optional] if non-null, will be filled with any error messages.
     * @param aAppend if true: do not delete old layout, and load only \a aFullFileName.
     */
    bool LoadDrawingSheet( const wxString& aFullFileName, wxString* aMsg, bool aAppend = false );

    /**
     * Populate the list from a S expr description stored in a string.
     *
     * @param aPageLayout is the S expr string.
     * @param aAppend Do not delete old layout if true and append \a aPageLayout the existing
     *                one.
       @param aSource is the layout source description.
     */
    void SetPageLayout( const char* aPageLayout, bool aAppend = false,
                        const wxString& aSource = wxT( "Sexpr_string" )  );

    /**
     * Resolve a path which might be project-relative or contain env variable references.
     */
    static const wxString ResolvePath( const wxString& aPath, const wxString& aProjectPath );

    double   m_WSunits2Iu;            // conversion factor between
                                      // ws units (mils) and draw/plot units
    VECTOR2D m_RB_Corner;             // coordinates of the right bottom corner (in mm)
    VECTOR2D m_LT_Corner;             // coordinates of the left top corner (in mm)
    double   m_DefaultLineWidth;      // Used when object line width is 0
    VECTOR2D m_DefaultTextSize;       // Used when object text size is 0
    double   m_DefaultTextThickness;  // Used when object text stroke width is 0
    bool     m_EditMode;              // Used in drawing sheet editor to toggle variable substitution
                                      // In normal mode (m_EditMode = false) the %format is
                                      // replaced by the corresponding text.
                                      // In edit mode (m_EditMode = true) the %format is
                                      // displayed "as this"

private:
    std::vector <DS_DATA_ITEM*> m_list;
    bool     m_allowVoidList;         // If false, the default drawing sheet will be loaded the
                                      // first time DS_DRAW_ITEM_LIST::BuildDrawItemsList is run
                                      // (useful mainly for drawing sheet editor)
    int      m_fileFormatVersionAtLoad;
    double   m_leftMargin;            // the left page margin in mm
    double   m_rightMargin;           // the right page margin in mm
    double   m_topMargin;             // the top page margin in mm
    double   m_bottomMargin;          // the bottom page margin in mm
};

#endif      // DS_DATA_MODEL_H
