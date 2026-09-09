/*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*/

#pragma once

#include <vector>
#include <map>
#include <set>

#include <board.h>
#include <board_item.h>
#include <dialog_find_by_properties_base.h>

class PCB_EDIT_FRAME;
class PROPERTY_BASE;
class SCINTILLA_TRICKS;

enum class PROPERTY_MATCH_MODE
{
    IGNORED,
    MATCHING,
    DIFFERENT
};

struct PROPERTY_ROW_DATA
{
    wxString            propertyName;
    wxString            displayValue;
    wxVariant           rawValue;
    PROPERTY_BASE*      property;
    PROPERTY_MATCH_MODE matchMode;
    bool                isMixed;
};

class DIALOG_FIND_BY_PROPERTIES : public DIALOG_FIND_BY_PROPERTIES_BASE
{
public:
    DIALOG_FIND_BY_PROPERTIES( PCB_EDIT_FRAME* aParent );
    ~DIALOG_FIND_BY_PROPERTIES() override;

    bool Show( bool show = true ) override;

    void OnSelectionChanged();

protected:
    void OnBoardChanged( wxCommandEvent& event );

    // Event handlers from base
    void onNotebookPageChanged( wxNotebookEvent& event ) override;
    void onSelectMatchingClick( wxCommandEvent& event ) override;
    void onCreateQueryClick( wxCommandEvent& event ) override;
    void onCheckSyntaxClick( wxCommandEvent& event ) override;
    void onRecentQuerySelected( wxCommandEvent& event ) override;
    void OnCloseButtonClick( wxCommandEvent& event ) override;

private:
    void                     rebuildPropertyGrid();
    void                     selectMatchingFromProperties();
    void                     selectMatchingFromQuery();
    void                     applyMatchResults( EDA_ITEMS& aMatchList, wxStaticText* aStatusLabel );
    wxString                 generateExpressionFromProperties( wxArrayString* aSkippedRows = nullptr );
    std::vector<BOARD_ITEM*> collectAllBoardItems();
    bool                     itemMatchesPropertyCriteria( BOARD_ITEM* aItem );
    void                     saveRecentQuery( const wxString& aQuery );
    void                     loadRecentQueries();
    void                     onGridCellClick( wxGridEvent& aEvent );
    void                     onGridCellChanged( wxGridEvent& aEvent );
    void                     onGridSizeChanged( wxSizeEvent& aEvent );
    void                     onScintillaCharAdded( wxStyledTextEvent& aEvent );
    void                     updateMatchModeCell( int aRow );

    static wxString  propNameToExprField( const wxString& aPropName );
    static wxVariant anyToVariant( const wxAny& aValue );
    wxString         formatValueForExpression( PROPERTY_BASE* aProp, const wxVariant& aValue );
    wxVariant        getVariantAwareValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty );

    PCB_EDIT_FRAME*                m_frame;
    BOARD*                         m_board;
    SCINTILLA_TRICKS*              m_scintillaTricks;
    std::vector<PROPERTY_ROW_DATA> m_propertyRows;
    std::set<size_t>               m_selectedTypes;
};
