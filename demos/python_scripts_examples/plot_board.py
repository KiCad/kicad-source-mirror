'''
    A python script example to create various plot files from a board:
    Fab files
    Doc files
    Gerber files

    Important note:
        this python script does not plot frame references.
        the reason is it is not yet possible from a python script because plotting
        plot frame references needs loading the corresponding page layout file
        (.wks file) or the default template.

        This info (the page layout template) is not stored in the board, and therefore
        not available.

        Do not try to change SetPlotFrameRef(False) to SetPlotFrameRef(true)
        the result is the pcbnew lib will crash if you try to plot
        the unknown frame references template.
'''

import sys

from pcbnew import *
filename=sys.argv[1]

board = LoadBoard(filename)

pctl = PLOT_CONTROLLER(board)

popt = pctl.GetPlotOptions()

popt.SetOutputDirectory("plot/")

# Set some important plot options:
popt.SetPlotFrameRef(False)
popt.SetLineWidth(FromMM(0.35))

popt.SetAutoScale(False)
popt.SetScale(1)
popt.SetMirror(False)
popt.SetUseGerberAttributes(True)
popt.SetExcludeEdgeLayer(False);
popt.SetScale(1)
popt.SetUseAuxOrigin(True)

# This by gerbers only (also the name is truly horrid!)
popt.SetSubtractMaskFromSilk(False)

pctl.SetLayer(F_SilkS)
pctl.OpenPlotfile("Silk", PLOT_FORMAT_PDF, "Assembly guide")
pctl.PlotLayer()

# Once the defaults are set it become pretty easy...
# I have a Turing-complete programming language here: I'll use it...
# param 0 is a string added to the file base name to identify the drawing
# param 1 is the layer ID
plot_plan = [
    ( "CuTop", F_Cu, "Top layer" ),
    ( "CuBottom", B_Cu, "Bottom layer" ),
    ( "PasteBottom", B_Paste, "Paste Bottom" ),
    ( "PasteTop", F_Paste, "Paste top" ),
    ( "SilkTop", F_SilkS, "Silk top" ),
    ( "SilkBottom", B_SilkS, "Silk top" ),
    ( "MaskBottom", B_Mask, "Mask bottom" ),
    ( "MaskTop", F_Mask, "Mask top" ),
    ( "EdgeCuts", Edge_Cuts, "Edges" ),
]


for layer_info in plot_plan:
    pctl.SetLayer(layer_info[1])
    pctl.OpenPlotfile(layer_info[0], PLOT_FORMAT_GERBER, layer_info[2])
    pctl.PlotLayer()

# Our fabricators want two additional gerbers:
# An assembly with no silk trim and all and only the references
# (you'll see that even holes have designators, obviously)
popt.SetSubtractMaskFromSilk(False)
popt.SetPlotReference(True)
popt.SetPlotValue(False)
popt.SetPlotInvisibleText(True)

pctl.SetLayer(F_SilkS)
pctl.OpenPlotfile("AssyTop", PLOT_FORMAT_PDF, "Assembly top")
pctl.PlotLayer()

# And a gerber with only the component outlines (really!)
popt.SetPlotReference(False)
popt.SetPlotInvisibleText(False)
pctl.SetLayer(F_SilkS)
pctl.OpenPlotfile("AssyOutlinesTop", PLOT_FORMAT_PDF, "Assembly outline top")
pctl.PlotLayer()

# The same could be done for the bottom side, if there were components
popt.SetUseAuxOrigin(False)

## For documentation we also want a general layout PDF
## I usually use a shell script to merge the ps files and then distill the result
## Now I can do it with a control file. As a bonus I can have references in a
## different colour, too.

popt.SetPlotReference(True)
popt.SetPlotValue(True)
popt.SetPlotInvisibleText(False)
# Remember that the frame is always in color 0 (BLACK) and should be requested
# before opening the plot
popt.SetPlotFrameRef(False)
pctl.SetLayer(Dwgs_User)

pctl.OpenPlotfile("Layout", PLOT_FORMAT_PDF, "General layout")
pctl.PlotLayer()

# Do the PCB edges in yellow
popt.SetColor(YELLOW)
pctl.SetLayer(Edge_Cuts)
pctl.PlotLayer()

## Comments in, uhmm... green
popt.SetColor(GREEN)
pctl.SetLayer(Cmts_User)
pctl.PlotLayer()

# Bottom mask as lines only, in red
#popt.SetMode(LINE)
popt.SetColor(RED)
pctl.SetLayer(B_Mask)
pctl.PlotLayer()

# Top mask as lines only, in blue
popt.SetColor(BLUE)
pctl.SetLayer(F_Mask)
pctl.PlotLayer()

# Top paste in light blue, filled
popt.SetColor(BLUE)
#popt.SetMode(FILLED)
pctl.SetLayer(F_Paste)
pctl.PlotLayer()

# Top Silk in cyan, filled, references in dark cyan
popt.SetReferenceColor(DARKCYAN)
popt.SetColor(CYAN)
pctl.SetLayer(F_SilkS)
pctl.PlotLayer()

popt.SetTextMode(PLOTTEXTMODE_STROKE)
pctl.SetLayer(F_Mask)
pctl.OpenPlotfile("Assembly", PLOT_FORMAT_SVG, "Master Assembly")
pctl.SetColorMode(True)

# We want *everything*
popt.SetPlotReference(True)
popt.SetPlotValue(True)
popt.SetPlotInvisibleText(True)

# Remember than the DXF driver assigns colours to layers. This means that
# we will be able to turn references on and off simply using their layers
# Also most of the layer are now plotted in 'line' mode, because DXF handles
# fill mode almost like sketch mode (this is to keep compatibility with
# most CAD programs; most of the advanced primitive attributes required are
# handled only by recent autocads...); also the entry level cads (qcad
# and derivatives) simply don't handle polyline widths...

# Here I'm using numbers for colors and layers, I'm too lazy too look them up:P
popt.SetReferenceColor(19)
popt.SetValueColor(21)

popt.SetColor(0)
#popt.SetMode(LINE)
pctl.SetLayer(B_SilkS)
pctl.PlotLayer()
popt.SetColor(14)
pctl.SetLayer(F_SilkS)
pctl.PlotLayer()
popt.SetColor(2)
pctl.SetLayer(B_Mask)
pctl.PlotLayer()
popt.SetColor(4)
pctl.SetLayer(F_Mask)
pctl.PlotLayer()
popt.SetColor(1)
pctl.SetLayer(B_Paste)
pctl.PlotLayer()
popt.SetColor(9)
pctl.SetLayer(F_Paste)
pctl.PlotLayer()
popt.SetColor(3)
pctl.SetLayer(Edge_Cuts)
pctl.PlotLayer()

# Export the copper layers too... exporting one of them in filled mode with
# drill marks will put the marks in the WHITE later (since it tries to blank
# the pads...); these will be obviously great reference points for snap
# and stuff in the cad. A pctl function to only plot them would be
# better anyway...

popt.SetColor(17)
#popt.SetMode(FILLED)
popt.SetDrillMarksType(PCB_PLOT_PARAMS.FULL_DRILL_SHAPE)
pctl.SetLayer(B_Cu)
pctl.PlotLayer()
popt.SetColor(20)
popt.SetDrillMarksType(PCB_PLOT_PARAMS.NO_DRILL_SHAPE)
pctl.SetLayer(F_Cu)
pctl.PlotLayer()

# At the end you have to close the last plot, otherwise you don't know when
# the object will be recycled!
pctl.ClosePlot()

# We have just generated 21 plotfiles with a single script
