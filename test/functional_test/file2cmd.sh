
#!/bin/sh

num2cmd () {
    COMMAND=$1
    ARG=$2

    if [ $COMMAND = "ALLOC" ]; then
        echo 0
    elif [ $COMMAND = "TALLOC" ]; then
        echo 1
    elif [ $COMMAND = "CALLOC" ]; then
        echo 2
    elif [ $COMMAND = "REALLOC" ]; then
        echo 3
    elif [ $COMMAND = "DEALLOC" ]; then
        echo 4
    elif [ $COMMAND = "ACCESS" ]; then
        echo 5
    elif [ $COMMAND = "TACCESS" ]; then
        echo 6
    elif [ $COMMAND = "SIZE" ]; then
        echo 7
    elif [ $COMMAND = "CLEAR" ]; then
        echo 8
    elif [ $COMMAND = "EXIT" ]; then
        echo 9
    elif [ $COMMAND = "INPUT" ]; then
        echo $ARG
    fi
}

file2cmd() {
    FILE=$1
    while IFS= read -r line; do
        num2cmd $line
    done < $FILE
}
file2cmd $1