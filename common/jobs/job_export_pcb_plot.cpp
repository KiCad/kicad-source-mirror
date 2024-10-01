

#include <jobs/job_export_pcb_plot.h>

JOB_EXPORT_PCB_PLOT::JOB_EXPORT_PCB_PLOT( PLOT_FORMAT aFormat, const std::string& aType,
                                          bool aOutputIsDirectory,
                                          bool aIsCli )
	: JOB( aType, aOutputIsDirectory, aIsCli ),
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
	m_printMaskLayer(),
	m_drillShapeOption( 2 )
{

}