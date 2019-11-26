#!/bin/bash
# Third test BASE - m = 1 (global one processor), GANG = m

# Constants
#---------------------------------------#
SIM_RESULTS_FOLDER="sim_results"
GENERATE_TASKS_FOLDER="generate_tasks"
mkdir -p $SIM_RESULTS_FOLDER
mkdir -p $GENERATE_TASKS_FOLDER

function cleanup() {
    #clean up
    rm ./$SIM_RESULTS_FOLDER/*.csv
    rm ./$GENERATE_TASKS_FOLDER/*.csv
}

GENERATE="../random-tasks.py"
CONVERT="../dag-tasks-to-jobs-gang.py"

BASE="../../../BASE-np-schedulability-analysis/cmake-build-release/nptest"
BASE_STR="base_"

GANG="../../cmake-build-release/nptest"
GANG_STR="gang_"
#---------------------------------------#

#clean up
cleanup;

#Parameters
MAXIMUM_PER_SCENARIO=20 #number of tests per scenario
MAXIMUM_CORES=32 #each test randomly choosen from 1 to m cores
MAXIMUM_JOBS_PER_TASK=10 #maximum jobs per task (each task may have from 1 to JOBS_PER_TASK vertices)
MAXIMUM_TASKS=4 #maximum tasks
# timeout used in bash in order to not produce at all results -> just to avoid very very large tasksets for now
# only used if base code is timeout not in gang code, if base is ran less than the timeout gang must run too
TIMEOUT=180 #in seconds

#Initialise
i=0
NUMBER_OF_TASKS=2 #number of random tasks created
JOBS_PER_TASK=2 #each task may have from 1 to JOBS_PER_TASK vertices

while [ $NUMBER_OF_TASKS -le $MAXIMUM_TASKS ]; do
  while [ $JOBS_PER_TASK -le $MAXIMUM_JOBS_PER_TASK ]; do

    #clean up
    cleanup;

    echo "-------- Scenario = Tasks:" $NUMBER_OF_TASKS " ,Jobs:" $JOBS_PER_TASK " --------"
    while [ $i -lt $MAXIMUM_PER_SCENARIO ]; do
        echo "-------- Random test " $i "--------"
        TASK="task_"$i
        JOBS="test_"$i
        EXTENSION=".csv"

        GEN_OUTPUT=$GENERATE_TASKS_FOLDER"/"$TASK$EXTENSION
        JOBS_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS$EXTENSION

        #choose random cores 2-MAXIMUM_CORES
        CORES=$(( ( RANDOM % $MAXIMUM_CORES )  + 2 ))

        #generate random DAG tests
        python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $JOBS_PER_TASK -p 0

        #convert them DAG tasks to jobs -> edge.csv (precedence constraints not used)
        python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT $GENERATE_TASKS_FOLDER/edge.csv --fixed $CORES

        #get a small example if failed
        #lines=$(wc -l $JOBS_OUTPUT | cut -f1 -d' ')
        #if [ $lines -gt 40 ]; then
        #    continue;
        #fi

        BOOL_TIMEOUT=false

        OUTPUT_BASE=$SIM_RESULTS_FOLDER"/"$BASE_STR$JOBS".rta"$EXTENSION
        OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION

        #run simulation base
        echo "Running base with 1 core"
        ./timeout3.sh -t $TIMEOUT ./$BASE -m 1 $JOBS_OUTPUT -r
        RTA_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS".rta"$EXTENSION

        if [ -f $RTA_OUTPUT ]; then
          mv $RTA_OUTPUT $OUTPUT_BASE

          #run simulation gang if base is not timeout -> gang needs a little more time (few seconds)
          echo "Running gang with " $CORES " cores"
          ./$GANG -m $CORES $JOBS_OUTPUT -r
          mv $RTA_OUTPUT $OUTPUT_GANG

        else
          BOOL_TIMEOUT=true
        fi

        if [ $BOOL_TIMEOUT == false ]; then
          DIFF=$(diff $OUTPUT_BASE $OUTPUT_GANG)
          if [ "$DIFF" != "" ]
          then
            echo "-------------FILES DIFFERS-----------"
            echo "$DIFF"
            exit 1; #FAILED EXIT SCRIPT
          fi
        fi

      i=$[i+1]
    done
    i=0
    JOBS_PER_TASK=$[JOBS_PER_TASK+1]
  done
  JOBS_PER_TASK=2
  NUMBER_OF_TASKS=$[NUMBER_OF_TASKS+1]
done

#last clean up only if not failed (exit 1)
cleanup;