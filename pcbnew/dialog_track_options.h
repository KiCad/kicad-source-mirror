/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_track_options.h
// Author:      jean-pierre Charras
// Created:     17 feb 2009
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_TRACK_OPTIONS_H_
#define _DIALOG_TRACK_OPTIONS_H_

#include "dialog_track_options_base.h"

/**
 *  DIALOG_TRACKS_OPTIONS, derived from DIALOG_TRACKS_OPTIONS_BASE
 *  @see dialog_track_options_base.h and dialog_track_options_base.cpp,
 *  automatically created by wxFormBuilder
 */
class DIALOG_TRACKS_OPTIONS : public DIALOG_TRACKS_OPTIONS_BASE
{
public:
    WinEDA_PcbFrame* m_Parent;

public:
    DIALOG_TRACKS_OPTIONS( WinEDA_PcbFrame* parent );
    ~DIALOG_TRACKS_OPTIONS() {};
private:
    void SetDisplayValue();
    virtual void OnInitDialog( wxInitDialogEvent& event );
    virtual void OnCheckboxAllowsMicroviaClick( wxCommandEvent& event );
    virtual void OnButtonOkClick( wxCommandEvent& event );
    virtual void OnButtonCancelClick( wxCommandEvent& event );
};

#endif    // _DIALOG_TRACK_OPTIONS_H_
