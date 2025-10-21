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

#ifndef FIELDS_GRID_TABLE_H
#define FIELDS_GRID_TABLE_H

#include <wx/grid.h>
#include <sch_symbol.h>
#include <grid_tricks.h>
#include <validators.h>
#include <vector>

class SCH_BASE_FRAME;
class DIALOG_SHIM;
class EMBEDDED_FILES;
class SCH_LABEL_BASE;


class FIELDS_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_GRID_TRICKS( WX_GRID* aGrid, DIALOG_SHIM* aDialog,
                        std::vector<EMBEDDED_FILES*> aFilesStack,
                        std::function<void( wxCommandEvent& )> aAddHandler ) :
        GRID_TRICKS( aGrid, std::move( aAddHandler ) ),
        m_dlg( aDialog ),
        m_filesStack( aFilesStack )
    {}

protected:
    int getFieldRow( FIELD_T aFieldId );

    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    DIALOG_SHIM*                 m_dlg;
    std::vector<EMBEDDED_FILES*> m_filesStack;
};


enum FIELDS_DATA_COL_ORDER
{
    FDC_NAME = 0,
    FDC_VALUE,
    FDC_SHOWN,
    FDC_SHOW_NAME,
    FDC_H_ALIGN,
    FDC_V_ALIGN,
    FDC_ITALIC,
    FDC_BOLD,
    FDC_TEXT_SIZE,
    FDC_ORIENTATION,
    FDC_POSX,
    FDC_POSY,
    FDC_FONT,
    FDC_COLOR,
    FDC_ALLOW_AUTOPLACE,

    FDC_SCH_EDIT_COUNT,

    FDC_PRIVATE = FDC_SCH_EDIT_COUNT,

    FDC_SYMBOL_EDITOR_COUNT
};


class FIELDS_GRID_TABLE : public WX_GRID_TABLE_BASE, public std::vector<SCH_FIELD>
{
public:
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_BASE_FRAME* aFrame, WX_GRID* aGrid,
                       LIB_SYMBOL* aSymbol, std::vector<EMBEDDED_FILES*> aFilesStack = {} );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_SYMBOL* aSymbol );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_SHEET* aSheet );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_LABEL_BASE* aLabel );
    ~FIELDS_GRID_TABLE() override;

    int GetNumberRows() override { return getVisibleRowCount(); }
    int GetNumberCols() override { return getColumnCount(); }

    int GetMandatoryRowCount() const;

    wxString GetColLabelValue( int aCol ) override;

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    wxString GetValue( int aRow, int aCol ) override;
    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    wxString StringFromBool( bool aValue ) const;
    bool BoolFromString( const wxString& aValue ) const;

    SCH_FIELD* GetField( FIELD_T aFieldId );
    int GetFieldRow( FIELD_T aFieldId );

    void AddInheritedField( const SCH_FIELD& aParent );

    void SetFieldInherited( size_t aRow, const SCH_FIELD& aParent )
    {
        m_isInherited.resize( aRow + 1, false );
        m_parentFields.resize( aRow + 1 );
        m_parentFields[aRow] = aParent;
        m_isInherited[aRow] = true;
    }

    bool IsInherited( size_t aRow ) const
    {
        if( aRow >= m_isInherited.size() || aRow >= m_parentFields.size() )
            return false;

        return m_isInherited[aRow] && m_parentFields[aRow].GetText() == at( aRow ).GetText();
    }

    const SCH_FIELD& ParentField( size_t row ) const { return m_parentFields[row]; }

    void push_back( const SCH_FIELD& field );
    // For std::vector compatibility, but we don't use it directly.
    void emplace_back( const SCH_FIELD& field ) { push_back( field ); }

    bool EraseRow( size_t row );
    void SwapRows( size_t a, size_t b );

    void DetachFields();

protected:
    void initGrid( WX_GRID* aGrid );

    void onUnitsChanged( wxCommandEvent& aEvent );

    int getColumnCount() const;
    int getVisibleRowCount() const;

    SCH_FIELD& getField( int aRow );

private:
    SCH_BASE_FRAME*              m_frame;
    DIALOG_SHIM*                 m_dialog;
    KICAD_T                      m_parentType;
    LIB_SYMBOL*                  m_part;
    std::vector<EMBEDDED_FILES*> m_filesStack;
    wxString                     m_symbolNetlist;
    wxString                     m_curdir;

    FIELD_VALIDATOR   m_fieldNameValidator;
    FIELD_VALIDATOR   m_referenceValidator;
    FIELD_VALIDATOR   m_valueValidator;
    FIELD_VALIDATOR   m_urlValidator;
    FIELD_VALIDATOR   m_nonUrlValidator;
    FIELD_VALIDATOR   m_filepathValidator;

    wxGridCellAttr*   m_readOnlyAttr;
    wxGridCellAttr*   m_fieldNameAttr;
    wxGridCellAttr*   m_referenceAttr;
    wxGridCellAttr*   m_valueAttr;
    wxGridCellAttr*   m_footprintAttr;
    wxGridCellAttr*   m_urlAttr;
    wxGridCellAttr*   m_nonUrlAttr;
    wxGridCellAttr*   m_filepathAttr;
    wxGridCellAttr*   m_boolAttr;
    wxGridCellAttr*   m_vAlignAttr;
    wxGridCellAttr*   m_hAlignAttr;
    wxGridCellAttr*   m_orientationAttr;
    wxGridCellAttr*   m_netclassAttr;
    wxGridCellAttr*   m_fontAttr;
    wxGridCellAttr*   m_colorAttr;

    std::vector<bool>      m_isInherited;
    std::vector<SCH_FIELD> m_parentFields;

    std::unique_ptr<NUMERIC_EVALUATOR>        m_eval;
    std::map< std::pair<int, int>, wxString > m_evalOriginal;
};


#endif  // FIELDS_GRID_TABLE_H
