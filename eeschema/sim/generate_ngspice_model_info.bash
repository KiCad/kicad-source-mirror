#!/bin/bash

# This script is disgusting (very slow and hacky). Please rewrite it in Python if you have time.


MODELS=$(cat << END
    resistor  R    -    R 2 0  0
    capacitor C    -    C 2 0  0
    inductor  L    -    L 2 0  0

    ltra      LTRA -    O 4 0  0
    tranline  -    -    T 4 0  0
    urc       URC  -    U 3 0  0
    transline -    -    Y 4 0  0

    diode     D    -    D 2 0  0

    bjt       NPN  PNP  Q 3 1  0
    vbic      NPN  PNP  Q 3 4  0
    hicum2    NPN  PNP  Q 3 8  0

    jfet      NJF  PJF  J 3 1  0
    jfet2     NJF  PJF  J 3 2  0

    mes       NMF  PMF  Z 3 1  0
    mesa      NMF  PMF  Z 3 2  0
    hfet1     NMF  PMF  Z 3 5  0
    hfet2     NMF  PMF  Z 3 6  0

    mos1      NMOS PMOS M 4 1  0
    mos2      NMOS PMOS M 4 2  0
    mos3      NMOS PMOS M 4 3  0
    bsim1     NMOS PMOS M 4 4  0
    bsim2     NMOS PMOS M 4 5  0
    mos6      NMOS PMOS M 4 6  0
    bsim3     NMOS PMOS M 4 8  3.3.0
    mos9      NMOS PMOS M 4 9  0
    b4soi     NMOS PMOS M 4 10 0
    bsim4     NMOS PMOS M 4 14 4.8.1
    b3soifd   NMOS PMOS M 4 55 0
    b3soidd   NMOS PMOS M 4 56 0
    b3soipd   NMOS PMOS M 4 57 0
    hisim2    NMOS PMOS M 4 68 0
    hisimhv1  NMOS PMOS M 4 73 1.2.4
    hisimhv2  NMOS PMOS M 4 73 2.2.0
END
)


UNITS=$(cat << END
    %/deg C
        exponent alternative

    -
        exponent

    ohm/m
        sheet resistance
        resistance per unit length

    ohm
        resistance
        resistor model default value

    F/m^2
        cap per area
    
    F/m
        capacitance per meter
        overlap cap
        capacitance per unit length
        capacitance grading coefficient per unit length

    F
        capacitance
        cap\.
    
    H
        inductance

    1/W
        coef of thermal current reduction

    sqrt V
        bulk effect coefficient 1
        bulk threshold parameter

    1/V
        channel length modulation
        vgs dependence on mobility

    V/cm
        Crit. field for mob. degradation

    V
        voltage
        potential
    
    A/V^2
        transconductance parameter

    A/m^2
        current density

    A/m
        current per unit length

    A
        current

    ohm/deg C^2
        second order temp. coefficient

    ohm/deg C
        first order temp. coefficient

    1/deg C^2
        grading coefficient 1st temp. coeff

    1/deg C
        grading coefficient 2nd temp. coeff

    deg C/W
        thermal resistance

    deg C
        temperature

    eV
        energy
    
    cm^2/V^2 s
        VBS dependence on muz
        VBS dependence on mus
        VDS dependence on mus

    cm^2/V s
        zero field mobility
        surface mobility

    um/V^2
        VDS depence of u1
    
    um
        .* in um
    
    1/cm^3
        substrate doping

    1/cm^2
        surface state density
        fast surface state density

    m/s
        velocity

    m
        length
        width
        thickness
        narrowing of
        shortening of

    C
        epi charge parameter

    s
        time

    deg
        excess phase

    -
        .*
END
)


run_ngspice()
{
    ngspice -n 2>/dev/null << END
        *
        $1
        .control
        $2
        .endc
END
}


