///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
///////////////////////////////////////////////////////////////////////////

#ifndef __DialogLayerSetup__
#define __DialogLayerSetup__

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/scrolwin.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////////
/// Class DialogLayerSetup
///////////////////////////////////////////////////////////////////////////////
class DialogLayerSetup : public wxDialog
{
    private:

    protected:
        wxPanel*                        m_MainPanel;

            wxStaticText*               m_PresetsCaption;
            wxChoice*                   m_PresetsChoice;

            wxStaticText*               m_LayerNumberCaption;
            wxChoice*                   m_LayerNumberChoice;

            wxStaticLine*               m_Separator1;

            wxStaticText*               m_LayersCaptionText;

            wxPanel*                    m_LayersPanel;

                wxStaticText*           m_LayerNameCaptionText;
                wxStaticText*           m_LayerEnabledCaptionText;
                wxStaticText*           m_LayerTypeCaptionText;

                wxScrolledWindow*       m_LayerListScroller;

                    wxPanel*            m_LayerNamePanel[NB_LAYERS];
                        wxStaticText*   m_LayerNameStaticText[NB_LAYERS-NB_COPPER_LAYERS];
                        wxTextCtrl*     m_LayerNameTextCtrl[NB_COPPER_LAYERS];

                    wxPanel*            m_LayerEnabledPanel[NB_LAYERS];
                        wxCheckBox*     m_LayerEnabledCheckBox[NB_LAYERS];

                    wxPanel*            m_LayerTypePanel[NB_LAYERS];
                        wxStaticText*   m_LayerTypeStaticText[NB_LAYERS-NB_COPPER_LAYERS];
                        wxChoice*       m_LayerTypeChoice[NB_COPPER_LAYERS];

            wxStaticLine*               m_Separator2;

            wxStdDialogButtonSizer*     m_StdButtonsSizer;
                wxButton*               m_StdButtonsSizerOK;
                wxButton*               m_StdButtonsSizerCancel;

        int                             m_LayersMask;

        static wxPoint                  m_DialogLastPosition;

        WinEDA_PcbFrame*                m_Parent;
        BOARD*                          m_Pcb;

        
        wxString GetLayerName( int Layer );
        int GetLayerType( int Layer );
        void SetLayerName( int Layer, wxString Name );
        void SetLayerType( int Layer, LAYER_T Type );
        int GetLayersMask();


        // Virtual event handlers, overide them in your derived class
        virtual void OnPresetChoice( wxCommandEvent& event );
        virtual void OnCopperLayersChoice( wxCommandEvent& event );

        virtual void OnLayerNameKillFocus( wxFocusEvent& event );
        virtual void OnLayerNameSetFocus( wxFocusEvent& event );

        virtual void OnLayerEnabledCheckBox( wxCommandEvent& event );

        virtual void OnLayerEnabledSetFocus( wxFocusEvent& event );
        virtual void OnLayerEnabledKillFocus( wxFocusEvent& event );

        virtual void OnLayerTypeChoice( wxCommandEvent& event ){ event.Skip(); }

        virtual void OnLayerTypeSetFocus( wxFocusEvent& event );
        virtual void OnLayerTypeKillFocus( wxFocusEvent& event );

        virtual void OnCancelClick( wxCommandEvent& event );
        virtual void OnOKClick( wxCommandEvent& event );

        void UpdateCheckBoxes();
        void UpdateCopperLayersChoice();
        void UpdatePresetsChoice();

    public:
        DialogLayerSetup(   WinEDA_PcbFrame* parent,
                            const wxPoint&  pos     = wxDefaultPosition,
                            wxWindowID      id      = wxID_ANY,
                            const wxString& title   = wxT("Layer Setup"),
                            const wxSize&   size    = wxSize( -1,-1 ),
                            long            style   = wxDEFAULT_DIALOG_STYLE );

        ~DialogLayerSetup();
};

#endif //__DialogLayerSetup__
