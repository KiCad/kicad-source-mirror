#!/usr/bin/env python3
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""
Regenerate include/drc_rules_release_vocabulary.h from the release tags.

Saving a project for an older KiCad drops any custom rule that release could not
parse. The vocabulary of a release series is read straight from its tags and is
the words every point release in the series understood, so it is a record of
what shipped rather than something kept up by hand.

Run from the repository root with the release tags fetched:

    python3 tools/generate_drc_rules_vocabulary.py

Add a target here when a release becomes a downgrade target. A new point release
only changes the result if it stopped reading a word.
"""

import re
import subprocess

# Downgrade target id, and the tag prefix shared by every release in that series.
TARGETS = [
    ("9.0", "9.0."),
    ("10.0", "10.0."),
]

OUTPUT = "include/drc_rules_release_vocabulary.h"

KEYWORDS = "common/drc_rules.keywords"
FUNCTIONS = "pcbnew/pcbexpr_functions.cpp"
EVALUATOR = "pcbnew/pcbexpr_evaluator.cpp"

# A property is constructed as new PROPERTY<CLASS, TYPE[, BASE]>( _HKI( "Name" ), ... ), with
# or without an AddProperty around it. The kind and the type decide whether a rule can read it.
PROPERTY = re.compile(
    r'new\s+(PROPERTY[A-Z_]*)<\s*[A-Za-z_0-9:]+\s*,\s*((?:[^,<>]|<[^<>]*>)+?)\s*[,>][^;]*?_HKI\(\s*"([^"]+)"', re.S)
# The evaluator names the value types it can read, one TYPE_HASH each. Enum properties are
# read through their choice list instead, so every PROPERTY_ENUM counts as readable.
VALUE_TYPE = re.compile(r'TYPE_HASH\(\s*([^)]+?)\s*\)')
# A value's unit suffix is whatever the unit resolver lists, and ${...} text variables are
# accepted only once the expression tokenizer looks for them.
UNIT = re.compile(r'"([a-z%]+)"')
TOKENIZER = "common/libeval_compiler/libeval_compiler.cpp"
FUNCTION = re.compile(r'RegisterFunc\(\s*wxT\(\s*"([^"(]+)\(')
# The evaluator resolves a few names itself before asking the property manager,
# and keeps aliases for properties that were renamed.
SPECIAL = re.compile(r'aField\.CmpNoCase\(\s*wxT\(\s*"([A-Za-z_]+)"')
ALIAS = re.compile(r'field\.CmpNoCase\(\s*wxT\(\s*"([^"]+)"')


def git(*args):
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True).stdout


def show(rev, path):
    return git("show", f"{rev}:{path}")


def release_tags(prefix):
    tags = [tag for tag in git("tag", "-l", prefix + "*").split() if "-" not in tag]

    if not tags:
        raise SystemExit(f"no release tags match {prefix}*, fetch the tags first")

    return sorted(tags, key=lambda tag: [int(part) for part in tag.split(".")])


IDENTIFIER = re.compile(r"^[a-z_][a-z0-9_]*$")


def normalise(name):
    # Match the evaluator, which swaps underscores for spaces and compares without case.
    return name.strip().lower().replace(" ", "_")


def squash(text):
    return re.sub(r"\s+", "", text)


def referenceable(name):
    # A display name with a hyphen or a percent sign cannot be reached from an expression,
    # where those are operators, so it is not part of the vocabulary.
    return IDENTIFIER.match(name) is not None


def vocabulary(rev):
    keywords = {line.strip() for line in show(rev, KEYWORDS).splitlines()
                if line.strip() and not line.startswith("#")}

    functions = {name.lower() for name in FUNCTION.findall(show(rev, FUNCTIONS))}

    evaluator = show(rev, EVALUATOR)
    readable = {squash(t) for t in VALUE_TYPE.findall(evaluator)}

    properties = set()
    sources = [f for f in git("ls-tree", "-r", "--name-only", rev, "pcbnew", "common").split()
               if f.endswith(".cpp")]

    for path in sources:
        for kind, value_type, name in PROPERTY.findall(show(rev, path)):
            if kind != "PROPERTY_ENUM" and squash(value_type) not in readable:
                continue

            if referenceable(normalise(name)):
                properties.add(normalise(name))

    properties |= {name.lower() for name in SPECIAL.findall(evaluator)}
    properties |= {normalise(name) for name in ALIAS.findall(evaluator)}

    resolver = re.search(r"GetSupportedUnits\(\)(.*?)\n}", evaluator, re.S)
    units = set(UNIT.findall(resolver.group(1))) if resolver else set()
    text_variables = 'MatchAhead( "${"' in show(rev, TOKENIZER)

    return keywords, functions, properties, units, text_variables


def shared_vocabulary(tags):
    keywords, functions, properties, units, text_variables = vocabulary(tags[0])

    for tag in tags[1:]:
        k, f, p, u, t = vocabulary(tag)
        keywords &= k
        functions &= f
        properties &= p
        units &= u
        text_variables = text_variables and t

    return sorted(keywords), sorted(functions), sorted(properties), sorted(units), text_variables


def emit_array(out, name, values):
    out.append(f"static constexpr const char* const {name}[] = {{")

    for value in values:
        out.append(f'    "{value}",')

    out.append("};")
    out.append("")


def main():
    out = [
        "// This file is generated by tools/generate_drc_rules_vocabulary.py from the release branches.",
        "// Do not edit it by hand. Rerun the script when a release becomes a downgrade target.",
        "",
        "#ifndef DRC_RULES_RELEASE_VOCABULARY_H",
        "#define DRC_RULES_RELEASE_VOCABULARY_H",
        "",
        "#include <cstddef>",
        "#include <iterator>",
        "",
        "/// Everything a rules file could name in one release. Tokens are the rule grammar, the",
        "/// rest is what a condition or assertion may call or read.",
        "struct DRC_RULE_VOCABULARY",
        "{",
        "    const char*        m_targetId;",
        "    const char* const* m_tokens;",
        "    size_t             m_tokenCount;",
        "    const char* const* m_functions;",
        "    size_t             m_functionCount;",
        "    const char* const* m_properties;",
        "    size_t             m_propertyCount;",
        "    const char* const* m_units;",
        "    size_t             m_unitCount;",
        "    bool               m_textVariables;",
        "};",
        "",
    ]

    entries = []

    for target_id, prefix in TARGETS:
        tag = "KICAD" + target_id.replace(".", "_")
        tags = release_tags(prefix)
        keywords, functions, properties, units, text_variables = shared_vocabulary(tags)

        out.append(f"// KiCad {target_id}, the words every release from {tags[0]} to {tags[-1]} reads.")
        emit_array(out, f"{tag}_DRC_RULE_TOKENS", keywords)
        emit_array(out, f"{tag}_DRC_RULE_FUNCTIONS", functions)
        emit_array(out, f"{tag}_DRC_RULE_PROPERTIES", properties)
        emit_array(out, f"{tag}_DRC_RULE_UNITS", units)

        # One field per line, so the committed file survives clang-format unchanged.
        entries.append(
            "    {\n"
            f'        "{target_id}",\n'
            f"        {tag}_DRC_RULE_TOKENS,\n"
            f"        std::size( {tag}_DRC_RULE_TOKENS ),\n"
            f"        {tag}_DRC_RULE_FUNCTIONS,\n"
            f"        std::size( {tag}_DRC_RULE_FUNCTIONS ),\n"
            f"        {tag}_DRC_RULE_PROPERTIES,\n"
            f"        std::size( {tag}_DRC_RULE_PROPERTIES ),\n"
            f"        {tag}_DRC_RULE_UNITS,\n"
            f"        std::size( {tag}_DRC_RULE_UNITS ),\n"
            f"        {'true' if text_variables else 'false'},\n"
            "    },")

        print(f"{target_id}: {len(keywords)} tokens, {len(functions)} functions, "
              f"{len(properties)} properties, units {' '.join(units)}, "
              f"text variables {'yes' if text_variables else 'no'}")

    out.append("static constexpr DRC_RULE_VOCABULARY DRC_RULE_VOCABULARIES[] = {")
    out.extend(entries)
    out.append("};")
    out.append("")
    out.append("#endif // DRC_RULES_RELEASE_VOCABULARY_H")

    with open(OUTPUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")

    print(f"wrote {OUTPUT}")


if __name__ == "__main__":
    main()
