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

### Constraints

| Constraint type           | Argument type                                                                                                         | Description                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|---------------------------|-----------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `annular_width`           | min/opt/max                                                                                                           | Checks the width of annular rings on vias.<br>                                                                                                                                                                                                                                                                                                                                                                                                         |
| `assertion`               | "&lt;expression>"                                                                                                     | Checks the given expression.<br>                                                                                                                                                                                                                                                                                                                                                                                                  |
| `clearance`               | min                                                                                                                   | Specifies the **electrical** clearance between copper objects of different nets.  (See `physical_clearance` if you wish to specify clearance between objects regardless of net.)<br><br>To allow copper objects to overlap (collide), create a `clearance` constraint with the `min` value less than zero (for example, `-1`).<br>                                                                                                                     |
| `courtyard_clearance`     | min                                                                                                                   | Checks the clearance between footprint courtyards and generates an error if any two courtyards are closer than the `min` distance.  If a footprint does not have a courtyard shape, no errors will be generated from this constraint.<br>                                                                                                                                                                                                              |
| `diff_pair_gap`           | min/opt/max                                                                                                           | Checks the gap between coupled tracks in a differential pair.  Coupled tracks are segments that are parallel to each other.  Differential pair gap is not tested on uncoupled portions of a differential pair (for example, the fanout from a component).<br>                                                                                                                                                                                          |
| `diff_pair_uncoupled`     | max                                                                                                                   | Checks the distance that a differential pair track is routed uncoupled from the other polarity track in the pair (for example, where the pair fans out from a component, or becomes uncoupled to pass around another object such as a via).<br>                                                                                                                                                                                                        |
| `disallow`                | `track`<br>`via`<br>`micro_via`<br>`buried_via`<br>`pad`<br>`zone`<br>`text`<br>`graphic`<br>`hole`<br>`footprint`<br> | Specify one or more object types to disallow, separated by spaces.  For example, `(constraint disallow track)` or `(constraint disallow track via pad)`.  If an object of this type matches the rule condition, a DRC error will be created.<br><br>This constraint is essentially the same as a keepout rule area, but can be used to create more specific keepout restrictions.<br>                                                                  |
| `edge_clearance`          | min/opt/max                                                                                                           | Checks the clearance between objects and the board edge.<br><br>This can also be thought of as the "milling tolerance" as the board edge will include all graphical items on the `Edge.Cuts` layer as well as any *oval* pad holes.  (See `physical_hole_clearance` for the drilling tolerance.)<br>                                                                                                                                                   |
| `length`                  | min/max                                                                                                               | Checks the total routed length for the nets that match the rule condition and generates an error for each net that is below the `min` value (if specified) or above the `max` value (if specified) of the constraint.<br>                                                                                                                                                                                                                              |
| `hole`                    | min/max                                                                                                               | Checks the size (diameter) of a drilled hole in a pad or via.  For oval holes, the smaller (minor) diameter will be tested against the `min` value (if specified) and the larger (major) diameter will be tested against the `max` value (if specified).<br>                                                                                                                                                                                           |
| `hole_clearance`          | min                                                                                                                   | Checks the clearance between a drilled hole in a pad or via and copper objects on a different net.  The clearance is measured from the diameter of the hole, not its center.<br>                                                                                                                                                                                                                                                                       |
| `hole_to_hole`            | min                                                                                                                   | Checks the clearance between mechanically-drilled holes in pads and vias.  The clearance is measured between the diameters of the holes, not between their centers.<br><br>This constraint is soley for the protection of drill bits.  The clearance between **laser-drilled** (microvias) and other non-mechanically-drilled holes is not checked, nor is the clearance between **milled** (oval-shaped) and other non-mechanically-drilled holes.<br> |
| `physical_clearance`      | min                                                                                                                   | Checks the clearance between two objects on a given layer (including non-copper layers).<br><br>While this can perform more general-purpose checks than `clearance`, it is much slower.  Use `clearance` where possible.<br>                                                                                                                                                                                                                           |
| `physical_hole_clearance` | min                                                                                                                   | Checks the clearance between a drilled hole in a pad or via and another object, regardless of net. The clearance is measured from the diameter of the hole, not its center.<br><br>This can also be thought of as the "drilling tolerance" as it only includes **round** holes (see `edge_clearance` for the milling tolerance).<br>                                                                                                                   |
| `silk_clearance`          | min/opt/max                                                                                                           | Checks the clearance between objects on silkscreen layers and other objects.<br>                                                                                                                                                                                                                                                                                                                                                                       |
| `skew`                    | max                                                                                                                   | Checks the total skew for the nets that match the rule condition, that is, the difference between the length of each net and the average of all the lengths of each net that is matched by the rule.  If the absolute value of the difference between that average and the length of any one net is above the constraint `max` value, an error will be generated.<br>                                                                                  |
| `thermal_relief_gap`      | min                                                                                                                   | Specifies the width of the gap between a pad and a zone with a thermal-relief connection.<br>                                                                                                                                                                                                                                                                                                                                                          |
| `thermal_spoke_width`     | opt                                                                                                                   | Specifies the width of the spokes connecting a pad to a zone with a thermal-relief connection.<br>                                                                                                                                                                                                                                                                                                                                                     |
| `track_width`             | min/opt/max                                                                                                           | Checks the width of track and arc segments.  An error will be generated for each segment that has a width below the `min` value (if specified) or above the `max` value (if specified).<br>                                                                                                                                                                                                                                                            |
| `via_count`               | max                                                                                                                   | Counts the number of vias on every net matched by the rule condition.  If that number exceeds the constraint `max` value on any matched net, an error will be generated for that net.<br>                                                                                                                                                                                                                                                              |
| `zone_connection`         | `solid`<br>`thermal_reliefs`<br>`none`                                                                                | Specifies the connection to be made between a zone and a pad.<br>                                                                                                                                                                                                                                                                                                                                                                                      |


