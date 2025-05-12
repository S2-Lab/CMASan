env_script=""
script_suffix=""
arg1=$1
if [ -n "$2" ]; then
    script_suffix="_$2"
elif [ "$arg1" == "builtin" ] || [ "$arg1" == "with_flags" ]; then
    arg1=""
    script_suffix="_$1"
fi

if [ "$arg1" = "log" ]; then
    env_script="env_cmasan${script_suffix}.sh log"
elif [ "$arg1" = "codeql" ]; then
    env_script="env_codeql${script_suffix}.sh"
elif [ "$arg1" = "" ] || [ "$arg1" == "cmasan" ]; then
    env_script="env_cmasan${script_suffix}.sh"
else 
    export PNAME=${PNAME}_$arg1
    if [ "$arg1" == "pure" ]; then
        env_script="env_pure${script_suffix}.sh"
    elif [ "$arg1" == "asan" ]; then
        env_script="env_asan${script_suffix}.sh"
    fi
fi

rm -rf src/$PNAME result/$PNAME

source env/$env_script