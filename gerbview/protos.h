/* declarations prototype */

/***************************/
/* select_layers_to_pcb.cpp*/
/***************************/
int* InstallDialogLayerPairChoice( WinEDA_GerberFrame* parent );

/***********************/
/* gerbview_config.cpp */
/***********************/
bool Read_Config();
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose );


/****************/
/* lay2plot.cpp */

void Print_PcbItems( BOARD* Pcb, wxDC* DC, int drawmode, int printmasklayer );

/*****************/
/* set_color.cpp */
/*****************/
void DisplayColorSetupFrame( WinEDA_DrawFrame* parent, const wxPoint& framepos );


/***************/
/* trpiste.cpp */
/***************/
void Trace_Segment( WinEDA_DrawPanel* panel,
                    wxDC*             DC,
                    TRACK*            pt_piste,
                    int               draw_mode );

