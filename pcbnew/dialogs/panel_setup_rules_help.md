### Top-level Clauses

    (version <number>)

    (rule <rule_name> <rule_clause> ...)


<br>

### Rule Clauses

    (constraint <constraint_type> ...)

    (condition "<expression>")

    (layer "<layer_name>")

    (severity <severity_name>)


<br>

### Constraint Types

 * annular\_width
 * assertion
 * clearance
 * connection\_width
 * courtyard_clearance
 * diff\_pair\_gap
 * diff\_pair\_uncoupled
 * disallow
 * edge\_clearance
 * length
 * hole\_clearance
 * hole\_size
 * min\_resolved\_spokes
 * physical\_clearance
 * physical\_hole\_clearance
 * silk\_clearance
 * skew
 * text\_height
 * text\_thickness
 * thermal\_relief\_gap
 * thermal\_spoke\_width
 * track\_width
 * via\_count
 * via\_diameter
 * zone\_connection

Note: `clearance` and `hole_clearance` rules are not run against items of the same net; `physical_clearance` and `physical_hole_clearance` rules are.
<br><br>

### Items

 * `A` &nbsp;&nbsp; _the first (or only) item under test_
 * `B` &nbsp;&nbsp; _the second item under test (for binary tests)_
 * `L` &nbsp;&nbsp; _the layer currently under test_

<br>

### Item Types

 * buried\_via
 * graphic
 * hole
 * micro\_via
 * pad
 * text
 * track
 * via
 * zone

<br>

### Zone Connections

 * solid
 * thermal\_reliefs
 * none

<br>

### Severity Names

 * warning
 * error
 * exclusion
 * ignore

<br>

### Examples

    (version 1)

    (rule HV
       (constraint clearance (min 1.5mm))
       (condition "A.NetClass == 'HV'"))


    (rule HV
       (layer outer)
       (constraint clearance (min 1.5mm))
       (condition "A.NetClass == 'HV'"))


    (rule HV_HV
       # wider clearance between HV tracks
       (constraint clearance (min "1.5mm + 2.0mm"))
       (condition "A.NetClass == 'HV' && B.NetClass == 'HV'"))


    (rule HV_unshielded
       (constraint clearance (min 2mm))
       (condition "A.NetClass == 'HV' && !A.enclosedByArea('Shield*')"))


    (rule heavy_thermals
       (constraint thermal_spoke_width (min 0.5mm))
       (condition "A.NetClass == 'HV'"))
<br><br>

### Notes

Version clause must be the first clause.  It indicates the syntax version of the file so that 
future rules parsers can perform automatic updates.  It should be
set to "1".

Rules should be ordered by specificity.  Later rules take
precedence over earlier rules; once a matching rule is found
no further rules will be checked.

Use Ctrl+/ to comment or uncomment line(s).
<br><br><br>

### Expression functions

All function parameters support simple wildcards (`*` and `?`).
<br><br>

    A.intersectsCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's principal courtyard.
<br><br>

    A.intersectsFrontCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's front courtyard.
<br><br>

    A.intersectsBackCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's back courtyard.
<br><br>

    A.intersectsArea('<zone_name>')
True if any part of `A` lies within the given zone's outline.
<br><br>

    A.enclosedByArea('<zone_name>')
True if all of `A` lies within the given zone's outline.  

NB: this is potentially a more expensive call than `intersectsArea()`.  Use `intersectsArea()` 
where possible.
<br><br>

    A.isPlated()
True if `A` has a hole which is plated.
<br><br>

    A.inDiffPair('<net_name>')
True if `A` has a net that is part of the specified differential pair.
`<net_name>` is the base name of the differential pair.  For example, `inDiffPair('/CLK')`
matches items in the `/CLK_P` and `/CLK_N` nets.
<br><br>

    AB.isCoupledDiffPair()
True if `A` and `B` are members of the same diff pair.
<br><br>

    A.memberOfGroup('<group_name>')
True if `A` is a member of the given group. The name can contain wildcards.
Includes nested membership.
<br><br>

    A.memberOfFootprint('<footprint_reference>')
True if `A` is a member of a footprint matching the given reference designator.  The
reference can contain wildcards.
<br><br>

    A.existsOnLayer('<layer_name>')
True if `A` exists on the given layer.  The layer name can be
either the name assigned in Board Setup > Board Editor Layers or
the canonical name (ie: `F.Cu`).

NB: this returns true if `A` is on the given layer, independently
of whether or not the rule is being evaluated for that layer.
For the latter use a `(layer "layer_name")` clause in the rule.
<br><br>

    !!! A.memberOf('<group_name>') !!!
