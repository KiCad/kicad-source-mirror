/**
 * @file select_layers_to_pcb.h
 */

#ifndef _SELECT_LAYERS_TO_PCB_H_
#define _SELECT_LAYERS_TO_PCB_H_

#include <dialogs/dialog_layers_select_to_pcb_base.h>

/*
 * This dialog shows the gerber files loaded, and allows user to choose
 * equivalence between gerber layers and pcb layers
 */
class LAYERS_MAP_DIALOG : public LAYERS_MAP_DIALOG_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;
    int m_gerberActiveLayersCount;                  // Number of initialized gerber layers
    static int m_exportBoardCopperLayersCount;
    wxFlexGridSizer* m_flexRightColumnBoxSizer;     // An extra wxFlexGridSizer used
                                                    // when we have more than 16 gerber files loaded
    LAYER_NUM m_layersLookUpTable[GERBER_DRAWLAYERS_COUNT]; // Indexes Gerber layers to PCB file layers
                                                    // the last value in table is the number of copper layers
    int    m_buttonTable[int(GERBER_DRAWLAYERS_COUNT)+1];       // Indexes buttons to Gerber layers
    wxStaticText* m_layersList[int(GERBER_DRAWLAYERS_COUNT)+1]; // Indexes text strings to buttons

public: LAYERS_MAP_DIALOG( GERBVIEW_FRAME* parent );
    ~LAYERS_MAP_DIALOG() {};

    LAYER_NUM * GetLayersLookUpTable() { return m_layersLookUpTable; }
    static int GetCopperLayersCount() { return m_exportBoardCopperLayersCount; }

private:
    void initDialog();
    void normalizeBrdLayersCount();
    void OnBrdLayersCountSelection( wxCommandEvent& event );
    void OnSelectLayer( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    void OnStoreSetup( wxCommandEvent& event );
    void OnGetSetup( wxCommandEvent& event );
    void OnResetClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

#endif      // _SELECT_LAYERS_TO_PCB_H_
