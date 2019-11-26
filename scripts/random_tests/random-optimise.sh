#!/bin/bash
#NOT RANDOM TEST!
i="0"
MAXIMUM=10
MAXIMUM_CORES=32
NUMBER_OF_TASKS=3
MAXIMUM_JOBS_PER_TASK=6

while [ $i -lt $MAXIMUM ]; do
    echo "-------- Random test " $i "--------"
    TASK="task_"$i
    JOBS="test_"$i
    EXTENSION=".csv"

    GEN_OUTPUT=$TASK$EXTENSION
    JOBS_OUTPUT=$JOBS$EXTENSION

    #convert them DAG tasks to jobs
    CORES=$(( ( RANDOM % $MAXIMUM_CORES )  + 1 ))

    #generate random DAG tests
    echo "GENERATE MAXIMUM " $CORES
    GENERATE="../random-tasks-gang.py"
    python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $MAXIMUM_JOBS_PER_TASK -p $CORES

    CONVERT="../dag-tasks-to-jobs-gang.py"
    python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT edge.csv
    #compare them
    SIM_RESULTS_FOLDER="sim_results"
    mkdir -p $SIM_RESULTS_FOLDER

    #BASE="../../../BASE-np-schedulability-analysis/cmake-build-debug/nptest"
    #BASE_STR="base_"

    #GANG="../../../gang-np-schedulability-analysis/cmake-build-debug/nptest"
    GANG="../../cmake-build-debug/nptest"
    GANG_STR="gang_"

    RCORES=$(( ( RANDOM % $MAXIMUM_CORES )  + 2*$CORES ))

    echo "Running gang with " $RCORES " cores"

    #run simulation gang
    ./$GANG -m $RCORES $JOBS_OUTPUT -r
    OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION
    mv $JOBS".rta"$EXTENSION $OUTPUT_GANG

    i=$[i+1]
done
#clean up only if not failed (exit 1)
rm ./$SIM_RESULTS_FOLDER/*.csv
rm task_*.csv
rm test_*.csv