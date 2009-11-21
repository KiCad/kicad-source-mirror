"<b>Guidelines:</b><br><br>\
<b>1)</b> in pcbnew: establish board perimeter.<br>\
<b>2)</b> in pcbnew: establish any zones, inclusive of net association.<br>\
<b>3)</b> in pcbnew: load in the netlist so you have all the components defined and instantiated.<br>\
<b>4)</b> in pcbnew: do the degree of component placements you are comfortable with.<br>\
It is a little easier to accurately position components in pcbnew than in freerouter, but either will work.<br>\
<b>5)</b> in pcbnew: set up the netclasses. power traces might be a little thicker.<br>\
so add a netclass called \"power\".<br>\
Make its traces thicker than what you establish for netclass \"Default\".<br>\
Set spacing and vias for each netclass.<br>\
<b>6)</b> in pcbnew: export to DSN.<br>\
<b>7)</b> load up freerouter (keep it running for any subsequent iterations of 6) through 14) here ).<br>\
<b>8)</b> in freerouter: load the project's *.dsn file.<br>\
<b>9)</b> useful, not mandatory: in freerouter: set your move snap modulus, which seems to default to 1 internal unit.<br>\
    20 mils in x and in y is about reasonable.<br>\
<b>10)</b> in freerouter: finish placing any components, you can change sides of a part here also, rotate, whatever.<br>\
<b>11)</b> in freerouter: route the board, save frequently to a *.dsn file<br>\
while routing, in case of power loss, not yet a session file but a full *.dsn file.<br>\
The full freerouter *.dsn file is a superset format,\
one that fully defines the board and can be reloaded between power outages,\
whereas the *.ses file is not a complete design,\
but with the *.brd file constitutes a full design.<br>\
<b>12)</b> in freerouter: when done, or when you want to back import, then save as a session file, *.ses.<br>\
<b>13)</b> in pcbnew: backimport the session file<br>\
<b>14)</b> in pcbnew: at this point the zones have to be refilled. One way to do that is to simply run DRC."
