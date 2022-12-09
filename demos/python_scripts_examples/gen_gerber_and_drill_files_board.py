'''
    A python script example to create plot files to build a board:
    Gerber files
    Drill files
    Map dril files

    Important note:
        this python script does not plot frame references (page layout).
        the reason is it is not yet possible from a python script because plotting
        plot frame references needs loading the corresponding page layout file
        (.wks file) or the default template.

        This info (the page layout template) is not stored in the board, and therefore
        not available.

        Do not try to change SetPlotFrameRef(False) to SetPlotFrameRef(true)
        the result is the pcbnew lib will crash if you try to plot
        the unknown frame references template.

        Anyway, in gerber and drill files the page layout is not plot
'''

import sys
import os
from pcbnew import *

# A helper function to convert a UTF8 string for python2 or python3
def fromUTF8Text( afilename ):
    if sys.version_info < (3, 0):
        return afilename.encode()
    else:
        return afilename

filename=sys.argv[1]

board = LoadBoard(filename)

plotDir = "plot/"

#prepare the gerber job file
gen_job_file=True

pctl = PLOT_CONTROLLER(board)

popt = pctl.GetPlotOptions()

popt.SetOutputDirectory(plotDir)

# Set some important plot options (see pcb_plot_params.h):
popt.SetPlotFrameRef(False)     #do not change it
popt.SetSketchPadLineWidth(FromMM(0.1))

popt.SetAutoScale(False)        #do not change it
popt.SetScale(1)                #do not change it
popt.SetMirror(False)
popt.SetUseGerberAttributes(True)
popt.SetIncludeGerberNetlistInfo(True)
popt.SetCreateGerberJobFile(gen_job_file)
popt.SetUseGerberProtelExtensions(False)
popt.SetUseAuxOrigin(True)

# This by gerbers only
popt.SetSubtractMaskFromSilk(False)
# Disable plot pad holes
popt.SetDrillMarksType( DRILL_MARKS_NO_DRILL_SHAPE );
# Skip plot pad NPTH when possible: when drill size and shape == pad size and shape
# usually sel to True for copper layers
popt.SetSkipPlotNPTH_Pads( False );


#prepare the gerber job file
jobfile_writer = GERBER_JOBFILE_WRITER( board )

# Once the defaults are set it become pretty easy...
# I have a Turing-complete programming language here: I'll use it...
# param 0 is a string added to the file base name to identify the drawing
# param 1 is the layer ID
# param 2 is a comment
plot_plan = [
    ( "CuTop", F_Cu, "Top layer" ),
    ( "In1Top", In1_Cu, "In1 layer" ),
    ( "CuBottom", B_Cu, "Bottom layer" ),
    ( "PasteBottom", B_Paste, "Paste Bottom" ),
    ( "PasteTop", F_Paste, "Paste top" ),
    ( "SilkTop", F_SilkS, "Silk top" ),
    ( "SilkBottom", B_SilkS, "Silk top" ),
    ( "MaskBottom", B_Mask, "Mask bottom" ),
    ( "MaskTop", F_Mask, "Mask top" ),
    ( "EdgeCuts", Edge_Cuts, "Edges" ),
]


# In Gerber format, Set layer before calling OpenPlotfile is mandatory to generate
# the right Gerber file header.
for layer_info in plot_plan:
    if layer_info[1] <= B_Cu:
        popt.SetSkipPlotNPTH_Pads( True )
    else:
        popt.SetSkipPlotNPTH_Pads( False )

    pctl.SetLayer(layer_info[1])
    pctl.OpenPlotfile(layer_info[0], PLOT_FORMAT_GERBER, layer_info[2])

    print( 'plot %s' % fromUTF8Text( pctl.GetPlotFileName() ) )

    if gen_job_file == True:
        jobfile_writer.AddGbrFile( layer_info[1], os.path.basename(pctl.GetPlotFileName()) );
    if pctl.PlotLayer() == False:
        print( "plot error" )

#generate internal copper layers, if any
lyrcnt = board.GetCopperLayerCount();

for innerlyr in range ( 1, lyrcnt-1 ):
    popt.SetSkipPlotNPTH_Pads( True );
    pctl.SetLayer(innerlyr)
    lyrname = 'inner%s' % innerlyr
    pctl.OpenPlotfile(lyrname, PLOT_FORMAT_GERBER, "inner")

    print( "plot %s" % fromUTF8Text( pctl.GetPlotFileName() ) )

    if pctl.PlotLayer() == False:
        print( "plot error" )


# At the end you have to close the last plot, otherwise you don't know when
# the object will be recycled!
pctl.ClosePlot()

# Fabricators need drill files.
# sometimes a drill map file is asked (for verification purpose)
drlwriter = EXCELLON_WRITER( board )
drlwriter.SetMapFileFormat( PLOT_FORMAT_PDF )

mirror = False
minimalHeader = False
offset = VECTOR2I(0,0)
# False to generate 2 separate drill files (one for plated holes, one for non plated holes)
# True to generate only one drill file
mergeNPTH = False
drlwriter.SetOptions( mirror, minimalHeader, offset, mergeNPTH )

metricFmt = True
drlwriter.SetFormat( metricFmt )

genDrl = True
genMap = True
print( 'create drill and map files in %s' % fromUTF8Text( pctl.GetPlotFileName() ) )
drlwriter.CreateDrillandMapFilesSet( pctl.GetPlotDirName(), genDrl, genMap );

# One can create a text file to report drill statistics
rptfn = pctl.GetPlotDirName() + 'drill_report.rpt'
print( 'report: %s' % fromUTF8Text( rptfn ) )
drlwriter.GenDrillReportFile( rptfn );

if gen_job_file == True:
    #job_fn=os.path.splitext(pctl.GetPlotFileName())[0] + '.gbrjob'
    job_fn=os.path.dirname(pctl.GetPlotFileName()) + '/' + os.path.basename(filename)
    job_fn=os.path.splitext(job_fn)[0] + '.gbrjob'

    print( 'create job file %s ' % fromUTF8Text( job_fn ) )

    jobfile_writer.CreateJobFile( job_fn )