Deprecated; use `memberOfGroup()` instead.
<br><br>

    !!! A.insideCourtyard('<footprint_refdes>') !!!
Deprecated; use `intersectsCourtyard()` instead.
<br><br>

    !!! A.insideFrontCourtyard('<footprint_refdes>') !!!
Deprecated; use `intersectsFrontCourtyard()` instead.
<br><br>

    !!! A.insideBackCourtyard('<footprint_refdes>') !!!
Deprecated; use `intersectsBackCourtyard()` instead.
<br><br>

    !!! A.insideArea('<zone_name>') !!!
Deprecated; use `intersectsArea()` instead.
<br><br><br>

### More Examples

    (rule "copper keepout"
       (constraint disallow track via zone)
       (condition "A.intersectsArea('zone3')"))


    (rule "BGA neckdown"
       (constraint track_width (min 0.2mm) (opt 0.25mm))
       (constraint clearance (min 0.05mm) (opt 0.08mm))
       (condition "A.intersectsCourtyard('U3')"))


    # prevent silk over tented vias
    (rule silk_over_via
       (constraint silk_clearance (min 0.2mm))
       (condition "A.Type == '*Text' && B.Type == 'Via'"))


    (rule "Distance between Vias of Different Nets"
        (constraint hole_to_hole (min 0.254mm))
        (condition "A.Type == 'Via' && B.Type == 'Via' && A.Net != B.Net"))

    (rule "Clearance between Pads of Different Nets"
        (constraint clearance (min 3.0mm))
        (condition "A.Type == 'Pad' && B.Type == 'Pad' && A.Net != B.Net"))


    (rule "Via Hole to Track Clearance"
        (constraint hole_clearance (min 0.254mm))
        (condition "A.Type == 'Via' && B.Type == 'Track'"))

    (rule "Pad to Track Clearance"
        (constraint clearance (min 0.2mm))
        (condition "A.Type == 'Pad' && B.Type == 'Track'"))


    (rule "clearance-to-1mm-cutout"
        (constraint clearance (min 0.8mm))
        (condition "A.Layer == 'Edge.Cuts' && A.Thickness == 1.0mm"))


    (rule "Max Drill Hole Size Mechanical"
        (constraint hole_size (max 6.3mm))
        (condition "A.Pad_Type == 'NPTH, mechanical'"))

    (rule "Max Drill Hole Size PTH"
        (constraint hole_size (max 6.35mm))
        (condition "A.Pad_Type == 'Through-hole'"))


    # Specify an optimal gap for a particular diff-pair
    (rule "dp clock gap"
        (constraint diff_pair_gap (opt "0.8mm"))
        (condition "A.inDiffPair('/CLK')"))

    # Specify a larger clearance around any diff-pair
    (rule "dp clearance"
        (constraint clearance (min "1.5mm"))
        (condition "A.inDiffPair('*') && !AB.isCoupledDiffPair()"))


    # Don't use thermal reliefs on heatsink pads
    (rule heat_sink_pad
        (constraint zone_connection solid)
        (condition "A.Fabrication_Property == 'Heatsink pad'"))

    # Require all four thermal relief spokes to connect to parent zone
    (rule fully_spoked_pads
        (constraint min_resolved_spokes 4))

    # Set thermal relief gap & spoke width for all zones
    (rule defined_relief
        (constraint thermal_relief_gap (min 10mil))
        (constraint thermal_spoke_width (min 12mil)))

    # Override thermal relief gap & spoke width for GND and PWR zones
    (rule defined_relief_pwr
        (constraint thermal_relief_gap (min 10mil))
        (constraint thermal_spoke_width (min 12mil))
        (condition "A.Name == 'zone_GND' || A.Name == 'zone_PWR'"))


    # Prevent solder wicking from SMD pads
    (rule holes_in_pads
        (constraint physical_hole_clearance (min 0.2mm))
        (condition "B.Pad_Type == 'SMD'"))

    # Disallow solder mask margin overrides
    (rule "disallow solder mask margin overrides"
        (constraint assertion "A.Soldermask_Margin_Override == 0mm")
        (condition "A.Type == 'Pad'"))


    # Enforce a mechanical clearance between components and board edge
    (rule front_mechanical_board_edge_clearance
        (layer "F.Courtyard")
        (constraint physical_clearance (min 3mm))
        (condition "B.Layer == 'Edge.Cuts'"))


    # Check current-carrying capacity
    (rule high-current
        (constraint track_width (min 1.0mm))
        (constraint connection_width (min 0.8mm))
        (condition "A.NetClass == 'Power'"))

### Documentation

For the full documentation see [https://docs.kicad.org](https://docs.kicad.org/GetMajorMinorVersion/en/pcbnew/pcbnew.html#custom_design_rules).