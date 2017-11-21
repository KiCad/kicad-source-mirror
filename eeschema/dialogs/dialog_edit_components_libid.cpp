/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eeschema/dialogs/dialog_edit_components_libid.cpp
 * @brief Dialog to remap library id of components to an other library id
 */


#include <fctsys.h>
#include <dialog_edit_components_libid_base.h>
#include <schframe.h>
#include <class_drawpanel.h>
#include <sch_component.h>
#include <sch_reference_list.h>

#define COL_REFS 0
#define COL_CURR_LIBID 1
#define COL_NEW_LIBID 2

// a helper class to handle components to edit
class CMP_CANDIDATE
{
public:
    SCH_COMPONENT* m_Component; // the schematic component
    int         m_Row;          // the row index in m_grid
    SCH_SCREEN* m_Screen;       // the screen where m_Component lives
    wxString    m_Reference;    // the schematic reference, only to display it in list
    wxString    m_InitialLibId; // the Lib Id of the component before any change
    bool        m_IsOrphan;     // true if a component has no corresponding symbol found in libs

    CMP_CANDIDATE( SCH_COMPONENT* aComponent )
    {
        m_Component = aComponent;
        m_InitialLibId = m_Component->GetLibId().Format();
        m_Row = -1;
        m_IsOrphan = false;
    }

    // Returns a string like mylib:symbol_name from the LIB_ID of the component
    wxString GetStringLibId()
    {
        return wxString( m_Component->GetLibId().Format().c_str() );
    }

    // Returns a string containing the reference of the component
    wxString GetSchematicReference()
    {
        return m_Reference;
    }
};


/**
 * DIALOG_EDIT_COMPONENTS_LIBID is a dialog to globally edit the LIB_ID of groups if components
 * having the same initial LIB_ID.
 * this is useful when you want:
 *  to move a symbol from a symbol library to an other symbol library
 *  to change the nickname of a library
 *  globally replace the symbol used by a group of components by an other symbol.
 */
class DIALOG_EDIT_COMPONENTS_LIBID : public DIALOG_EDIT_COMPONENTS_LIBID_BASE
{
public:
    DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent );

    bool IsSchelaticModified() { return m_isModified; }

private:
    SCH_EDIT_FRAME* m_parent;
    bool m_isModified;      // set to true is the schemaic is modified

    std::vector<CMP_CANDIDATE> m_components;

    void initDlg();

    // returns true if all new lib id are valid
    bool validateLibIds();

    // Reverts all changes already made
    void revertChanges();

    // Events handlers
    // called on a right click or a left double click:
	void onCellBrowseLib( wxGridEvent& event ) override;
	void onApplyButton( wxCommandEvent& event ) override;

	void onCancel( wxCommandEvent& event ) override
    {
        if( m_isModified )
            revertChanges();
        event.Skip();
    }

	void onUndoChangesButton( wxCommandEvent& event ) override;

    bool TransferDataFromWindow() override;
};


DIALOG_EDIT_COMPONENTS_LIBID::DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent )
    :DIALOG_EDIT_COMPONENTS_LIBID_BASE( aParent )
{
    m_parent = aParent;
    initDlg();

    // Gives a min size to display m_grid, now it is populated:
    int minwidth = 30   // a margin
                   + m_grid->GetRowLabelSize() + m_grid->GetColSize( COL_REFS )
                   + m_grid->GetColSize( COL_CURR_LIBID ) + m_grid->GetColSize( COL_NEW_LIBID );
    m_panelGrid->SetMinSize( wxSize( minwidth, -1) );
    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


// A sort compare function to sort components list by LIB_ID
// inside the group of same LIB_ID, sort by reference
static bool sort_by_libid( const CMP_CANDIDATE& cmp1, const CMP_CANDIDATE& cmp2 )
{
    if( cmp1.m_Component->GetLibId() == cmp2.m_Component->GetLibId() )
        return cmp1.m_Reference.Cmp( cmp2.m_Reference ) < 0;

    return cmp1.m_Component->GetLibId() < cmp2.m_Component->GetLibId();
}


void DIALOG_EDIT_COMPONENTS_LIBID::initDlg()
{
    // Build the component list:
#if 0
    // This option build a component list that works fine to edit LIB_ID fields, but does not display
    // all components in a complex hierarchy.
    // the list is shorter, but can be look like there are missing components in list
    SCH_SCREENS screens;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() == SCH_COMPONENT_T )
            {
                CMP_CANDIDATE candidate( static_cast< SCH_COMPONENT* >( item ) );
                candidate.m_Screen = screen;
                candidate.m_Reference = candidate.m_Component->GetField( REFERENCE )->GetFullyQualifiedText();
                m_components.push_back( candidate );
            }
        }
    }
