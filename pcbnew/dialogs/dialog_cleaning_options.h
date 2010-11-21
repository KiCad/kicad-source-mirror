/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_cleaning_options.h
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

#ifndef DIALOG_CLEANING_OPTIONS_H_
#define DIALOG_CLEANING_OPTIONS_H_

#include "dialog_cleaning_options_base.h"

class DIALOG_CLEANING_OPTIONS: public DIALOG_CLEANING_OPTIONS_BASE
{
public:
    static bool cleanVias;
    static bool mergeSegments;
    static bool deleteUnconnectedSegm;
	static bool connectToPads;

public:
    DIALOG_CLEANING_OPTIONS( wxWindow* parent );

    ~DIALOG_CLEANING_OPTIONS()
    {
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

		void OnCloseWindow( wxCloseEvent& event )
        {
            GetOpts( );
        }

        void GetOpts( )
        {
            cleanVias = m_cleanViasOpt->GetValue( );
            mergeSegments = m_mergeSegmOpt->GetValue( );
            deleteUnconnectedSegm = m_deleteUnconnectedOpt->GetValue( );
            connectToPads = m_reconnectToPadsOpt->GetValue( );
        }
};

#endif
    // DIALOG_CLEANING_OPTIONS_H_
