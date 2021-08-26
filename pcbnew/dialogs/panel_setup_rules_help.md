### Top-level Clauses

    (version <number>)

    (rule <rule_name> <rule_clause> ...)


<br><br>

### Rule Clauses

    (constraint <constraint_type> ...)

    (condition "<expression>")

    (layer "<layer_name>")


<br><br>

### Constraint Types

 * annular\_width
 * clearance
 * courtyard_clearance
 * diff\_pair\_gap
 * diff\_pair\_uncoupled
 * disallow
 * edge\_clearance
 * length
 * hole\_clearance
 * hole\_size
 * silk\_clearance
 * skew
 * track\_width
 * via\_count
 * via\_diameter


<br><br>

### Item Types

 * buried_via
 * graphic
 * hole
 * micro_via
 * pad
 * text
 * track
 * via
 * zone

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
       (condition "A.NetClass == 'HV' && !A.insideArea('Shield*')"))
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

    A.insideCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's principal courtyard.
<br><br>

    A.insideFrontCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's front courtyard.
<br><br>

    A.insideBackCourtyard('<footprint_refdes>')
True if any part of `A` lies within the given footprint's back courtyard.
<br><br>

    A.insideArea('<zone_name>')
True if any part of `A` lies within the given zone's outline.
<br><br>

    A.isPlated()
True if `A` has a hole which is plated.
<br><br>

    A.inDiffPair('<net_name>')
True if `A` has net that is part of the specified differential pair.
`<net_name>` is the base name of the differential pair.  For example, `inDiffPair('CLK')`
matches items in the `CLK_P` and `CLK_N` nets.
<br><br>

    AB.isCoupledDiffPair()
True if `A` and `B` are members of the same diff pair.
<br><br>

    A.memberOf('<group_name>')
True if `A` is a member of the given group. Includes nested membership.
<br><br>

    A.existsOnLayer('<layer_name>')
True if `A` exists on the given layer.  The layer name can be
either the name assigned in Board Setup > Board Editor Layers or
the canonical name (ie: `F.Cu`).

NB: this returns true if `A` is on the given layer, independently
of whether or not the rule is being evaluated for that layer.
For the latter use a `(layer "layer_name")` clause in the rule.
<br><br><br>

### More Examples

    (rule "copper keepout"
       (constraint disallow track via zone)
       (condition "A.insideArea('zone3')"))


    (rule "BGA neckdown"
       (constraint track_width (min 0.2mm) (opt 0.25mm))
       (constraint clearance (min 0.05mm) (opt 0.08mm))
       (condition "A.insideCourtyard('U3')"))


    # prevent silk over tented vias
    (rule silk_over_via
       (constraint silk_clearance (min 0.2mm))
       (condition "A.Type == '*Text' && B.Type == 'Via'"))


    (rule "Distance between Vias of Different Nets"  
        (constraint hole_to_hole (min 0.254mm))
        (condition "A.Type =='Via' && B.Type =='Via' && A.Net != B.Net"))

    (rule "Clearance between Pads of Different Nets"  
        (constraint clearance (min 3.0mm))
        (condition "A.Type =='Pad' && B.Type =='Pad' && A.Net != B.Net"))


    (rule "Via Hole to Track Clearance"  
        (constraint hole_clearance (min 0.254mm))
        (condition "A.Type =='Via' && B.Type =='Track'"))
    
    (rule "Pad to Track Clearance"  
        (constraint clearance (min 0.2mm))
        (condition "A.Type =='Pad' && B.Type =='Track'"))


    (rule "clearance-to-1mm-cutout"
        (constraint clearance (min 0.8mm))
        (condition "A.Layer=='Edge.Cuts' && A.Thickness == 1.0mm"))


    (rule "Max Drill Hole Size Mechanical"  
        (constraint hole_size (max 6.3mm))
        (condition "A.Pad_Type == 'NPTH, mechanical'"))
    
    (rule "Max Drill Hole Size PTH"  
        (constraint hole_size (max 6.35mm))
        (condition "A.Pad_Type == 'Through-hole'"))


    # Specify an optimal gap for a particular diff-pair
    (rule "dp clock gap"
        (constraint diff_pair_gap (opt "0.8mm"))
        (condition "A.inDiffPair('CLK') && AB.isCoupledDiffPair()"))

    # Specify a larger clearance around any diff-pair
    (rule "dp clearance"
        (constraint clearance (min "1.5mm"))
        (condition "A.inDiffPair('*') && !AB.isCoupledDiffPair()"))
