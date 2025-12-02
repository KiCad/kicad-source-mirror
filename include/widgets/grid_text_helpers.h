/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <functional>
#include <memory>
#include <wx/generic/gridctrl.h>
#include <wx/combo.h>

class wxGrid;
class wxStyledTextCtrl;
class wxStyledTextEvent;
class SCINTILLA_TRICKS;


/**
 * This class works around a bug in wxGrid where the first keystroke doesn't get sent through
 * the validator if the editor wasn't already open.
 */
class KICOMMON_API GRID_CELL_TEXT_EDITOR : public wxGridCellTextEditor
{
public:
    GRID_CELL_TEXT_EDITOR();

    void SetSize( const wxRect& aRect ) override;

    virtual void SetValidator( const wxValidator& validator ) override;
    virtual void StartingKey( wxKeyEvent& event ) override;

protected:
    std::unique_ptr<wxValidator> m_validator;
};


/**
 * A general-purpose text renderer for WX_GRIDs backed by WX_GRID_TABLE_BASE tables that can handle
 * drawing expand/collapse controls.
 */
class KICOMMON_API GRID_CELL_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_TEXT_RENDERER();

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool isSelected ) override;

    wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col ) override;
};


/**
 * A text renderer that can unescape text for display
 * This is useful where it's desired to keep the underlying storage escaped.
 */
class KICOMMON_API GRID_CELL_ESCAPED_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_ESCAPED_TEXT_RENDERER();

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool isSelected ) override;

    wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col ) override;
};


class KICOMMON_API GRID_CELL_STC_EDITOR : public wxGridCellEditor
{
public:
    GRID_CELL_STC_EDITOR( bool aIgnoreCase, bool aSingleLine,
                          std::function<void( wxStyledTextEvent&, SCINTILLA_TRICKS* )> onCharFn );

    void SetSize( const wxRect& aRect ) override;
    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_STC_EDITOR( m_ignoreCase, m_singleLine, m_onCharFn );
    }

    wxString GetValue() const override;

    void StartingKey( wxKeyEvent& event ) override;
    void Show( bool aShow, wxGridCellAttr *aAttr = nullptr ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int aRow, int aCol, const wxGrid*, const wxString&, wxString* aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override {}

protected:
    void onFocusLoss( wxFocusEvent& aEvent );

    wxStyledTextCtrl* stc_ctrl() const;

protected:
    SCINTILLA_TRICKS* m_scintillaTricks;
    bool              m_ignoreCase;
    bool              m_singleLine;
    wxString          m_value;

    std::function<void( wxStyledTextEvent&, SCINTILLA_TRICKS* )> m_onCharFn;
};


class KICOMMON_API GRID_CELL_TEXT_BUTTON : public wxGridCellEditor
{
public:
    GRID_CELL_TEXT_BUTTON() {};

    wxString GetValue() const override;

    void SetSize( const wxRect& aRect ) override;

    void StartingKey( wxKeyEvent& event ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int , int , const wxGrid* , const wxString& , wxString *aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;

#if wxUSE_VALIDATORS
    void SetValidator( const wxValidator& validator );
#endif

protected:
    wxComboCtrl* Combo() const { return static_cast<wxComboCtrl*>( m_control ); }

#if wxUSE_VALIDATORS
    std::unique_ptr< wxValidator > m_validator;
#endif

    wxString     m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_TEXT_BUTTON );
};


