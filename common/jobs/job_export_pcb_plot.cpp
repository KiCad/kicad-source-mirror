

#include <jobs/job_export_pcb_plot.h>

JOB_EXPORT_PCB_PLOT::JOB_EXPORT_PCB_PLOT( PLOT_FORMAT aFormat, const std::string& aType,
                                          bool aOutputIsDirectory )
    : JOB( aType, aOutputIsDirectory ),
    m_plotFormat( aFormat ),
    m_filename(),
    m_colorTheme(),
    m_drawingSheet(),
    m_mirror( false ),
    m_blackAndWhite( false ),
    m_negative( false ),
    m_sketchPadsOnFabLayers( false ),
    m_hideDNPFPsOnFabLayers( false ),
    m_sketchDNPFPsOnFabLayers( true ),
    m_crossoutDNPFPsOnFabLayers( true ),
    m_plotFootprintValues( true ),
    m_plotRefDes( true ),
    m_plotDrawingSheet( true ),
    m_plotPadNumbers( false ),
    m_plotInvisibleText( false ),
    m_printMaskLayer(),
    m_printMaskLayersToIncludeOnAllLayers(),
    m_drillShapeOption( DRILL_MARKS::FULL_DRILL_SHAPE ),
    m_useDrillOrigin( false )
{
    m_params.emplace_back( new JOB_PARAM<LSEQ>( "layers", &m_printMaskLayer, m_printMaskLayer ) );
    m_params.emplace_back( new JOB_PARAM<LSEQ>( "layers_to_include_on_all_layers",
                                                &m_printMaskLayersToIncludeOnAllLayers,
                                                m_printMaskLayersToIncludeOnAllLayers ) );


    m_params.emplace_back(
            new JOB_PARAM<bool>( "plot_pad_numbers", &m_plotPadNumbers, m_plotPadNumbers ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_invisible_text", &m_plotInvisibleText,
                                                m_plotInvisibleText ) );

    m_params.emplace_back(
            new JOB_PARAM<bool>( "plot_drawing_sheet", &m_plotDrawingSheet, m_plotDrawingSheet ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_ref_des", &m_plotRefDes, m_plotRefDes ) );

    m_params.emplace_back(
            new JOB_PARAM<bool>( "use_drill_origin", &m_useDrillOrigin, m_useDrillOrigin ) );

    m_params.emplace_back(
            new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
}