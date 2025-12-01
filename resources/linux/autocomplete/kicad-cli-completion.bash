# kicad-cli bash completion

_kicad_cli()
{
    local cur prev words cword
    if type _init_completion >/dev/null 2>&1; then
    _init_completion || return
    else
        COMPREPLY=()
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    local command_chain
    command_chain=()

    # Find the command chain
    for ((i=1; i < cword; i++)); do
        if [[ "${words[i]}" != -* ]]; then
            command_chain+=("${words[i]}")
        fi
    done

    local cmd_str="${command_chain[*]}"

    case "$cmd_str" in
        "")
            COMPREPLY=( $(compgen -W "fp jobset pcb sch sym version --help --version -h -v" -- "$cur") )
            return 0
            ;;
        "fp")
            COMPREPLY=( $(compgen -W "export upgrade --help -h" -- "$cur") )
            return 0
            ;;
        "fp export")
            COMPREPLY=( $(compgen -W "svg --help -h" -- "$cur") )
            return 0
            ;;
        "fp export svg")
            COMPREPLY=( $(compgen -W "--black-and-white --cdnp --crossout-DNP-footprints-on-fab-layers --define-var --footprint --fp --hdnp --help --hide-DNP-footprints-on-fab-layers --layers --output --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --theme -D -h -l -o -t" -- "$cur") )
            return 0
            ;;
        "fp upgrade")
            COMPREPLY=( $(compgen -W "--force --help --output -h -o" -- "$cur") )
            return 0
            ;;
        "jobset")
            COMPREPLY=( $(compgen -W "run --help -h" -- "$cur") )
            return 0
            ;;
        "jobset run")
            COMPREPLY=( $(compgen -W "--file --help --output --stop-on-error -f -h" -- "$cur") )
            return 0
            ;;
        "pcb")
            COMPREPLY=( $(compgen -W "drc export render upgrade --help -h" -- "$cur") )
            return 0
            ;;
        "pcb drc")
            COMPREPLY=( $(compgen -W "--all-track-errors --define-var --exit-code-violations --format --help --output --refill-zones --save-board --schematic-parity --severity-all --severity-error --severity-exclusions --severity-warning --units -D -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export")
            COMPREPLY=( $(compgen -W "3dpdf brep drill dxf gencad gerber gerbers glb hpgl ipc2581 ipcd356 odb pdf ply pos ps stats step stl stpz svg u3d vrml xao --help -h" -- "$cur") )
            return 0
            ;;
        "pcb export 3dpdf")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export brep")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export drill")
            COMPREPLY=( $(compgen -W "--drill-origin --excellon-min-header --excellon-mirror-y --excellon-oval-format --excellon-separate-th --excellon-units --excellon-zeros-format --format --generate-map --generate-tenting --gerber-precision --help --map-format --output -h -o -u" -- "$cur") )
            return 0
            ;;
        "pcb export dxf")
            COMPREPLY=( $(compgen -W "--cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --drawing-sheet --drill-shape-opt --erd --ev --exclude-refdes --exclude-value --hdnp --help --hide-DNP-footprints-on-fab-layers --ibt --include-border-title --layers --mode-multi --mode-single --ou --output --output-units --plot-invisible-text --scale --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --uc --udo --use-contours --use-drill-origin -D -h -l -o" -- "$cur") )
            return 0
            ;;
        "pcb export gencad")
            COMPREPLY=( $(compgen -W "--define-var --flip-bottom-pads --help --output --store-origin-coord --unique-footprints --unique-pins --use-drill-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export gerber")
            COMPREPLY=( $(compgen -W "--cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --disable-aperture-macros --drawing-sheet --erd --ev --exclude-refdes --exclude-value --hdnp --help --hide-DNP-footprints-on-fab-layers --ibt --include-border-title --layers --no-netlist --no-protel-ext --no-x2 --output --plot-invisible-text --precision --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --use-drill-file-origin -D -h -l -o" -- "$cur") )
            return 0
            ;;
        "pcb export gerbers")
            COMPREPLY=( $(compgen -W "--board-plot-params --cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --disable-aperture-macros --drawing-sheet --erd --ev --exclude-refdes --exclude-value --hdnp --help --hide-DNP-footprints-on-fab-layers --ibt --include-border-title --layers --no-netlist --no-protel-ext --no-x2 --output --plot-invisible-text --precision --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --use-drill-file-origin -D -h -l -o" -- "$cur") )
            return 0
            ;;
        "pcb export glb")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export hpgl")
            COMPREPLY=( $(compgen -W "--help --output -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export ipc2581")
            COMPREPLY=( $(compgen -W "--bom-col-dist --bom-col-dist-pn --bom-col-int-id --bom-col-mfg --bom-col-mfg-pn --compress --define-var --drawing-sheet --help --output --precision --units --version -D -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export ipcd356")
            COMPREPLY=( $(compgen -W "--help --output -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export odb")
            COMPREPLY=( $(compgen -W "--compression --define-var --drawing-sheet --help --output --precision --units -D -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export pdf")
            COMPREPLY=( $(compgen -W "--bg-color --black-and-white --cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --drawing-sheet --drill-shape-opt --erd --ev --exclude-refdes --exclude-value --hdnp --help --hide-DNP-footprints-on-fab-layers --ibt --include-border-title --layers --mirror --mode-multipage --mode-separate --mode-single --negative --output --plot-invisible-text --scale --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --theme -D -h -l -m -n -o -t" -- "$cur") )
            return 0
            ;;
        "pcb export ply")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export pos")
            COMPREPLY=( $(compgen -W "--bottom-negate-x --exclude-dnp --exclude-fp-th --format --gerber-board-edge --help --output --side --smd-only --units --use-drill-file-origin -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export ps")
            COMPREPLY=( $(compgen -W "--black-and-white --cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --drawing-sheet --drill-shape-opt --erd --ev --exclude-refdes --exclude-value --force-a4 --hdnp --help --hide-DNP-footprints-on-fab-layers --ibt --include-border-title --layers --mirror --mode-multi --mode-single --negative --output --scale --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --theme --track-width-correction --x-scale-factor --y-scale-factor -A -C -D -X -Y -h -l -m -n -o -t" -- "$cur") )
            return 0
            ;;
        "pcb export stats")
            COMPREPLY=( $(compgen -W "--exclude-footprints-without-pads --format --help --output --subtract-holes-from-board --subtract-holes-from-copper --units -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export step")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-optimize-step --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export stl")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export stpz")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-optimize-step --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export svg")
            COMPREPLY=( $(compgen -W "--black-and-white --cdnp --check-zones --cl --common-layers --crossout-DNP-footprints-on-fab-layers --define-var --drawing-sheet --drill-shape-opt --exclude-drawing-sheet --fit-page-to-board --hdnp --help --hide-DNP-footprints-on-fab-layers --layers --mirror --mode-multi --mode-single --negative --output --page-size-mode --plot-invisible-text --scale --sdnp --sketch-DNP-footprints-on-fab-layers --sketch-pads-on-fab-layers --sp --subtract-soldermask --theme -D -h -l -m -n -o -t" -- "$cur") )
            return 0
            ;;
        "pcb export u3d")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export vrml")
            COMPREPLY=( $(compgen -W "--define-var --force --help --models-dir --models-relative --no-dnp --no-unspecified --output --units --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb export xao")
            COMPREPLY=( $(compgen -W "--board-only --component-filter --cut-vias-in-body --define-var --drill-origin --fill-all-vias --force --fuse-shapes --grid-origin --help --include-inner-copper --include-pads --include-silkscreen --include-soldermask --include-tracks --include-zones --min-distance --net-filter --no-board-body --no-components --no-dnp --no-unspecified --output --subst-models --user-origin -D -f -h -o" -- "$cur") )
            return 0
            ;;
        "pcb render")
            COMPREPLY=( $(compgen -W "--background --define-var --floor --height --help --light-bottom --light-camera --light-side --light-side-elevation --light-top --output --pan --perspective --pivot --preset --quality --rotate --side --use-board-stackup-colors --width --zoom -D -h -o -w" -- "$cur") )
            return 0
            ;;
        "pcb upgrade")
            COMPREPLY=( $(compgen -W "--force --help -h" -- "$cur") )
            return 0
            ;;
        "sch")
            COMPREPLY=( $(compgen -W "erc export upgrade --help -h" -- "$cur") )
            return 0
            ;;
        "sch erc")
            COMPREPLY=( $(compgen -W "--define-var --exit-code-violations --format --help --output --severity-all --severity-error --severity-exclusions --severity-warning --units -D -h -o" -- "$cur") )
            return 0
            ;;
        "sch export")
            COMPREPLY=( $(compgen -W "bom dxf hpgl netlist pdf ps python-bom svg --help -h" -- "$cur") )
            return 0
            ;;
        "sch export bom")
            COMPREPLY=( $(compgen -W "--exclude-dnp --field-delimiter --fields --filter --format-preset --group-by --help --include-excluded-from-bom --keep-line-breaks --keep-tabs --labels --output --preset --ref-delimiter --ref-range-delimiter --sort-asc --sort-field --string-delimiter -h -o" -- "$cur") )
            return 0
            ;;
        "sch export dxf")
            COMPREPLY=( $(compgen -W "--black-and-white --default-font --define-var --draw-hop-over --drawing-sheet --exclude-drawing-sheet --help --output --pages --theme -D -b -e -h -o -t" -- "$cur") )
            return 0
            ;;
        "sch export hpgl")
            COMPREPLY=( $(compgen -W "--black-and-white --default-font --define-var --draw-hop-over --drawing-sheet --exclude-drawing-sheet --help --origin --output --pages --pen-size --theme -D -b -e -h -o -p -r -t" -- "$cur") )
            return 0
            ;;
        "sch export netlist")
            COMPREPLY=( $(compgen -W "--format --help --output -h -o" -- "$cur") )
            return 0
            ;;
        "sch export pdf")
            COMPREPLY=( $(compgen -W "--black-and-white --default-font --define-var --draw-hop-over --drawing-sheet --exclude-drawing-sheet --exclude-pdf-hierarchical-links --exclude-pdf-metadata --exclude-pdf-property-popups --help --no-background-color --output --pages --theme -D -b -e -h -n -o -t" -- "$cur") )
            return 0
            ;;
        "sch export ps")
            COMPREPLY=( $(compgen -W "--black-and-white --default-font --define-var --draw-hop-over --drawing-sheet --exclude-drawing-sheet --help --no-background-color --output --pages --theme -D -b -e -h -n -o -t" -- "$cur") )
            return 0
            ;;
        "sch export python-bom")
            COMPREPLY=( $(compgen -W "--help --output -h -o" -- "$cur") )
            return 0
            ;;
        "sch export svg")
            COMPREPLY=( $(compgen -W "--black-and-white --default-font --define-var --draw-hop-over --drawing-sheet --exclude-drawing-sheet --help --no-background-color --output --pages --theme -D -b -e -h -n -o -t" -- "$cur") )
            return 0
            ;;
        "sch upgrade")
            COMPREPLY=( $(compgen -W "--force --help -h" -- "$cur") )
            return 0
            ;;
        "sym")
            COMPREPLY=( $(compgen -W "export upgrade --help -h" -- "$cur") )
            return 0
            ;;
        "sym export")
            COMPREPLY=( $(compgen -W "svg --help -h" -- "$cur") )
            return 0
            ;;
        "sym export svg")
            COMPREPLY=( $(compgen -W "--black-and-white --help --include-hidden-fields --include-hidden-pins --output --symbol --theme -h -o -s -t" -- "$cur") )
            return 0
            ;;
        "sym upgrade")
            COMPREPLY=( $(compgen -W "--force --help --output -h -o" -- "$cur") )
            return 0
            ;;
        "version")
            COMPREPLY=( $(compgen -W "--format --help -h" -- "$cur") )
            return 0
            ;;
    esac
}
complete -F _kicad_cli kicad-cli
