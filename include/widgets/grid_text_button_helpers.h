/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GRID_TEXT_BUTTON_HELPERS_H
#define GRID_TEXT_BUTTON_HELPERS_H

#include <wx/combo.h>
#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>


class wxGrid;
class WX_GRID;
class DIALOG_SHIM;


class GRID_CELL_TEXT_BUTTON : public wxGridCellEditor
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
    wxScopedPtr< wxValidator > m_validator;
#endif

    wxString     m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_TEXT_BUTTON );
};


class GRID_CELL_SYMBOL_ID_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_SYMBOL_ID_EDITOR( DIALOG_SHIM* aParent,
                                const wxString& aPreselect = wxEmptyString ) :
            m_dlg( aParent ),
            m_preselect( aPreselect )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_SYMBOL_ID_EDITOR( m_dlg, m_preselect );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
};


class GRID_CELL_FOOTPRINT_ID_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_FOOTPRINT_ID_EDITOR( DIALOG_SHIM* aParent,
                                   const wxString& aPreselect = wxEmptyString ) :
            m_dlg( aParent ),
            m_preselect( aPreselect )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_FOOTPRINT_ID_EDITOR( m_dlg );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
};


class GRID_CELL_URL_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_URL_EDITOR( DIALOG_SHIM* aParent ) :
            m_dlg( aParent )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_URL_EDITOR( m_dlg );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
};


/**
 *  Editor for wxGrid cells that adds a file/folder browser to the grid input field
 */
class GRID_CELL_PATH_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    /**
     * Constructor
     *
     * @param aCurrentDir is current directory the path editor will open at
     * @param aExt is the file extension(s) to filter by. If empty, the path editor will switch
     *             to folder mode instead of file.
     * @param aNormalize indicates whether to normalize the selected path (replace part of path
     *                   with variables or relative path)
     * @param aNormalizeBasePath is the path to use when trying to base variables (generally
     *                           current project path)
     */
    GRID_CELL_PATH_EDITOR( DIALOG_SHIM* aParentDialog, WX_GRID* aGrid, wxString* aCurrentDir,
                           const wxString& aExt, bool aNormalize = false,
                           const wxString& aNormalizeBasePath = wxEmptyString ) :
            m_dlg( aParentDialog ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_ext( aExt ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_PATH_EDITOR( m_dlg, m_grid, m_currentDir, m_ext );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
    WX_GRID*     m_grid;
    wxString*    m_currentDir;
    wxString     m_ext;
    bool         m_normalize;
    wxString     m_normalizeBasePath;
};


#endif  // GRID_TEXT_BUTTON_HELPERS_H
