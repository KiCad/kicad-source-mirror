/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_track_options.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     17 feb 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "dialog_track_options.h"
#include <algorithm>

/**
 *  DIALOG_TRACKS_OPTIONS, derived from DIALOG_TRACKS_OPTIONS_BASE
 *  @see dialog_track_options_base.h and dialog_track_options_base.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_TRACKS_OPTIONS::DIALOG_TRACKS_OPTIONS( WinEDA_PcbFrame* parent ) :
    DIALOG_TRACKS_OPTIONS_BASE( parent )
{
    m_Parent = parent;
    MyInit();
    GetSizer()->SetSizeHints( this );
}


void DIALOG_TRACKS_OPTIONS::MyInit()
{
    SetFocus();

    AddUnitSymbol( *m_MaskClearanceTitle );

    int Internal_Unit = m_Parent->m_InternalUnits;
    PutValueInLocalUnits( *m_OptCustomViaDrill,
                          g_DesignSettings.m_ViaDrillCustomValue,
                          Internal_Unit );
    PutValueInLocalUnits( *m_OptMaskMargin, g_DesignSettings.m_MaskMargin, Internal_Unit );

    // Vias and Tracks sizes values.
    // note we display only extra values, never the current netclass value.
    // (the first value in histories list)
    m_TracksWidthList = m_Parent->GetBoard()->m_TrackWidthList;
    m_TracksWidthList.erase( m_TracksWidthList.begin() );   // remove the netclass value
    m_ViasDiameterList = m_Parent->GetBoard()->m_ViaSizeList;
    m_ViasDiameterList.erase( m_ViasDiameterList.begin() ); // remove the netclass value
    // Display values:
    InitDimensionsLists();
}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonDeleteViaSizeClick( wxCommandEvent& event )
/*******************************************************************/
{
    int isel = m_ViaSizeListCtrl->GetSelection();

    if( isel < 0 )
        return;
    m_ViasDiameterList.erase( m_ViasDiameterList.begin() + isel );
    InitDimensionsLists();
}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonAddViaSizeClick( wxCommandEvent& event )
/*******************************************************************/
{
    wxString msg = wxGetTextFromUser( wxEmptyString,
                                      _( "Enter new via diameter value:" ), wxEmptyString, this );

    if( msg.IsEmpty() )
        return;

    bool error = false;
    int  value = ReturnValueFromString( g_UnitMetric, msg, m_Parent->m_InternalUnits );

    if( value <= 0 )
        error = true;
    if( value > 10000 )        //  a value > 1 inch is surely a stupid value
        error = true;

    if( error )
    {
        DisplayError( this, _( "Incorrect entered value. Aborted" ) );
        return;
    }

    // values are sorted by increasing value in list, so we can use binary_search()
    // (see C++ Standard Template Library » C++ Algorithms » binary_search)
    if( binary_search( m_ViasDiameterList.begin(), m_ViasDiameterList.end(), value ) == false ) // value not already existing
    {
        if( m_ViasDiameterList.size() >= HISTORY_MAX_COUNT - 1 )
        {
            DisplayError( this, _( "Too many values in list (max count reached). Aborted" ) );
            return;
        }
        m_ViasDiameterList.push_back( value );

        // Sort new list by by increasing value
        sort( m_ViasDiameterList.begin(), m_ViasDiameterList.end() );
    }
    InitDimensionsLists();
}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonDeleteTrackSizeClick( wxCommandEvent& event )
/*******************************************************************/
{
    int isel = m_TrackWidthListCtrl->GetSelection();

    if( isel < 0 )
        return;
    m_TracksWidthList.erase( m_TracksWidthList.begin() + isel );
    InitDimensionsLists();
}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonAddTrackSizeClick( wxCommandEvent& event )
/*******************************************************************/
{
    wxString msg = wxGetTextFromUser( wxEmptyString,
                                      _( "Enter new track size value:" ), wxEmptyString, this );

    if( msg.IsEmpty() )
        return;

    bool error = false;
    int  value = ReturnValueFromString( g_UnitMetric, msg, m_Parent->m_InternalUnits );

    if( value <= 0 )
        error = true;
    if( value > 10000 )        // a value > 1 inche is surely a stupid value
        error = true;

    if( error )
    {
        DisplayError( this, _( "Incorrect entered value. Aborted" ) );
        return;
    }

    // values are sorted by increasing value in list, so we can use binary_search()
    // (see C++ Standard Template Library » C++ Algorithms » binary_search)
    if( binary_search( m_TracksWidthList.begin(), m_TracksWidthList.end(), value ) == false ) // value not already existing
    {
        if( m_TracksWidthList.size() >= HISTORY_MAX_COUNT - 1 )
        {
            DisplayError( this, _( "Too many values in list (max count reached). Aborted" ) );
            return;
        }
        m_TracksWidthList.push_back( value );

        // Sort new list by by increasing value
        sort( m_TracksWidthList.begin(), m_TracksWidthList.end() );
    }
    InitDimensionsLists();
}


/***************************************************/
void DIALOG_TRACKS_OPTIONS::InitDimensionsLists()
/***************************************************/

/* Populates the 2 lists of sizes (Tracks width list and Vias diameters list)
 */
{
    wxString msg;
    int      Internal_Unit = m_Parent->m_InternalUnits;

    m_TrackWidthListCtrl->Clear();
    for( unsigned ii = 0; ii < m_TracksWidthList.size(); ii++ )
    {
        msg = ReturnStringFromValue( g_UnitMetric, m_TracksWidthList[ii], Internal_Unit, true );
        m_TrackWidthListCtrl->Append( msg );
    }

    m_ViaSizeListCtrl->Clear();
    for( unsigned ii = 0; ii < m_ViasDiameterList.size(); ii++ )
    {
        msg = ReturnStringFromValue( g_UnitMetric, m_ViasDiameterList[ii], Internal_Unit, true );
        m_ViaSizeListCtrl->Append( msg );
    }
}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    g_DesignSettings.m_ViaDrillCustomValue =
        ReturnValueFromTextCtrl( *m_OptCustomViaDrill, m_Parent->m_InternalUnits );

    g_DesignSettings.m_MaskMargin =
        ReturnValueFromTextCtrl( *m_OptMaskMargin, m_Parent->m_InternalUnits );

    // Reinitialize m_TrackWidthList and m_ViaSizeList
    std::vector <int>* list = &m_Parent->GetBoard()->m_TrackWidthList;
    list->erase( list->begin() + 1, list->end() );  // Remove old "custom" sizes
    list->insert( list->end(), m_TracksWidthList.begin(), m_TracksWidthList.end() ); //Add new "custom" sizes

    list = &m_Parent->GetBoard()->m_ViaSizeList;
    list->erase( list->begin() + 1, list->end() );
    list->insert( list->end(), m_ViasDiameterList.begin(), m_ViasDiameterList.end() );

    EndModal( 1 );

    m_Parent->m_TrackAndViasSizesList_Changed = true;
    m_Parent->AuxiliaryToolBar_Update_UI();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_TRACKS_OPTIONS::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}

