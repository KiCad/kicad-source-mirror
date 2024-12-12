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

Multi-constraint panels like Via Style (via_diameter + hole_size) and
Differential Pair Routing (track_width + diff_pair_gap) are matched first.
Single-constraint panels handle the remainder. Unmapped constraints appear
in the Custom Rule panel.

Rules that are not modified in the graphical editor preserve their original
text formatting when saved.
<br><br><br>

