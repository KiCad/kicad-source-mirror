### Top-level Clauses

    (version <number>)

    (rule <rule_name> <rule_clause> ...)


### Rule Clauses

    (constraint <constraint_type> ...)

    (condition "<expression>")

    (layer "<layer_name>")


### Constraint Types

 * annular_width
 * clearance
 * disallow
 * hole
 * track_width


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


### Examples

    # Comment
    
    (rule "copper keepout"
       (constraint disallow track via zone)
       (condition "A.insideArea('zone3')"))


    (rule "BGA neckdown"
       (constraint track_width (min 0.2mm) (opt 0.25mm))
       (constraint clearance (min 0.05) (opt 0.08mm))
       (condition "A.insideCourtyard('U3')"))


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
       (condition "A.NetClass == 'HV' && !A.insideArea('Shield*')))


### Notes

Version clause must be the first clause.

Rules should be ordered by specificity.  Later rules take
precedence over earlier rules; once a matching rule is found
no further rules will be checked.

Use Ctrl+/ to comment or uncomment line(s).



### Expression functions

All function parameters support simple wildcards (`*` and `?`).

True if any part of `A` lies within the given footprint's courtyard.

    A.insideCourtyard('<footprint_refdes>')


True if any part of `A` lies within the given zone's outline.

    A.insideArea('<zone_name>')


True if `A` has a hole which is plated.

    A.isPlated()


True if `A` is a member of the given group. Includes nested membership.

    A.memberOf('<group_name>')


True if `A` exists on the given layer.  The layer name can be
either the name assigned in Board Setup > Board Editor Layers or
the canonical name (ie: `F.Cu`).

    A.existsOnLayer('<layer_name>')

NB: this returns true if `A` is on the given layer, independently
of whether or not the rule is being evaluated for that layer.
For the latter use a `(layer "layer_name")` clause in the rule.
