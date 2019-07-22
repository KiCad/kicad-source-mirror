/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <base_units.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_track.h>
#include <confirm.h>
#include <dialog_board_statistics_base.h>
#include <pad_shapes.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <wx/datetime.h>

/**
 * Class DIALOG_BOARD_STATISTIC
 *
 * Dialog to show common board info.
 */
class DIALOG_BOARD_STATISTICS : public DIALOG_BOARD_STATISTICS_BASE
{
public:
    /**
     * Struct to hold type information, which will be shown in dialog.
     */
    template <typename T>
    struct typeContainer_t
    {
        typeContainer_t<T>( T aAttribute, wxString aTitle )
                : attribute( aAttribute ),
                  title( aTitle ),
                  qty( 0 )
        {
        }
        T          attribute;
        wxString   title;
        int        qty;
    };

    using padsType_t = typeContainer_t<PAD_ATTR_T>;
    using viasType_t = typeContainer_t<VIATYPE_T>;

    /**
     * Struct holds information about component type (such as SMD, THT,
     * Virtual and so on), which will be shown in the dialog. Holds both
     * front and bottom components quantities
     */
    struct componentsType_t
    {
        componentsType_t( MODULE_ATTR_T aAttribute, wxString aTitle )
                : attribute( aAttribute ),
                  title( aTitle ),
                  frontSideQty( 0 ),
                  backSideQty( 0 )
        {
        }
        MODULE_ATTR_T attribute;
        wxString      title;
        int           frontSideQty;
        int           backSideQty;
    };

    using componentsTypeList_t = std::deque<componentsType_t>;
    using padsTypeList_t = std::deque<padsType_t>;
    using viasTypeList_t = std::deque<viasType_t>;

    DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_STATISTICS();

    ///> Get data from the PCB board and print it to dialog
    bool TransferDataToWindow() override;

    ///> Function to fill up all items types to be shown in the dialog.
    void refreshItemsTypes();

    ///> Gets data from board
    void getDataFromPCB();

    ///> Applies data to dialog widgets
    void updateWidets();

private:
    PCB_EDIT_FRAME* m_parentFrame;
    int             m_boardWidth;
    int             m_boardHeight;
    double          m_boardArea;

    ///> Shows if board outline properly defined
    bool m_hasOutline;

    ///> Holds all components types to be shown in the dialog
    componentsTypeList_t m_componentsTypes;

    ///> Holds all pads types to be shown in the dialog
    padsTypeList_t  m_padsTypes;

    ///> Holds all vias types to be shown in the dialog
    viasTypeList_t m_viasTypes;

    ///> Save board statistics to a file
    void saveReportClicked( wxCommandEvent& event ) override;
    void checkboxClicked( wxCommandEvent& event ) override;
};

#endif // __DIALOG_BOARD_STATISTICS_H
