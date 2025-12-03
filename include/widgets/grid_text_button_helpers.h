/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
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

#ifndef GRID_TEXT_BUTTON_HELPERS_H
#define GRID_TEXT_BUTTON_HELPERS_H

#include <widgets/grid_icon_text_helpers.h>

#include <memory>

#include <wx/combo.h>
#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>
#include <search_stack.h>
#include <widgets/grid_text_helpers.h>
#include <kicommon.h>


class wxGrid;
class WX_GRID;
class DIALOG_SHIM;
class EMBEDDED_FILES;


class KICOMMON_API GRID_CELL_SYMBOL_ID_EDITOR : public GRID_CELL_TEXT_BUTTON
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


class KICOMMON_API GRID_CELL_FPID_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_FPID_EDITOR( DIALOG_SHIM* aParent, const wxString& aSymbolNetlist,
                           const wxString& aPreselect = wxEmptyString ) :
            m_dlg( aParent ),
            m_preselect( aPreselect ),
            m_symbolNetlist( aSymbolNetlist )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_FPID_EDITOR( m_dlg, m_symbolNetlist );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
    wxString     m_symbolNetlist;
};


class KICOMMON_API GRID_CELL_URL_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    GRID_CELL_URL_EDITOR( DIALOG_SHIM* aParent, SEARCH_STACK* aSearchStack = nullptr,
                          std::vector<EMBEDDED_FILES*> aFilesStack = {} ) :
            m_dlg( aParent ),
            m_searchStack( aSearchStack ),
            m_filesStack( aFilesStack )
    { }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_URL_EDITOR( m_dlg );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM*                 m_dlg;
    SEARCH_STACK*                m_searchStack;     // No ownership.
    std::vector<EMBEDDED_FILES*> m_filesStack;      // No ownership.
};


/**
 *  Editor for wxGrid cells that adds a file/folder browser to the grid input field
 */
class KICOMMON_API GRID_CELL_PATH_EDITOR : public GRID_CELL_TEXT_BUTTON
{
public:
    /**
     * Constructor
     *
     * @param aCurrentDir is current directory the path editor will open at
     * @param aNormalize indicates whether to normalize the selected path (replace part of path
     *                   with variables or relative path)
     * @param aNormalizeBasePath is the path to use when trying to base variables (generally
     *                           current project path)
     * @param aFileFilterFn a callback which provides a file extension(s) filter.
     */
    GRID_CELL_PATH_EDITOR( DIALOG_SHIM* aParentDialog, WX_GRID* aGrid, wxString* aCurrentDir,
                           bool aNormalize, const wxString& aNormalizeBasePath,
                           std::function<wxString( WX_GRID* grid, int row )> aFileFilterFn,
                           std::function<wxString( const wxString& )> aEmbedCallback = nullptr ) :
            m_dlg( aParentDialog ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath ),
            m_fileFilterFn( std::move( aFileFilterFn ) ),
            m_embedCallback( std::move( aEmbedCallback ) )
    { }

    /**
     * Constructor
     *
     * @param aCurrentDir is current directory the path editor will open at
     * @param aFileFilter is the file extension(s) to filter by. If empty, the path editor will
     *                    switch to folder mode instead of file.
     * @param aNormalize indicates whether to normalize the selected path (replace part of path
     *                   with variables or relative path)
     * @param aNormalizeBasePath is the path to use when trying to base variables (generally
     *                           current project path)
     */
    GRID_CELL_PATH_EDITOR( DIALOG_SHIM* aParentDialog, WX_GRID* aGrid, wxString* aCurrentDir,
                           const wxString& aFileFilter, bool aNormalize = false,
                           const wxString& aNormalizeBasePath = wxEmptyString,
                           std::function<wxString( const wxString& )> aEmbedCallback = nullptr ) :
            m_dlg( aParentDialog ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath ),
            m_fileFilter( aFileFilter ),
            m_embedCallback( std::move( aEmbedCallback ) )
    { }

    wxGridCellEditor* Clone() const override
    {
        if( m_fileFilterFn )
        {
            return new GRID_CELL_PATH_EDITOR( m_dlg, m_grid, m_currentDir, m_normalize,
                                              m_normalizeBasePath, m_fileFilterFn, m_embedCallback );
        }
        else
        {
            return new GRID_CELL_PATH_EDITOR( m_dlg, m_grid, m_currentDir, m_fileFilter,
                                              m_normalize, m_normalizeBasePath, m_embedCallback );
        }
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

protected:
    DIALOG_SHIM* m_dlg;
    WX_GRID*     m_grid;
    wxString*    m_currentDir;
    bool         m_normalize;
    wxString     m_normalizeBasePath;

    wxString                                            m_fileFilter;
    std::function<wxString( WX_GRID* aGrid, int aRow )> m_fileFilterFn;
    std::function<wxString( const wxString& )>          m_embedCallback;
};


/**
* A cell editor which runs a provided function when the grid cell button is clicked.
*
* The function has the signature (int, int) -> void. The passed parameters are the (row, col) of the
* clicked grid cell
*/
class KICOMMON_API GRID_CELL_RUN_FUNCTION_EDITOR : public GRID_CELL_TEXT_BUTTON, public GRID_CELL_NULLABLE_INTERFACE
{
public:
    GRID_CELL_RUN_FUNCTION_EDITOR( DIALOG_SHIM* aParent, const std::function<void( int, int )> aFunction,
                                   const bool aIsNullable = false ) :
            GRID_CELL_NULLABLE_INTERFACE( aIsNullable ),
            m_dlg( aParent ),
            m_row( -1 ),
            m_col( -1 ),
            m_function( aFunction )
    {
    }

    wxGridCellEditor* Clone() const override
    {
        return new GRID_CELL_RUN_FUNCTION_EDITOR( m_dlg, m_function, IsNullable() );
    }

    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;


protected:
    DIALOG_SHIM* m_dlg;
    int          m_row;
    int          m_col;
    std::function<void( int, int )> m_function;
};

#endif  // GRID_TEXT_BUTTON_HELPERS_H
