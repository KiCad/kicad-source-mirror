////////////////////////////////////////////
// Name:        dialog_display_options.h
// Licence:     GPL
////////////////////////////////////////////

#ifndef _DIALOG_DISPLAY_OPTIONS_H_
#define _DIALOG_DISPLAY_OPTIONS_H_

#include <dialog_display_options_base.h>

////////////////////////////////////////////
/// Class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS
//  derived from DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE,
//  created by wxformBuilder
////////////////////////////////////////////

class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS :
    public DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE
{
private:
PCB_BASE_FRAME * m_Parent;

public:
    DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( PCB_BASE_FRAME* parent );
    ~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS();


private:
    void initDialog( );
    void UpdateObjectSettings( void );
    virtual void OnApplyClick( wxCommandEvent& event );
    virtual void OnCancelClick( wxCommandEvent& event );
    virtual void OnOkClick( wxCommandEvent& event );
};

#endif      // _DIALOG_DISPLAY_OPTIONS_H_
