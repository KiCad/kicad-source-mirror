/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_cleaning_options.h
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

#ifndef DIALOG_CLEANING_OPTIONS_H_
#define DIALOG_CLEANING_OPTIONS_H_

#include <dialog_cleaning_options_base.h>

class DIALOG_CLEANING_OPTIONS: public DIALOG_CLEANING_OPTIONS_BASE
{
public:
    static bool m_cleanVias;
    static bool m_mergeSegments;
    static bool m_deleteUnconnectedSegm;

public:
    DIALOG_CLEANING_OPTIONS( wxWindow* parent );

    ~DIALOG_CLEANING_OPTIONS()
    {
        GetOpts( );
    }

private:
        void OnCancelClick( wxCommandEvent& event )
        {
            EndModal( wxID_CANCEL );
        }
        void OnOKClick( wxCommandEvent& event )
        {
            GetOpts( );
            EndModal( wxID_OK );
        }

        void GetOpts( )
        {
            m_cleanVias = m_cleanViasOpt->GetValue( );
            m_mergeSegments = m_mergeSegmOpt->GetValue( );
            m_deleteUnconnectedSegm = m_deleteUnconnectedOpt->GetValue( );
        }
};

#endif
    // DIALOG_CLEANING_OPTIONS_H_
