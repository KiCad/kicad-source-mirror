/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <board_design_settings.h>
#include <layers_id_colors_and_visibility.h>
#include <pcbnew_id.h>


PARAM_CFG_ARRAY& FOOTPRINT_EDIT_FRAME::GetConfigurationSettings()
{
    auto& displ_opts = m_DisplayOptions;
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    // Update everything
    m_configParams.clear();   // boost::ptr_vector destroys the pointers inside

    // Display options:
    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorDisplayPolarCoords" ),
                                                    &m_PolarCoords, false ) );
    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorPadDisplayMode" ),
                                                    &displ_opts.m_DisplayPadFill, true ) );
    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorGraphicLinesDisplayMode" ),
                                                    &displ_opts.m_DisplayModEdgeFill, FILLED ) );
    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorTextsDisplayMode" ),
                                                    &displ_opts.m_DisplayModTextFill, FILLED ) );
    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorTextsDisplayMode" ),
                                                    &displ_opts.m_DisplayModTextFill, FILLED ) );
    m_configParams.push_back( new PARAM_CFG_WXSTRING( true, wxT( "FpEditorTextsRefDefaultText" ),
                                                    &settings.m_RefDefaultText, wxT( "REF**" ) ) );

    // design settings
    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorSilkLineWidth" ),
            &settings.m_LineThickness[ LAYER_CLASS_SILK ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 100.0 ),
            nullptr, 1/IU_PER_MM, wxT( "FpEditorGrlineWidth" ) ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorSilkTextSizeH" ),
            &settings.m_TextSize[ LAYER_CLASS_SILK ].x,
            Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, 1/IU_PER_MM, wxT( "FpEditorTextsDefaultSizeH" ) ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorSilkTextSizeV" ),
            &settings.m_TextSize[ LAYER_CLASS_SILK ].y,
            Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, 1/IU_PER_MM, wxT( "FpEditorTextsDefaultSizeV" ) ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorSilkTextThickness" ),
            &settings.m_TextThickness[ LAYER_CLASS_SILK ],
            Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH,
            nullptr, 1/IU_PER_MM, wxT( "FpEditorTextsDefaultThickness" ) ) );

    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorSilkTextItalic" ),
            &settings.m_TextItalic[ LAYER_CLASS_SILK ] ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorCopperLineWidth" ),
            &settings.m_LineThickness[ LAYER_CLASS_COPPER ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorCopperTextSizeV" ),
            &settings.m_TextSize[ LAYER_CLASS_COPPER ].y,
            Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE  ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorCopperTextSizeH" ),
            &settings.m_TextSize[ LAYER_CLASS_COPPER ].x,
            Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE  ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorCopperTextThickness" ),
            &settings.m_TextThickness[ LAYER_CLASS_COPPER ],
            Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorCopperTextItalic" ),
            &settings.m_TextItalic[ LAYER_CLASS_COPPER ] ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorEdgeCutLineWidth" ),
            &settings.m_LineThickness[ LAYER_CLASS_EDGES ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorCourtyardLineWidth" ),
            &settings.m_LineThickness[ LAYER_CLASS_COURTYARD ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorOthersLineWidth" ),
            &settings.m_LineThickness[ LAYER_CLASS_OTHERS ],
            Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorOthersTextSizeV" ),
            &settings.m_TextSize[ LAYER_CLASS_OTHERS ].x,
            Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorOthersTextSizeH" ),
            &settings.m_TextSize[ LAYER_CLASS_OTHERS ].y,
            Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorOthersTextSizeThickness" ),
            &settings.m_TextThickness[ LAYER_CLASS_OTHERS ],
            Millimeter2iu( DEFAULT_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH,
            nullptr, MM_PER_IU ) );

    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorOthersTextItalic" ),
            &settings.m_TextItalic[ LAYER_CLASS_OTHERS ] ) );

    m_configParams.push_back( new PARAM_CFG_WXSTRING( true, wxT( "FpEditorRefDefaultText" ),
            &settings.m_RefDefaultText, wxT( "REF**" ) ) );

    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorRefDefaultVisibility" ),
            &settings.m_RefDefaultVisibility, true ) );

    m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorRefDefaultLayer" ),
            &settings.m_RefDefaultlayer,
            int( F_SilkS ), int( F_SilkS ), int( F_Fab ) ) );

    m_configParams.push_back( new PARAM_CFG_WXSTRING( true, wxT( "FpEditorValueDefaultText" ),
            &settings.m_ValueDefaultText, wxT( "" ) ) );

    m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorValueDefaultVisibility" ),
            &settings.m_ValueDefaultVisibility, true ) );

    m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorValueDefaultLayer" ),
            &settings.m_ValueDefaultlayer,
            int( F_Fab ), int( F_SilkS ), int( F_Fab ) ) );

    return m_configParams;
}
