/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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

#ifndef DIALOG_BOARD_REANNOTATE_H
#define DIALOG_BOARD_REANNOTATE_H

#include <board.h>
#include <footprint.h>
#include <dialogs/dialog_board_reannotate_base.h>
#include <layer_ids.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <frame_type.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

#define SORTXFIRST 0b000       // Sort on X
#define SORTYFIRST 0b100       // Sort on Y
#define ASCENDINGFIRST 0b000   // Sort low to high
#define DESCENDINGFIRST 0b010  // Sort high to low
#define ASCENDINGSECOND 0b000  // Sort low to high
#define DESCENDINGSECOND 0b001 // Sort high to low

#define MINGRID 1000
#define MAXERROR 10
#define VALIDPREFIX "_-+=/\\"  // Prefixs can be alpha or these symbols

enum ACTION_CODE
{
    UPDATE_REFDES,
    EMPTY_REFDES,
    INVALID_REFDES,
    EXCLUDE_REFDES
};

enum ANNOTATION_SCOPE
{
    ANNOTATE_ALL = 0,
    ANNOTATE_FRONT,
    ANNOTATE_BACK,
    ANNOTATE_SELECTED
};

struct REFDES_CHANGE
{
    KIID        Uuid;
    wxString    NewRefDes;          // The new reference designation (F_U21)
    wxString    OldRefDesString;    // What the old refdes preamble + number was
    bool        Front;              // True if on the front of the board
    ACTION_CODE  Action;             // Used to skip (if #, etc)
};

struct REFDES_INFO
{
    KIID        Uuid;
    bool        Front;              // True if on the front of the board
    wxString    RefDesString;       // What its refdes is R1, C2
    wxString    RefDesPrefix;       // ie R, C, etc
    int         x, y;               // The coordinates
    int         roundedx, roundedy; // The coordinates after rounding.
    ACTION_CODE Action;             // Used to skip (if #, etc)
    LIB_ID      FPID;
};

struct REFDES_PREFIX_INFO
{
    wxString               RefDesPrefix;
    unsigned int           LastUsedRefDes;
    std::set<unsigned int> UnavailableRefs;
};


class DIALOG_BOARD_REANNOTATE : public DIALOG_BOARD_REANNOTATE_BASE
{
public:
    DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_REANNOTATE();

private:
    std::vector<wxRadioButton*> m_sortButtons = {
            m_Down_Right,
            m_Right_Down,
            m_Down_Left,
            m_Left_Down,
            m_Up_Right,
            m_Right_Up,
            m_Up_Left,
            m_Left_Up
    };

    std::vector<wxRadioButton*> m_scopeRadioButtons = {
            m_AnnotateAll,
            m_AnnotateFront,
            m_AnnotateBack,
            m_AnnotateSelection
    };

    std::vector<wxStaticBitmap*> Bitmaps = {
            reannotate_down_right_bitmap,
            reannotate_right_down_bitmap,
            reannotate_down_left_bitmap,
            reannotate_left_down_bitmap,
            reannotate_up_right_bitmap,
            reannotate_right_up_bitmap,
            reannotate_up_left_bitmap,
            reannotate_left_up_bitmap
    };

    void GetParameters( void );

    /// Copy saved app settings to the dialog.
    void InitValues( void );

    void OnApplyClick( wxCommandEvent& event ) override;
    void OnCloseClick( wxCommandEvent& event ) override;
    void FilterFrontPrefix( wxCommandEvent& event ) override;
    void FilterBackPrefix( wxCommandEvent& event ) override;

    /// Break report into strings separated by \n and sent to the reporter.
    void ShowReport( const wxString& aMessage, SEVERITY aSeverity );

	/// Create a list of the footprints and their coordinates.
    void LogFootprints( const wxString& aMessage, const std::vector<REFDES_INFO>& aFootprints );

    /// Create an audit trail of the changes.
    void LogChangePlan( void );

    /// Actually reannotate the board.
    /// @return false if fail, true if success.
    bool ReannotateBoard( void );

    /// Build the footprint lists, sort it, filter for excludes, then build the change list.
    /// @return true if success, false if errors
    bool BuildFootprintList( std::vector<REFDES_INFO>& aBadRefDes );

    /// Build list of unavailable references. E.g. unselected footprints or locked footprints.
    void BuildUnavailableRefsList();

    /// Scan through the footprint arrays and create the from -> to array.
    void BuildChangeArray( std::vector<REFDES_INFO>& aFootprints, unsigned int aStartRefDes,
                           const wxString& aPrefix, bool aRemovePrefix,
                           std::vector<REFDES_INFO>& aBadRefDes );

    /// @return the new reference for this footprint.
    REFDES_CHANGE* GetNewRefDes( FOOTPRINT* aFootprint );

    /// Round an int coordinate to a suitable grid
    int RoundToGrid( int aCoord, int aGrid );

    /// Convert coordinates to wxString.
    /// @return the string
    wxString CoordTowxString( int aX, int aY );

    /// Make the text to summarize what is about to happen.
    void MakeSampleText( wxString& aMessage );

    /// Check to make sure the prefix (if there is one) is properly constructed.
    void FilterPrefix( wxTextCtrl* aPrefix );

    /// Get the structure representing the information currently held for aRefDesPrefix or create one
    /// if it doesn't exist
    REFDES_PREFIX_INFO* GetOrBuildRefDesInfo( const wxString& aRefDesPrefix, int aStartRefDes = 1 );

    PCB_EDIT_FRAME*  m_frame;
    FOOTPRINTS       m_footprints;
    PCB_SELECTION    m_selection;

    std::vector<REFDES_CHANGE>      m_changeArray;
    std::vector<REFDES_INFO>        m_frontFootprints;
    std::vector<REFDES_INFO>        m_backFootprints;
    std::vector<REFDES_PREFIX_INFO> m_refDesPrefixInfos;
    std::vector<wxString>           m_excludeArray;

    int      m_sortCode;
    int      m_gridIndex;
    int      m_annotationScope;
    int      m_severity;

    double   m_sortGridx;
    double   m_sortGridy;

    wxString m_frontPrefixString;
    wxString m_backPrefixString;
    wxString m_validPrefixes;

    APP_SETTINGS_BASE* m_settings;

    APP_SETTINGS_BASE* m_Config;
};

#endif /* DIALOG_BOARD_REANNOTATE_H */