### Items

 * `A` &nbsp;&nbsp; _the first (or only) item under test_
 * `B` &nbsp;&nbsp; _the second item under test (for binary tests)_
 * `L` &nbsp;&nbsp; _the layer currently under test_

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

    A.getField('<field_name>')
The value of the given field. Only footprints have fields, so a field is only returned if
`A` is a footprint.
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

    A.memberOfFootprint('<footprint_reference>|<footprint_id>')
True if `A` is a member of a footprint matching the given reference designator or footprint
ID.  The parameter can contain wildcards.

NB: If matching against a footprint ID is desired, the parameter must contain a ':'.
<br><br>

    A.memberOfSheet('<sheet_path>')
True if `A` is a member of the given schematic sheet. The sheet path can contain wildcards.
<br><br>

    A.existsOnLayer('<layer_name>')
True if `A` exists on the given layer.  The layer name can be
either the name assigned in Board Setup > Board Editor Layers or
the canonical name (ie: `F.Cu`).

NB: this returns true if `A` is on the given layer, independently
of whether or not the rule is being evaluated for that layer.
For the latter use a `(layer "layer_name")` clause in the rule.
<br><br>

    A.hasNetclass('<netclass_name>')
True if `A` has had the given netclass assigned to it, either by an explicit netclass label 
or through a pattern match assignment.
<br><br>

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
        (constraint edge_clearance (min 0.8mm))
        (condition "A.Layer == 'Edge.Cuts' && A.Line_Width == 1.0mm"))


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

    # Prevent copper fills under the courtyards of capacitors
    (rule no_copper_under_caps
        (constraint physical_clearance (min 0mm))
        (condition "A.Type == 'Zone' && B.Reference == 'C*'"))


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


    # Separate drill bit and milling cutter size constraints
    (rule "Plated through-hole size"
        (constraint hole_size (min 0.2mm) (max 6.35mm))
        (condition "A.isPlated() && A.Hole_Size_X == A.Hole_Size_Y"))
    (rule "Plated slot size"
        (constraint hole_size (min 0.5mm))
        (condition "A.isPlated() && A.Hole_Size_X != A.Hole_Size_Y"))


    # Allow blind/buried to micro-via hole-to-hole violations when it is known that
    # the fab will mechanically drill blind/buried via holes -before- laser drilling 
    # micro-vias.
    (rule hole_to_hole_uvia_exclusion
        (condition "A.Via_Type == 'Blind/buried' && B.Via_Type == 'Micro'")
        (constraint hole_to_hole)
        (severity ignore))

### Documentation

For the full documentation see [https://docs.kicad.org](https://docs.kicad.org/GetMajorMinorVersion/en/pcbnew/pcbnew.html#custom_design_rules).