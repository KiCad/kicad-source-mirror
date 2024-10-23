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

