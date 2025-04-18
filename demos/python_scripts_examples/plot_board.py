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
# One cannot plot the frame references, because the board does not know
# the frame references.
popt.SetPlotFrameRef(False)
popt.SetSketchPadLineWidth(FromMM(0.1))

popt.SetAutoScale(False)
popt.SetScale(1)
popt.SetMirror(False)
popt.SetUseGerberAttributes(True)
popt.SetScale(1)
popt.SetUseAuxOrigin(True)

# This by gerbers only (also the name is truly horrid!)
popt.SetSubtractMaskFromSilk(False) #remove solder mask from silk to be sure there is no silk on pads

#Create a pdf file of the top silk layer
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


# In Gerber format, Set layer before calling OpenPlotfile is mandatory to generate
# the right Gerber file header.
for layer_info in plot_plan:
    pctl.SetLayer(layer_info[1])
    pctl.OpenPlotfile(layer_info[0], PLOT_FORMAT_GERBER, layer_info[2])
    print ( layer_info[0] )
    pctl.PlotLayer()

# Our fabricators want two additional gerbers:
# An assembly with no silk trim and all and only the references
# (you'll see that even holes have designators, obviously)
popt.SetPlotReference(True)
popt.SetPlotValue(False)

pctl.SetLayer(F_SilkS)
pctl.OpenPlotfile("AssyTop", PLOT_FORMAT_PDF, "Assembly top")
pctl.PlotLayer()

# And a gerber with only the component outlines (really!)
popt.SetPlotReference(False)
popt.SetPlotValue(False)
pctl.SetLayer(F_SilkS)
pctl.OpenPlotfile("AssyOutlinesTop", PLOT_FORMAT_PDF, "Assembly outline top")
pctl.PlotLayer()

# The same could be done for the bottom side, if there were components
popt.SetUseAuxOrigin(False)

# For documentation we also want a general layout PDF
# I usually use a shell script to merge the ps files and then distill the result
# Now I can do it with a control file. As a bonus I can have references in a
# different colour, too.

popt.SetPlotReference(True)
popt.SetPlotValue(True)

pctl.SetLayer(Cmts_User)
pctl.PlotLayer()


# At the end you have to close the last plot, otherwise you don't know when
# the object will be recycled!
pctl.ClosePlot()

# We have just generated your plotfiles with a single script
