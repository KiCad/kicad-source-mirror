import subprocess
import json
import re
import os
import sys

# --- Tree Generation Logic ---

KICAD_CLI = "kicad-cli"
ENV = os.environ.copy()
ENV["KICAD_RUN_FROM_BUILD_DIR"] = "1"

def get_help(args):
    cmd = [KICAD_CLI] + args + ["-h"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, env=ENV)
        return result.stdout
    except Exception as e:
        return ""

def parse_help(help_text):
    subcommands = {}
    options = {}

    lines = help_text.split('\n')
    section = None

    for line in lines:
        line = line.strip()
        if not line:
            continue

        if line.startswith("Subcommands:"):
            section = "subcommands"
            continue
        elif line.startswith("Optional arguments:") or line.startswith("Arguments:"):
            section = "options"
            continue

        if section == "subcommands":
            match = re.match(r'^([a-z0-9_-]+)\s+(.*)$', line)
            if match:
                subcommands[match.group(1)] = match.group(2).strip()

        if section == "options":
            parts = re.split(r'\s{2,}', line, maxsplit=1)
            flags_part = parts[0]
            description = parts[1] if len(parts) > 1 else ""

            flags = re.findall(r'(-[a-zA-Z0-9]|--[a-zA-Z0-9_-]+)', flags_part)
            for flag in flags:
                options[flag] = description

    return subcommands, options

def explore(path):
    sys.stderr.write(f"Exploring {' '.join(path)}\n")
    help_text = get_help(path)
    subs_map, opts_map = parse_help(help_text)

    tree = {
        "options": opts_map,
        "subcommands": {},
        "subcommand_descriptions": subs_map
    }

    for sub in subs_map.keys():
        tree["subcommands"][sub] = explore(path + [sub])

    return tree

# --- Zsh Completion Generation Logic ---

def escape_desc(desc):
    return desc.replace("'", "'\\''").replace("[", "\\[").replace("]", "\\]")

def generate_zsh_completion(tree):
    lines = []
    lines.append("#compdef kicad-cli")
    lines.append("")

    # Helper to generate function name from path
    def get_func_name(path):
        if not path:
            return "_kicad_cli"
        return "_kicad_cli_" + "_".join(path).replace("-", "_")

    all_functions = []

    def visit(node, path):
        func_name = get_func_name(path)

        current_func_lines = []
        current_func_lines.append(f"{func_name}() {{")
        current_func_lines.append("    local context state state_descr line")
        current_func_lines.append("    typeset -A opt_args")
        current_func_lines.append("")

        # Prepare arguments list
        args = []

        # Add options
        opts = node.get("options", {})
        processed_opts = set()
        sorted_opts = sorted(opts.keys())

        for opt in sorted_opts:
            if opt in processed_opts:
                continue

            desc = opts[opt]
            pair = []
            pair.append(opt)
            processed_opts.add(opt)

            for other_opt in sorted_opts:
                if other_opt == opt: continue
                if other_opt in processed_opts: continue

                if opts[other_opt] == desc:
                    pair.append(other_opt)
                    processed_opts.add(other_opt)

            desc_escaped = escape_desc(desc)

            if len(pair) > 1:
                exclusion = " ".join(pair)
                brace = ",".join(pair)
                args.append(f"        '({exclusion})'{{{brace}}}'[{desc_escaped}]' \\")
            else:
                args.append(f"        '{opt}[{desc_escaped}]' \\")

        subcommands = node.get("subcommands", {})
        sub_descs = node.get("subcommand_descriptions", {})

        sub_cmds_func = func_name + "_commands"

        if subcommands:
            args.append(f"        '1: :{sub_cmds_func}' \\")
            args.append("        '*:: :->args' \\")

        current_func_lines.append("    _arguments -C \\")
        for arg in args:
            current_func_lines.append(arg)
        current_func_lines.append("        && return 0")

        if subcommands:
            current_func_lines.append("")
            current_func_lines.append("    case $state in")
            current_func_lines.append("        (args)")
            current_func_lines.append("            case $words[1] in")

            for sub in sorted(subcommands.keys()):
                sub_func = get_func_name(path + [sub])
                current_func_lines.append(f"                ({sub})")
                current_func_lines.append(f"                    {sub_func}")
                current_func_lines.append("                    ;;")

            current_func_lines.append("            esac")
            current_func_lines.append("            ;;")
            current_func_lines.append("    esac")

            # Define the aux function for subcommands
            aux_lines = []
            aux_lines.append(f"{sub_cmds_func}() {{")
            aux_lines.append("    local -a commands")
            aux_lines.append("    commands=(")
            for sub in sorted(subcommands.keys()):
                desc = sub_descs.get(sub, "")
                desc_escaped = escape_desc(desc)
                aux_lines.append(f"        '{sub}:{desc_escaped}'")
            aux_lines.append("    )")
            aux_lines.append("    _describe -t commands 'command' commands")
            aux_lines.append("}")

            # Add aux function to global list
            all_functions.append("\n".join(aux_lines))

        current_func_lines.append("}")
        current_func_lines.append("")

        # Add current function to global list
        all_functions.append("\n".join(current_func_lines))

        # Recurse
        for sub in sorted(subcommands.keys()):
            visit(subcommands[sub], path + [sub])

    visit(tree, [])

    lines.extend(all_functions)

    lines.append("")
    lines.append("if type compdef >/dev/null 2>&1; then")
    lines.append("    compdef _kicad_cli kicad-cli")
    lines.append("fi")

    return "\n".join(lines)

if __name__ == "__main__":
    tree = explore([])
    print(generate_zsh_completion(tree))
