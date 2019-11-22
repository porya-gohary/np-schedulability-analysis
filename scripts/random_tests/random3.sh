#!/bin/bash
# Third test BASE = uniprocessor GANG = m
# Failed :( TODO investigate
i="0"
MAXIMUM=10
MAXIMUM_CORES=10
NUMBER_OF_TASKS=2
MAXIMUM_JOBS_PER_TASK=3

while [ $i -lt $MAXIMUM ]; do
    echo "-------- Random test " $i "--------"
    TASK="task_"$i
    JOBS="test_"$i
    EXTENSION=".csv"

    GEN_OUTPUT=$TASK$EXTENSION
    JOBS_OUTPUT=$JOBS$EXTENSION

    #generate random DAG tests
    GENERATE="../random_tasks.py"
    python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $MAXIMUM_JOBS_PER_TASK

    #convert them DAG tasks to jobs
    CORES=$(( ( RANDOM % $MAXIMUM_CORES )  + 2 ))

    CONVERT="../dag-tasks-to-jobs_gang.py"
    python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT edge.csv --fixed $CORES
    #compare them
    SIM_RESULTS_FOLDER="sim_results"
    mkdir -p $SIM_RESULTS_FOLDER

    BASE="../../../BASE-np-schedulability-analysis/cmake-build-debug/nptest"
    BASE_STR="base_"

    #GANG="../../../gang-np-schedulability-analysis/cmake-build-debug/nptest"
    GANG="../../cmake-build-debug/nptest"
    GANG_STR="gang_"

    echo "Running gang with " $CORES " cores"

    #run simulation base
    ./$BASE -m 1 $JOBS_OUTPUT -r
    OUTPUT_BASE=$SIM_RESULTS_FOLDER"/"$BASE_STR$JOBS".rta"$EXTENSION
    mv $JOBS".rta"$EXTENSION $OUTPUT_BASE

    #run simulation gang
    ./$GANG -m $CORES $JOBS_OUTPUT -r
    OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION
    mv $JOBS".rta"$EXTENSION $OUTPUT_GANG

    DIFF=$(diff $OUTPUT_BASE $OUTPUT_GANG)
    if [ "$DIFF" != "" ]
    then
      echo "-------------FILES DIFFERS-----------"
      echo $DIFF
      exit 1
    fi

    i=$[i+1]
done
#rm $SIM_RESULTS_FOLDER"/sim"*.csv