#else
    // This option build the full component list
    // In complex hierarchies, the same component is in fact duplicated, but
    // it is listed with different references (one by sheet instance)
    // the list is larger and looks like it contains all components
    SCH_SHEET_LIST sheets( g_RootSheet );
    SCH_REFERENCE_LIST references;
    // build the full list of components including component having no symbol in loaded libs
    // (orphan components)
    sheets.GetComponents( references, /* include power symbols */true,
                          /* include orphan components */true );

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& item = references[ii];
        CMP_CANDIDATE candidate( item.GetComp() );
        candidate.m_Screen = item.GetSheetPath().LastScreen();
        SCH_SHEET_PATH sheetpath = item.GetSheetPath();
        candidate.m_Reference = candidate.m_Component->GetRef( &sheetpath );
        // For multi units per package , add unit id.
        // however, there is a problem: the unit id stored is always >= 1
        // and 1 for no multi units.
        // so add unit id only if unit > 1 if the unit count is > 1
        // (can be 0 if the symbol is not found)
        int unit = candidate.m_Component->GetUnitSelection( &sheetpath );
        int unitcount = candidate.m_Component->GetUnitCount();
        candidate.m_IsOrphan = unitcount == 0;

        if( unitcount > 1 || unit > 1 )
        {
            candidate.m_Reference << wxChar( ('A' + unit -1) );
        }

        m_components.push_back( candidate );
    }
#endif

    if( m_components.size() == 0 )
        return;

    // now sort by lib id to create groups of items having the same lib id
    std::sort( m_components.begin(), m_components.end(), sort_by_libid );

    // Now, fill m_grid
    wxString last_str_libid = m_components.front().GetStringLibId();
    int row = 0;
    wxString refs;
    bool mark_cell = false;

    for( unsigned ii = 0; ii < m_components.size(); ii++ )
    {
        CMP_CANDIDATE& cmp = m_components[ii];

        wxString str_libid = cmp.GetStringLibId();

        if( last_str_libid != str_libid || ii == m_components.size()-1 )
        {

            if( ii == m_components.size()-1 )
            {
                if( !refs.IsEmpty() )
                        refs += " ";

                refs += cmp.GetSchematicReference();
                cmp.m_Row = row;
                mark_cell = cmp.m_IsOrphan;

                last_str_libid = str_libid;
            }

            if( m_grid->GetNumberRows() <= row )
                m_grid->AppendRows();

            m_grid->SetCellValue( row, COL_REFS, refs );
            m_grid->SetReadOnly( row, COL_REFS );

            m_grid->SetCellValue( row, COL_CURR_LIBID, last_str_libid );
            m_grid->SetReadOnly( row, COL_CURR_LIBID );

            if( mark_cell ) // A symbol is not existing in libraries: mark the cell
            {
                wxFont font = m_grid->GetDefaultCellFont();
                font.MakeBold();
                font.MakeItalic();
                m_grid->SetCellFont( row, COL_CURR_LIBID, font );
            }

            m_grid->SetCellRenderer( row, COL_REFS, new wxGridCellAutoWrapStringRenderer);
            m_grid->AutoSizeRow( row, false );

            // prepare next entry
            mark_cell = cmp.m_IsOrphan;
            last_str_libid = str_libid;
            refs.Empty();
            row++;
        }

        if( !refs.IsEmpty() )
            refs += " ";

        refs += cmp.GetSchematicReference();
        cmp.m_Row = row;
    }

    m_grid->AutoSizeColumn( COL_CURR_LIBID );

    // Gives a similar width to COL_NEW_LIBID because it can conatains similar strings
    if( m_grid->GetColSize( COL_CURR_LIBID ) > m_grid->GetColSize( COL_NEW_LIBID ) )
        m_grid->SetColSize( COL_NEW_LIBID, m_grid->GetColSize( COL_CURR_LIBID ) );
}