{
    echo "// Generated using the $(basename $0) script."
    echo "// Modify that script instead of this file if you want to make changes."
    echo ""
    echo "#include <sim/ngspice.h>"
    echo ""

    echo "enum class NGSPICE::MODEL_TYPE"
    echo "{"

    echo "$MODELS" | while read -r model_name model_primitive model_level model_version; do
        if [ -n "$model_name" ]; then
            echo "    ${model_name^^},"
        fi
    done

    echo "};"
    echo ""

    echo "NGSPICE::MODEL_INFO NGSPICE::GetModelInfo( NGSPICE::MODEL_TYPE aType )"
    echo "{"
    echo "    switch( aType )"
    echo "    {"

    echo "$MODELS" | while read -r model_name \
                                   model_type1 \
                                   model_type2 \
                                   model_primitive \
                                   model_pin_count \
                                   model_level \
                                   model_version
    do
        if [ -n "$model_name" ]; then
            # Print model description.
            run_ngspice "" "devhelp -type $model_name" | while read -r name sep description; do
                if [ "$sep" = "-" ]; then
                    echo -n "    case NGSPICE::MODEL_TYPE::${model_name^^}:"
                    echo -n " return { \"$name\","

                    for model_type in "$model_type1" "$model_type2"; do
                        if [ "$model_type" != "-" ]; then
                            echo -n " \"$model_type\","
                        else
                            echo -n " \"\","
                        fi
                    done

                    echo " \"$description\","
                fi
            done


            # Print model parameter ID, name, direction, type, unit, and description.
            run_ngspice "" "devhelp -type $model_name" | while read -r param_id \
                                                                       param_name \
                                                                       param_dir \
                                                                       param_type \
                                                                       param_description
            do
                if [ "$param_id" = "Model" ] && [ "$param_name" = "Parameters" ]; then
                    echo "        // Model parameters"
                    echo "        {"
                elif [ "$param_id" = "Instance" ] && [ "$param_name" = "Parameters" ]; then
                    echo "        },"
                    echo "        // Instance parameters"
                    echo "        {"
                elif [ "$param_id" -eq "$param_id" ] 2>/dev/null \
                &&   [ -n "$param_name" ] \
                &&   [ -n "$param_dir" ] \
                &&   [ -n "$param_description" ]
                then
                    echo -n "            { \"${param_name,,}\","
                    echo -n " { $param_id,"
                    echo -n " NGSPICE::PARAM_DIR::${param_dir^^},"
                    echo -n " NGSPICE::PARAM_TYPE::${param_type^^},"


                    unit=""

                    # Non-reals are unlikely to have units.
                    if [ "$param_type" = "real" ]; then
                        # Don't use a pipe here because it creates a subshell, as it prevents the
                        # changes to the variables from propagating upwards. Bash is cursed.
                        while read -r pattern; do
                            if [ "$unit" = "" ]; then
                                unit="$pattern"
                            elif [ -z "$pattern" ]; then
                                unit=""
                            elif grep -iE "$pattern" <<< "$param_description" >/dev/null; then
                                break
                            fi
                        done <<< "$UNITS"
                    fi

                    if [ "$unit" = "-" ]; then
                        unit=""
                    fi

                    echo -n " \"$unit\","

                    for model_type in "$model_type1" "$model_type2"; do
                        if [ "$model_type" = "-" ]; then
                            echo -n " \"\","
                            continue
                        fi

                        # For a given primitive, Ngspice determines the device model to be used
                        # from two parameters: "level" and "version".
                        params=""

                        if [ "$model_level" != 0 ]; then
                            params="$params level=$model_level"
                        fi

                        if [ "$model_version" != 0 ]; then
                            params="$params version=$model_version"
                        fi

                        netlist=$(cat << END
                            .model $model_type $model_type($params)
                            ${model_primitive}1 $(seq -s ' ' $model_pin_count) $model_type
END
                        )

                        control=$(cat << END
                            op
                            showmod ${model_primitive}1 : $param_name
END
                        )

                        was_model_line=0
                        was_echoed=0
                        
                        # Don't use a pipe here either.
                        while read -r name value; do
                            # Ngspice displays only the first 11 characters of the variable name.
                            # We also convert to lowercase because a few parameter names have
                            # uppercase characters in them.

                            lowercase_name=${name,,}
                            lowercase_param_name=${param_name,,}

                            if [ "$was_model_line" = 0 ] && [ "$lowercase_name" = "model" ]; then
                                was_model_line=1
                            elif [ "$was_model_line" = 1 ] \
                                && [ "$lowercase_name" = "${lowercase_param_name:0:11}" ]
                            then
                                if [ "$value" = "<<NAN, error = 7>>" ]; then
                                    value="NaN"
                                elif [ "$value" = "?????????" ]; then
                                    value=""
                                fi

                                was_echoed=1
                                echo -n " \"$value\","
                            fi
                        done < <(run_ngspice "$netlist" "$control")

                        if [ "$was_echoed" = 0 ]; then
                            echo ""
                            echo "Error! Default value not found."
                            exit 1
                        fi
                    done

                    echo " \"$param_description\" } },"
                fi
            done


            echo "        } };"
        fi
    done

    echo "    }"
    echo ""

    echo "    wxFAIL;"
    echo "    return {};"
    echo "}"
} > $(dirname "$0")/ngspice_models.cpp
