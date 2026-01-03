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

#ifndef GRID_ICON_TEXT_HELPERS_H
#define GRID_ICON_TEXT_HELPERS_H

#include <wx/bitmap.h>
#include <wx/bmpcbox.h>
#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>
#include <vector>
#include <kicommon.h>

class wxGrid;
enum class BITMAPS : unsigned int;


//---- Grid helpers: custom wxGridCellRenderer that renders icon and a label ------------

class KICOMMON_API GRID_CELL_ICON_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    /**
     * Construct a renderer that maps a list of icons from the bitmap system to a list of strings
     * @param icons is a list of possible icons to render
     * @param names is a list of names to render - must be the same length as icons
     */
    GRID_CELL_ICON_TEXT_RENDERER( const std::vector<BITMAPS>& icons, const wxArrayString& names );

    /**
     * Construct a renderer that renders a single icon next to the cell's value text
     * @param aIcon is the icon to render next to the cell's value
     */
    GRID_CELL_ICON_TEXT_RENDERER( const wxBitmapBundle& aIcon,
                                  wxSize aPreferredIconSize = wxDefaultSize );

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
               const wxRect& aRect, int aRow, int aCol, bool isSelected ) override;
    wxSize GetBestSize( wxGrid & grid, wxGridCellAttr & attr, wxDC & dc, int row,
                        int col ) override;

private:
    std::vector<BITMAPS> m_icons;
    wxArrayString        m_names;

    // For single-icon mode
    wxBitmapBundle m_icon;
    wxSize m_iconSize;
};

//---- Grid helpers: custom wxGridCellRenderer that renders just an icon ----------------
//
// Note: use with read only cells

class KICOMMON_API GRID_CELL_ICON_RENDERER : public wxGridCellRenderer
{
public:
    GRID_CELL_ICON_RENDERER( const wxBitmapBundle& aIcon );

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
               const wxRect& aRect, int aRow, int aCol, bool isSelected ) override;
    wxSize GetBestSize( wxGrid & grid, wxGridCellAttr & attr, wxDC & dc, int row,
                        int col ) override;
    wxGridCellRenderer* Clone() const override;

private:
    wxBitmapBundle m_icon;
};

//---- Grid helpers: custom wxGridCellRenderer that renders just an icon from wxArtprovider -
//
// Note: use with read only cells

class KICOMMON_API GRID_CELL_STATUS_ICON_RENDERER : public wxGridCellRenderer
{
public:
    GRID_CELL_STATUS_ICON_RENDERER( int aStatus );

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
               const wxRect& aRect, int aRow, int aCol, bool isSelected ) override;
    wxSize GetBestSize( wxGrid & grid, wxGridCellAttr & attr, wxDC & dc, int row,
                        int col ) override;
    wxGridCellRenderer* Clone() const override;

private:
    int            m_status;
    wxBitmapBundle m_bitmap;
};



//---- Grid helpers: custom wxGridCellEditor ------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor

class KICOMMON_API GRID_CELL_ICON_TEXT_POPUP : public wxGridCellEditor
{
public:
    GRID_CELL_ICON_TEXT_POPUP( const std::vector<BITMAPS>& icons, const wxArrayString& names );

    wxGridCellEditor* Clone() const override;
    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

    wxString GetValue() const override;

    void SetSize( const wxRect& aRect ) override;

    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int , int , const wxGrid* , const wxString& , wxString *aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;

protected:
    wxBitmapComboBox* Combo() const { return static_cast<wxBitmapComboBox*>( m_control ); }

    std::vector<BITMAPS> m_icons;
    wxArrayString        m_names;
    wxString             m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_ICON_TEXT_POPUP );
};


class GRID_CELL_NULLABLE_INTERFACE
{
public:
    GRID_CELL_NULLABLE_INTERFACE() :
            m_isNullable( true )
    {
    }
    GRID_CELL_NULLABLE_INTERFACE( bool aIsNullable ) :
            m_isNullable( aIsNullable )
    {
    }
    virtual ~GRID_CELL_NULLABLE_INTERFACE() = default;

    virtual bool IsNullable() const { return m_isNullable; }

protected:
    bool m_isNullable{ false };
};


//---- Grid helpers: custom wxGridCellTextEditor ------------------------------------------
//
// Note: This is used to mark WX_GRID cell as nullable
class GRID_CELL_MARK_AS_NULLABLE : public wxGridCellTextEditor, public GRID_CELL_NULLABLE_INTERFACE
{
public:
    GRID_CELL_MARK_AS_NULLABLE() :
            GRID_CELL_NULLABLE_INTERFACE( true )
    {
    }
    GRID_CELL_MARK_AS_NULLABLE( const bool aIsNullable ) :
            GRID_CELL_NULLABLE_INTERFACE( aIsNullable )
    {
    }

    wxGridCellEditor* Clone() const override { return new GRID_CELL_MARK_AS_NULLABLE( IsNullable() ); }

    void Reset() override {}


    wxDECLARE_NO_COPY_CLASS( GRID_CELL_MARK_AS_NULLABLE );
};


#endif  // GRID_ICON_TEXT_HELPERS_H
