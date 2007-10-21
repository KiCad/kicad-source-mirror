/* Set up the button list for the color selection for pcbnew layers */

static ColorButton Msg_Layers_Cu =
{
    _( "Copper Layers" ), -1    // Title
};

static ColorButton Layer_1_Butt =
{
    wxEmptyString,
    COPPER_LAYER_N,             // Layer
    ADR( COPPER_LAYER_N ),      // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_2_Butt =
{
    wxEmptyString,
    1,                          // Layer
    ADR( 1 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_3_Butt =
{
    wxEmptyString,
    2,                          // Layer
    ADR( 2 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_4_Butt =
{
    wxEmptyString,
    3,                          // Layer
    ADR( 3 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_5_Butt =
{
    wxEmptyString,
    4,                          // Layer
    ADR( 4 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_6_Butt =
{
    wxEmptyString,
    5,                          // Layer
    ADR( 5 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_7_Butt =
{
    wxEmptyString,
    6,                          // Layer
    ADR( 6 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_8_Butt =
{
    wxEmptyString,
    7,                          // Layer
    ADR( 7 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_9_Butt =
{
    wxEmptyString,
    8,                          // Layer
    ADR( 8 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_10_Butt =
{
    wxEmptyString,
    9,                          // Layer
    ADR( 9 ),                   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_11_Butt =
{
    wxEmptyString,
    10,                         // Layer
    ADR( 10 ),                  // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_12_Butt =
{
    wxEmptyString,
    11,                         // Layer
    ADR( 11 ),                  // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_13_Butt =
{
    wxEmptyString,
    12,                         // Layer
    ADR( 12 ),                  // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_14_Butt =
{
    wxEmptyString,
    13,                         // Layer
    ADR( 13 ),                  // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_15_Butt =
{
    wxEmptyString,
    14,                         // Layer
    ADR( 14 ),                  // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_16_Butt =
{
    wxEmptyString,
    CMP_N,                      // Layer
    ADR( CMP_N ),               // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};


static ColorButton Msg_Layers_Tech =
{
    _( "Tech Layers" ), -1      // Title
};

static ColorButton Layer_17_Butt =
{
    wxEmptyString,
    ADHESIVE_N_CU,              // Layer
    ADR( ADHESIVE_N_CU ),       // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_18_Butt =
{
    wxEmptyString,
    ADHESIVE_N_CMP,             // Layer
    ADR( ADHESIVE_N_CMP ),      // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_19_Butt =
{
    wxEmptyString,
    SOLDERPASTE_N_CU,           // Layer
    ADR( SOLDERPASTE_N_CU ),    // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_20_Butt =
{
    wxEmptyString,
    SOLDERPASTE_N_CMP,          // Layer
    ADR( SOLDERPASTE_N_CMP ),   // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_21_Butt =
{
    wxEmptyString,
    SILKSCREEN_N_CU,            // Layer
    ADR( SILKSCREEN_N_CU ),     // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_22_Butt =
{
    wxEmptyString,
    SILKSCREEN_N_CMP,           // Layer
    ADR( SILKSCREEN_N_CMP ),    // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_23_Butt =
{
    wxEmptyString,
    SOLDERMASK_N_CU,            // Layer
    ADR( SOLDERMASK_N_CU ),     // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_24_Butt =
{
    wxEmptyString,
    SOLDERMASK_N_CMP,           // Layer
    ADR( SOLDERMASK_N_CMP ),    // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_25_Butt =
{
    wxEmptyString,
    DRAW_N,                     // Layer
    ADR( DRAW_N ),              // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_26_Butt =
{
    wxEmptyString,
    COMMENT_N,                  // Layer
    ADR( COMMENT_N ),           // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_27_Butt =
{
    wxEmptyString,
    ECO1_N,                     // Layer
    ADR( ECO1_N ),              // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_28_Butt =
{
    wxEmptyString,
    ECO2_N,                     // Layer
    ADR( ECO2_N ),              // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Layer_29_Butt =
{
    wxEmptyString,
    EDGE_N,                     // Layer
    ADR( EDGE_N ),              // Address of optional parameter
    TRUE                        // Toggle ITEM_NOT_SHOW bit of the color variable
};


static ColorButton Msg_Others_Items =
{
    wxT( "Others" ), -1                         // Title
};

static ColorButton VIA_THROUGH_Butt =
{
    wxT( "*" ),
    VIA_THROUGH,                                // Layer
    &g_DesignSettings.m_ViaColor[VIA_THROUGH],  // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Via_Aveugle_Butt =
{
    wxT( "*" ),
    VIA_BLIND,                                  // Layer
    &g_DesignSettings.m_ViaColor[VIA_BLIND],    // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton BLIND_VIA_Butt =
{
    wxT( "*" ),
    VIA_BURIED,                                 // Layer
    &g_DesignSettings.m_ViaColor[VIA_BURIED],   // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Ratsnest_Butt =
{
    _( "Ratsnest" ),                            // Title
    -1,
    &g_DesignSettings.m_RatsnestColor,          // Address of optional parameter
    FALSE,
    &g_Show_Ratsnest                            // Address of boolean display control parameter to toggle
};

static ColorButton Pad_Cu_Butt =
{
    _( "Pad Cu" ),                              // Title
    -1,
    &g_PadCUColor,                              // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Pad_Cmp_Butt =
{
    _( "Pad Cmp" ),                             // Title
    -1,
    &g_PadCMPColor,                             // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Text_Mod_Cu_Butt =
{
    _( "Text Module Cu" ),                      // Title
    -1,
    &g_ModuleTextCUColor,                       // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Text_Mod_Cmp_Butt =
{
    _( "Text Module Cmp" ),                     // Title
    -1,
    &g_ModuleTextCMPColor,                      // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Text_Mod_NoVisible_Butt =
{
    _( "Text Module invisible" ),               // Title
    -1,
    &g_ModuleTextNOVColor,                      // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Anchors_Butt =
{
    _( "Anchors" ),                             // Title
    -1,
    &g_AnchorColor,                             // Address of optional parameter
    TRUE                                        // Toggle ITEM_NOT_SHOW bit of the color variable
};

static ColorButton Grid_Butt =
{
    _( "Grid" ),                                // Title
    -1,
    &g_GridColor,                               // Address of optional parameter
    FALSE,
    &g_ShowGrid                                 // Address of boolean display control parameter to toggle
};

static ColorButton Show_Zones_Butt =
{
    _( "Show Zones" ),                          // Title
    -1,
    NULL,                                       // Address of optional parameter
    FALSE,
    &DisplayOpt.DisplayZones                    // Address of boolean display control parameter to toggle
};

static ColorButton Show_Pads_Noconnect_Butt =
{
    _( "Show Noconnect" ),                      // Title
    -1,
    NULL,                                       // Address of optional parameter
    FALSE,
    &DisplayOpt.DisplayPadNoConn                // Address of boolean display control parameter to toggle
};

static ColorButton Show_Modules_Cmp_Butt =
{
    _( "Show Modules Cmp" ),                    // Title
    -1,
    NULL,                                       // Address of optional parameter
    FALSE,
    &DisplayOpt.Show_Modules_Cmp                // Address of boolean display control parameter to toggle
};

static ColorButton Show_Modules_Cu_Butt =
{
    _( "Show Modules Cu" ),                     // Title
    -1,
    NULL,                                       // Address of optional parameter
    FALSE,
    &DisplayOpt.Show_Modules_Cu                 // Address of boolean display control parameter to toggle
};


static ColorButton* laytool_list[] = {
    &Msg_Layers_Cu,
    &Layer_1_Butt,
    &Layer_2_Butt,
    &Layer_3_Butt,
    &Layer_4_Butt,
    &Layer_5_Butt,
    &Layer_6_Butt,
    &Layer_7_Butt,
    &Layer_8_Butt,
    &Layer_9_Butt,
    &Layer_10_Butt,
    &Layer_11_Butt,
    &Layer_12_Butt,
    &Layer_13_Butt,
    &Layer_14_Butt,
    &Layer_15_Butt,
    &Layer_16_Butt,

    &Msg_Layers_Tech,
    &Layer_17_Butt,
    &Layer_18_Butt,
    &Layer_19_Butt,
    &Layer_20_Butt,
    &Layer_21_Butt,
    &Layer_22_Butt,
    &Layer_23_Butt,
    &Layer_24_Butt,
    &Layer_25_Butt,
    &Layer_26_Butt,
    &Layer_27_Butt,
    &Layer_28_Butt,
    &Layer_29_Butt,
//  &Layer_30_Butt,
//  &Layer_31_Butt,
//  &Layer_32_Butt,

    &Msg_Others_Items,
    &VIA_THROUGH_Butt,
    &Via_Aveugle_Butt,
    &BLIND_VIA_Butt,
    &Ratsnest_Butt,
    &Pad_Cu_Butt,
    &Pad_Cmp_Butt,
    &Text_Mod_Cu_Butt,
    &Text_Mod_Cmp_Butt,
    &Text_Mod_NoVisible_Butt,
    &Anchors_Butt,
    &Grid_Butt,

    &Show_Zones_Butt,
    &Show_Pads_Noconnect_Butt,
    &Show_Modules_Cmp_Butt,
    &Show_Modules_Cu_Butt,

    NULL
};
