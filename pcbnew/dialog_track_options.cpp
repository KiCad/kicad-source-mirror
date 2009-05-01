/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_track_options.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     17 feb 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"

#include "dialog_track_options.h"


/**
 *  DIALOG_TRACKS_OPTIONS, derived from DIALOG_TRACKS_OPTIONS_BASE
 *  @see dialog_track_options_base.h and dialog_track_options_base.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_TRACKS_OPTIONS::DIALOG_TRACKS_OPTIONS( WinEDA_PcbFrame* parent )
    : DIALOG_TRACKS_OPTIONS_BASE(parent)
{
    m_Parent = parent;
}


void DIALOG_TRACKS_OPTIONS::OnInitDialog( wxInitDialogEvent& event )
{
    SetFocus();

    // deselect the existing text, seems SetFocus() wants to emulate Microsoft, which is not desireable here.
    m_OptViaSize->SetSelection( 0, 0 );

    SetDisplayValue();

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    event.Skip();
}


/*************************************************/
void DIALOG_TRACKS_OPTIONS::SetDisplayValue()
/*************************************************/
{
    AddUnitSymbol( *m_ViaSizeTitle );
    AddUnitSymbol( *m_MicroViaSizeTitle );
    AddUnitSymbol( *m_ViaDefaultDrillValueTitle );
    AddUnitSymbol( *m_MicroViaDrillTitle );
    AddUnitSymbol( *m_ViaAltDrillValueTitle );
    AddUnitSymbol( *m_TrackWidthTitle );
    AddUnitSymbol( *m_TrackClearanceTitle );
    AddUnitSymbol( *m_MaskClearanceTitle );

    int Internal_Unit = m_Parent->m_InternalUnits;
    PutValueInLocalUnits( *m_OptViaSize, g_DesignSettings.m_CurrentViaSize, Internal_Unit );
    PutValueInLocalUnits( *m_MicroViaSizeCtrl,
                          g_DesignSettings.m_CurrentMicroViaSize,
                          Internal_Unit );
    PutValueInLocalUnits( *m_OptViaDrill, g_DesignSettings.m_ViaDrill, Internal_Unit );
    PutValueInLocalUnits( *m_MicroViaDrillCtrl, g_DesignSettings.m_MicroViaDrill, Internal_Unit );
    PutValueInLocalUnits( *m_OptCustomViaDrill,
                          g_DesignSettings.m_ViaDrillCustomValue,
                          Internal_Unit );
    PutValueInLocalUnits( *m_OptTrackWidth, g_DesignSettings.m_CurrentTrackWidth, Internal_Unit );
    PutValueInLocalUnits( *m_OptTrackClearance, g_DesignSettings.m_TrackClearence, Internal_Unit );
    PutValueInLocalUnits( *m_OptMaskMargin, g_DesignSettings.m_MaskMargin, Internal_Unit );
    if( g_DesignSettings.m_CurrentViaType != VIA_THROUGH )
        m_OptViaType->SetSelection( 1 );

    m_MicroViaSizeTitle->Enable( g_DesignSettings.m_MicroViasAllowed );
    m_MicroViaSizeCtrl->Enable( g_DesignSettings.m_MicroViasAllowed );

    m_MicroViaDrillTitle->Enable( g_DesignSettings.m_MicroViasAllowed );
    m_MicroViaDrillCtrl->Enable( g_DesignSettings.m_MicroViasAllowed );

    m_AllowMicroViaCtrl->SetValue( g_DesignSettings.m_MicroViasAllowed );

}


