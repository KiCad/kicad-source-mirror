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

# --- Bash Completion Generation Logic ---

def generate_bash_completion(tree):
    script = []
    script.append("# kicad-cli bash completion")
    script.append("")
    script.append("_kicad_cli()")
    script.append("{")
    script.append("    local cur prev words cword")
    script.append("    if type _init_completion >/dev/null 2>&1; then")
    script.append("    _init_completion || return")
    script.append("    else")
    script.append("        COMPREPLY=()")
    script.append("        cur=\"${COMP_WORDS[COMP_CWORD]}\"")
    script.append("        prev=\"${COMP_WORDS[COMP_CWORD-1]}\"")
    script.append("        words=(\"${COMP_WORDS[@]}\")")
    script.append("        cword=$COMP_CWORD")
    script.append("    fi")
    script.append("")
    script.append("    local command_chain")
    script.append("    command_chain=()")
    script.append("")
    script.append("    # Find the command chain")
    script.append("    for ((i=1; i < cword; i++)); do")
    script.append("        if [[ \"${words[i]}\" != -* ]]; then")
    script.append("            command_chain+=(\"${words[i]}\")")
    script.append("        fi")
    script.append("    done")
    script.append("")
    script.append("    local cmd_str=\"${command_chain[*]}\"")
    script.append("")
    script.append("    case \"$cmd_str\" in")

    # Helper to flatten options
    def get_opts(node):
        opts = node.get("options", {})
        if isinstance(opts, dict):
            return " ".join(sorted(opts.keys()))
        return " ".join(sorted(list(set(opts))))

    # Helper to get subcommands
    def get_subs(node):
        return " ".join(sorted(node.get("subcommands", {}).keys()))

    # Recursive function to generate cases
    def visit(node, path):
        path_str = " ".join(path)
        opts = get_opts(node)
        subs = get_subs(node)

        # Case for this path
        script.append(f"        \"{path_str}\")")
        if subs:
            script.append(f"            COMPREPLY=( $(compgen -W \"{subs} {opts}\" -- \"$cur\") )")
        else:
            script.append(f"            COMPREPLY=( $(compgen -W \"{opts}\" -- \"$cur\") )")
        script.append("            return 0")
        script.append("            ;;")

        for sub_name, sub_node in node.get("subcommands", {}).items():
            visit(sub_node, path + [sub_name])

    visit(tree, [])

    script.append("    esac")
    script.append("}")
    script.append("complete -F _kicad_cli kicad-cli")

    return "\n".join(script)

if __name__ == "__main__":
    tree = explore([])
    print(generate_bash_completion(tree))
