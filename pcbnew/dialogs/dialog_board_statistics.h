/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
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


#ifndef _DIALOG_BOARD_STATISTICS_H
#define _DIALOG_BOARD_STATISTICS_H


#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <dialog_board_statistics_base.h>
#include <board_statistics.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <wx/datetime.h>

/**
 * Dialog to show common board info.
 */
class DIALOG_BOARD_STATISTICS : public DIALOG_BOARD_STATISTICS_BASE
{
public:
    /**
     * Type information, which will be shown in dialog.
     */
    template <typename T>
    struct INFO_LINE_ITEM
    {
        INFO_LINE_ITEM( T aAttribute, const wxString& aTitle ) :
                m_Attribute( aAttribute ),
                m_Title( aTitle ),
                m_Qty( 0 )
        {
        }

        T          m_Attribute;
        wxString   m_Title;
        int        m_Qty;
    };

    /**
     * Footprint attributes (such as SMD, THT, Virtual and so on), which will be shown in the
     * dialog. Holds both front and back footprint quantities.
     */
    struct FP_LINE_ITEM
    {
        FP_LINE_ITEM( int aAttributeMask, int aAttributeValue, wxString aTitle ) :
                m_Attribute_mask( aAttributeMask ),
                m_Attribute_value( aAttributeValue ),
                m_Title( aTitle ),
                m_FrontSideQty( 0 ),
                m_BackSideQty( 0 )
        {
        }

        int      m_Attribute_mask;
        int      m_Attribute_value;
        wxString m_Title;
        int      m_FrontSideQty;
        int      m_BackSideQty;
    };


    DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_STATISTICS();

    ///< Get data from the PCB board and print it to dialog
    bool TransferDataToWindow() override;

private:

    ///< Function to fill up all items types to be shown in the dialog.
    void refreshItemsTypes();

    ///< Get data from board.
    void getDataFromPCB();

    ///< Apply data to dialog widgets.
    void updateWidgets();

    ///< Update drills grid.
    void updateDrillGrid();

    ///< Print grid to string in tabular format.
    void printGridToStringAsTable( wxGrid* aGrid, wxString& aStr, bool aUseColLabels,
                                   bool aUseFirstColAsLabel );

    void adjustDrillGridColumns();

    void checkboxClicked( wxCommandEvent& aEvent ) override;

    ///< Save board statistics to a file
    void saveReportClicked( wxCommandEvent& aEvent ) override;

    void drillGridSize( wxSizeEvent& aEvent ) override;

    void drillGridSort( wxGridEvent& aEvent );

    PCB_EDIT_FRAME* m_frame;

    int             m_boardWidth;
    int             m_boardHeight;
    double          m_boardArea;
    double          m_frontCopperArea;
    double          m_backCopperArea;
    int             m_minClearanceTrackToTrack;
    int             m_minTrackWidth;
    int             m_minDrillSize;
    int             m_boardThickness;

    bool            m_hasOutline;          ///< Show if board outline properly defined.

    std::deque<FP_LINE_ITEM>                m_fpTypes;
    std::deque<INFO_LINE_ITEM<PAD_ATTRIB>>  m_padTypes;
    std::deque<INFO_LINE_ITEM<PAD_PROP>>    m_padFabProps;
    std::deque<INFO_LINE_ITEM<VIATYPE>>     m_viaTypes;
    std::deque<DRILL_LINE_ITEM>             m_drillTypes;

    int m_startLayerColInitialSize;        ///< Width of the start layer column as calculated by
                                           ///<    the wxWidgets autosizing algorithm.
    int m_stopLayerColInitialSize;         ///< Width of the stop layer column.
};

#endif // __DIALOG_BOARD_STATISTICS_H
