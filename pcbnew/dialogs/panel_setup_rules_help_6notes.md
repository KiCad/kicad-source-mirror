### Notes

Version clause must be the first clause.  It indicates the syntax version of the file so that
future rules parsers can perform automatic updates.  It should be
set to "1".

Rules should be ordered by specificity.  Later rules take
precedence over earlier rules; once a matching rule is found
no further rules will be checked.

Use Ctrl+/ to comment or uncomment line(s).

### Graphical Rule Editor

When loading rules into the graphical editor, constraints are automatically
mapped to appropriate editing panels. Rules with multiple constraints may be
split across panels, each showing a subset of the rule's constraints.

Multi-constraint panels like Via Style (`via_diameter` + `hole_size`) and
Differential Pair Routing (`track_width` + `diff_pair_gap`) are matched first.
Single-constraint panels handle the remainder. Unmapped constraints appear
in the Custom Rule panel.

Rules that are not modified in the graphical editor preserve their original
text formatting when saved.
<br><br><br>

`net_chain_length` vs `length`:

When a net belongs to a chain and both constraints match, `net_chain_length` is checked against the trunk (the unique path between the two terminal pads) and `length` is ignored.  Stubs branching off the trunk don't count toward the trunk length; use `stub_length` to bound them.  If the chain has no terminal pads or the routed copper contains a loop, `net_chain_length` falls back to the sum of every member net plus series-passive bridging.  Nets that don't belong to a chain still see `length`.

Time-domain targets:

Both constraints accept `(time_domain yes)` to bound propagation delay instead of physical length.  For `net_chain_length` in time-domain mode the delay window applies to the chain as a whole; the tuner subtracts the routed delay from the other member nets to figure out what's left for the net being tuned.


<br><br><br>