/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef DIALOG_GENERATORS_H_
#define DIALOG_GENERATORS_H_

#include "dialog_generators_base.h"
#include <widgets/progress_reporter_base.h>
#include <board.h>

class PCB_EDIT_FRAME;

#define DIALOG_GENERATORS_WINDOW_NAME wxT( "DialogGeneratorsWindowName" )


class DIALOG_GENERATORS : public DIALOG_GENERATORS_BASE, public BOARD_LISTENER
{
public:
    DIALOG_GENERATORS( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent );
    ~DIALOG_GENERATORS();

    void RebuildModels();

private:
    void OnItemSelected( wxDataViewEvent& aEvent );

    void OnRebuildSelectedClick( wxCommandEvent& event ) override;
    void OnRebuildTypeClick( wxCommandEvent& event ) override;
    void OnRebuildAllClick( wxCommandEvent& event ) override;

    void OnCancelClick( wxCommandEvent& aEvent ) override;

    wxDataViewListStore* getCurrentModel();
    void                 clearModels();

    void clearModel( const wxString& aName );

    void deleteModel( const wxString& aName );

    wxDataViewCtrl* addPage( const wxString& aName, const wxString& aTitle );

    void onUnitsChanged( wxCommandEvent& event );
    void onBoardChanged( wxCommandEvent& event );

    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsRemoved( BOARD&                    aBoard,
                                      std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsChanged( BOARD&                    aBoard,
                                      std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                         std::vector<BOARD_ITEM*>& aRemovedItems,
                                         std::vector<BOARD_ITEM*>& aChangedItems ) override;

    std::map<wxString, std::vector<std::pair<wxString, wxString>>> m_columnNameTypes;
    std::map<wxString, wxDataViewListStore*>                       m_dataModels;
    std::map<wxString, wxDataViewCtrl*>                            m_dataViews;

    BOARD*          m_currentBoard;
    PCB_EDIT_FRAME* m_frame;
};


#endif // DIALOG_GENERATORS_H_