bool DIALOG_EDIT_COMPONENTS_LIBID::validateLibIds()
{
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = m_grid->GetCellValue( row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() )
            continue;

        // a new lib id is found. validate this new value
        LIB_ID id;
        id.Parse( new_libid );

        if( !id.IsValid() )
        {
            wxString msg;
            msg.Printf( _( "Symbol library identifier '%s' is not valid at row %d!" ), new_libid, row+1 );
            wxMessageBox( msg );
            return false;
        }
    }

    return true;
}


void DIALOG_EDIT_COMPONENTS_LIBID::onApplyButton( wxCommandEvent& event )
{
    if( TransferDataFromWindow() )
        m_parent->GetCanvas()->Refresh();
}


void DIALOG_EDIT_COMPONENTS_LIBID::onUndoChangesButton( wxCommandEvent& event )
{
    revertChanges();

    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        m_grid->SetCellValue( row, COL_NEW_LIBID, wxEmptyString );
    }

    m_isModified = false;
}


void DIALOG_EDIT_COMPONENTS_LIBID::onCellBrowseLib( wxGridEvent& event )
{
    int row = event.GetRow();

    SCH_BASE_FRAME::HISTORY_LIST dummy;

    auto sel = m_parent->SelectComponentFromLibrary( NULL, dummy, true, 0, 0 );

    if( !sel.LibId.IsValid() )
        return;

    wxString new_libid;
    new_libid = sel.LibId.Format();

    m_grid->SetCellValue( row, COL_NEW_LIBID, new_libid );

}


bool DIALOG_EDIT_COMPONENTS_LIBID::TransferDataFromWindow()
{
    if( !validateLibIds() )
        return false;

    bool change = false;
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = m_grid->GetCellValue( row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() || new_libid == m_grid->GetCellValue( row, COL_CURR_LIBID ) )
            continue;

        // a new lib id is found and was already validated.
        // set this new value
        LIB_ID id;
        id.Parse( new_libid );

        for( CMP_CANDIDATE& cmp : m_components )
        {
            if( cmp.m_Row != row )
                continue;

            cmp.m_Component->SetLibId( id );
            change = true;
            cmp.m_Screen->SetModify();
        }
    }

    if( change )
    {
        m_isModified = true;
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks( true );
    }

    return true;
}


void DIALOG_EDIT_COMPONENTS_LIBID::revertChanges()
{
    bool change = false;
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        for( CMP_CANDIDATE& cmp : m_components )
        {
            if( cmp.m_Row != row )
                continue;

            LIB_ID id;
            id.Parse( cmp.m_InitialLibId );

            if( cmp.m_Component->GetLibId() != id )
            {
                cmp.m_Component->SetLibId( id );
                change = true;
            }
        }
    }

    if( change )
    {
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks( true );
        m_parent->GetCanvas()->Refresh();
    }
}


bool InvokeDialogEditComponentsLibId( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_EDIT_COMPONENTS_LIBID dlg( aCaller );
    dlg.ShowModal();

    return dlg.IsSchelaticModified();
}