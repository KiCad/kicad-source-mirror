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

#include <dialog_design_rules_base.h>


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

    static const wxString wildCard;     ///< the name of a fictitious netclass which includes all NETs

    PCB_EDIT_FRAME*         m_Parent;
    BOARD*                  m_Pcb;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    static int              s_LastTabSelection;     ///< which tab user had open last

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
    void OnNetClassesNameLeftClick( wxGridEvent& event ){ event.Skip(); }
    void OnNetClassesNameRightClick( wxGridEvent& event ){ event.Skip(); }
    void OnAddNetclassClick( wxCommandEvent& event );
    void OnRemoveNetclassClick( wxCommandEvent& event );

    /*
     * Called on "Move Up" button click
     * the selected(s) rules are moved up
     * The default netclass is always the first rule
     */
    void OnMoveUpSelectedNetClass( wxCommandEvent& event );

    /*
     * Called on the left Choice Box selection
     */
    void OnLeftCBSelection( wxCommandEvent& event );

    /*
     * Called on the Right Choice Box selection
     */
    void OnRightCBSelection( wxCommandEvent& event );

    void OnRightToLeftCopyButton( wxCommandEvent& event );
    void OnLeftToRightCopyButton( wxCommandEvent& event );

    void OnNotebookPageChanged( wxNotebookEvent& event );

    /*
     * Called on clicking the left "select all" button:
     * select all items of the left netname list list box
     */
    void OnLeftSelectAllButton( wxCommandEvent& event );

    /*
     * Called on clicking the right "select all" button:
     * select all items of the right netname list list box
     */
    void OnRightSelectAllButton( wxCommandEvent& event );

    /*
     * Function TestDataValidity
     *
     * Performs a check of design rule data validity and displays an error message if errors
     * are found.
     * @param aErrorMsg is a pointer to a wxString to copy the error message into.  Can be NULL.
     * @return true if Ok, false if error
     */
    bool TestDataValidity( wxString* aErrorMsg = NULL );

    void InitDialogRules();
    void InitGlobalRules();

    /**
     * Function InitRulesList
     * Fill the grid showing current rules with values
     */
    void InitRulesList();

    /* Populates the lists of sizes (Tracks width list and Vias diameters & drill list) */
    void InitDimensionsLists();

    void InitializeRulesSelectionBoxes();

    /* Copy the rules list from grid to board
     */
    void CopyRulesListToBoard();

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


public:
    DIALOG_DESIGN_RULES( PCB_EDIT_FRAME* parent );
    ~DIALOG_DESIGN_RULES( ) { }

    virtual bool TransferDataFromWindow();
};

#endif //__dialog_design_rules_h_