/*******************************************************************/
void DIALOG_TRACKS_OPTIONS::OnButtonOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    g_DesignSettings.m_CurrentViaType = VIA_THROUGH;
    if( m_OptViaType->GetSelection() > 0 )
        g_DesignSettings.m_CurrentViaType = VIA_BLIND_BURIED;

    g_DesignSettings.m_CurrentViaSize =
        ReturnValueFromTextCtrl( *m_OptViaSize, m_Parent->m_InternalUnits );
    g_DesignSettings.m_CurrentMicroViaSize =
        ReturnValueFromTextCtrl( *m_MicroViaSizeCtrl, m_Parent->m_InternalUnits );

    g_DesignSettings.m_MicroViaDrill =
        ReturnValueFromTextCtrl( *m_MicroViaDrillCtrl, m_Parent->m_InternalUnits );
    g_DesignSettings.m_ViaDrill =
        ReturnValueFromTextCtrl( *m_OptViaDrill, m_Parent->m_InternalUnits );
    g_DesignSettings.m_ViaDrillCustomValue =
        ReturnValueFromTextCtrl( *m_OptCustomViaDrill, m_Parent->m_InternalUnits );
    g_DesignSettings.m_MicroViasAllowed = m_AllowMicroViaCtrl->IsChecked();

    g_DesignSettings.m_CurrentTrackWidth =
        ReturnValueFromTextCtrl( *m_OptTrackWidth, m_Parent->m_InternalUnits );
    g_DesignSettings.m_TrackClearence =
        ReturnValueFromTextCtrl( *m_OptTrackClearance, m_Parent->m_InternalUnits );

    g_DesignSettings.m_MaskMargin =
        ReturnValueFromTextCtrl( *m_OptMaskMargin, m_Parent->m_InternalUnits );

    m_Parent->DisplayTrackSettings();

    m_Parent->AddHistory( g_DesignSettings.m_CurrentViaSize, TYPE_VIA );
    m_Parent->AddHistory( g_DesignSettings.m_CurrentTrackWidth, TYPE_TRACK );
    EndModal( 1 );
}


/*********************************************************************/
void WinEDA_BasePcbFrame::AddHistory( int value, KICAD_T type )
/**********************************************************************/

// Mise a jour des listes des dernieres epaisseurs de via et track utilisées
{
    bool addhistory = TRUE;
    int  ii;

    switch( type )
    {
    case TYPE_TRACK:
        for( ii = 0; ii < HISTORY_NUMBER; ii++ )
        {
            if( g_DesignSettings.m_TrackWidthHistory[ii] == value )
            {
                addhistory = FALSE;
                break;
            }
        }

        if( !addhistory )
            break;

        for( ii = HISTORY_NUMBER - 1;   ii > 0;  ii-- )
        {
            g_DesignSettings.m_TrackWidthHistory[ii] =
                g_DesignSettings.m_TrackWidthHistory[ii - 1];
        }

        g_DesignSettings.m_TrackWidthHistory[0] = value;

        // Reclassement par valeur croissante
        for( ii = 0; ii < HISTORY_NUMBER - 1; ii++ )
        {
            if( g_DesignSettings.m_TrackWidthHistory[ii + 1] == 0 )
                break;                                                          // Fin de liste

            if( g_DesignSettings.m_TrackWidthHistory[ii] >
                g_DesignSettings.m_TrackWidthHistory[ii + 1]  )
            {
                EXCHG( g_DesignSettings.m_TrackWidthHistory[ii],
                       g_DesignSettings.m_TrackWidthHistory[ii + 1] );
            }
        }

        break;

    case TYPE_VIA:
        for( ii = 0; ii < HISTORY_NUMBER; ii++ )
        {
            if( g_DesignSettings.m_ViaSizeHistory[ii] == value )
            {
                addhistory = FALSE;
                break;
            }
        }

        if( !addhistory )
            break;

        for( ii = HISTORY_NUMBER - 1;  ii > 0;  ii-- )
        {
            g_DesignSettings.m_ViaSizeHistory[ii] = g_DesignSettings.m_ViaSizeHistory[ii - 1];
        }

        g_DesignSettings.m_ViaSizeHistory[0] = value;

        // Reclassement par valeur croissante
        for( ii = 0;  ii < HISTORY_NUMBER - 1;  ii++ )
        {
            if( g_DesignSettings.m_ViaSizeHistory[ii + 1] == 0 )
                break;                                                      // Fin de liste

            if( g_DesignSettings.m_ViaSizeHistory[ii] > g_DesignSettings.m_ViaSizeHistory[ii + 1]  )
            {
                EXCHG( g_DesignSettings.m_ViaSizeHistory[ii],
                       g_DesignSettings.m_ViaSizeHistory[ii + 1] );
            }
        }

        break;

    default:
        break;
    }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_TRACKS_OPTIONS::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_ALLOWS_MICROVIA
 */

void DIALOG_TRACKS_OPTIONS::OnCheckboxAllowsMicroviaClick( wxCommandEvent& event )
{
    bool state = m_AllowMicroViaCtrl->IsChecked();

    m_MicroViaSizeTitle->Enable( state );
    m_MicroViaSizeCtrl->Enable( state );
    m_MicroViaDrillTitle->Enable( state );
    m_MicroViaDrillCtrl->Enable( state );
}
