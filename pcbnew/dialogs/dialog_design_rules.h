/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2015 KiCad Developers, see change_log.txt for contributors.
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
 * @file dialog_design_rules.h
 */

#ifndef __dialog_design_rules_h_
#define __dialog_design_rules_h_

#include <../class_board.h>
#include <widgets/unit_binder.h>

#include <dialog_design_rules_base.h>

#include <float.h>
#include <wx/valnum.h>


class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


// helper struct to handle a net and its netclass in dialog design rule editor
struct NETCUP
{
    NETCUP( const wxString& aNet, const wxString& aClass )
    {
        net = aNet;
        clazz = aClass;
    }

    wxString    net;            ///< a net name
    wxString    clazz;          ///< a class name
};


typedef std::vector<NETCUP>     NETCUPS;
typedef std::vector<NETCUP*>    PNETCUPS;

class DIALOG_DESIGN_RULES : public DIALOG_DESIGN_RULES_BASE
{

private:

    static const wxString   wildCard;               // The name of a fictitious netclass
                                                    // which includes all NETs
    static int              s_LastTabSelection;     // Which tab user had open last

    PCB_EDIT_FRAME*         m_Parent;
    BOARD*                  m_Pcb;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    int*                    m_originalColWidths;

    wxString                m_gridErrorMsg;
    wxGrid*                 m_gridErrorGrid;
    int                     m_gridErrorRow;
    int                     m_gridErrorCol;

    bool                    m_netclassesDirty;      // Indicates the netclass drop-down
                                                    // menus need rebuilding
    UNIT_BINDER             m_trackMinWidth;
    UNIT_BINDER             m_viaMinDiameter;
    UNIT_BINDER             m_viaMinDrill;
    UNIT_BINDER             m_microViaMinDiameter;
    UNIT_BINDER             m_microViaMinDrill;

    wxFloatingPointValidator< double > m_validator; // Floating point validator

    /**
     * A two column table which gets filled once and never loses any elements, so it is
     * basically constant, except that the NETCUP::clazz member can change for any
     * given row a NET is moved in and out of a class.  class reflects the respective
     * NET's current net class.
     */
    NETCUPS                 m_AllNets;

    // List of values to "customize" some tracks and vias
    std::vector <VIA_DIMENSION> m_ViasDimensionsList;
    std::vector <int> m_TracksWidthList;

private:
    void OnNetClassesNameLeftClick( wxGridEvent& event ) override { event.Skip(); }
    void OnNetClassesNameRightClick( wxGridEvent& event ) override { event.Skip(); }
    void OnAddNetclassClick( wxCommandEvent& event ) override;
    void OnRemoveNetclassClick( wxCommandEvent& event ) override;
    void CheckAllowMicroVias();
    void OnAllowMicroVias( wxCommandEvent& event ) override;
    void OnSizeNetclassGrid( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent &event ) override;
    void OnNetclassGridCellChanging( wxGridEvent& event );
    void OnMoveUpSelectedNetClass( wxCommandEvent& event ) override;
    void OnMoveDownSelectedNetClass( wxCommandEvent& event ) override;
    void OnLeftCBSelection( wxCommandEvent& event ) override;
    void OnRightCBSelection( wxCommandEvent& event ) override;
    void OnRightToLeftCopyButton( wxCommandEvent& event ) override;
    void OnLeftToRightCopyButton( wxCommandEvent& event ) override;
    void OnNotebookPageChanged( wxNotebookEvent& event ) override;
    void OnLeftSelectAllButton( wxCommandEvent& event ) override;
    void OnRightSelectAllButton( wxCommandEvent& event ) override;

    bool validateNetclassName( int aRow, wxString aName, bool focusFirst = true );
    bool validateData();

    void transferNetclassesToWindow();
    void transferGlobalRulesToWindow();

    void rebuildNetclassDropdowns();

    /* Populates the lists of sizes (Tracks width list and Vias diameters & drill list) */
    void InitDimensionsLists();

    void CopyNetclassesToBoard();
    void CopyGlobalRulesToBoard();
    void CopyDimensionsListsToBoard( );
    void SetRoutableLayerStatus();

    /**
     * Function FillListBoxWithNetNames
     * populates aListCtrl with net names and class names from m_AllNets in a two column display.
     */
    void FillListBoxWithNetNames( NETS_LIST_CTRL* aListCtrl, const wxString& aNetClass );

    /**
     * Function swapNetClass
     * replaces one net class name with another in the master list, m_AllNets.
     */
    void swapNetClass( const wxString& oldClass, const wxString& newClass )
    {
        for( NETCUPS::iterator i = m_AllNets.begin(); i!=m_AllNets.end();  ++i )
        {
            if( i->clazz == oldClass )
                i->clazz = newClass;
        }
    }

    void makePointers( PNETCUPS* aList, const wxString& aNetClassName );

    void setNetClass( const wxString& aNetName, const wxString& aClassName );

    void moveSelectedItems( NETS_LIST_CTRL* src, const wxString& newClassName );

    void setGridError( wxGrid* aGrid, const wxString& aMsg, int aRow, int aCol );

    void AdjustNetclassGridColumns( int aWidth );

public:
    DIALOG_DESIGN_RULES( PCB_EDIT_FRAME* parent );
    ~DIALOG_DESIGN_RULES( );

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;
};

#endif //__dialog_design_rules_h_
