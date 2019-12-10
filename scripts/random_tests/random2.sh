#!/bin/bash
# Second test with precedence contraint from 1 to m cores!

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
CONVERT="../dag-tasks-to-jobs.py"

BASE="../../../BASE-np-schedulability-analysis/cmake-build-debug-serial-jemalloc/nptest"
BASE_STR="base_"

GANG="../../cmake-build-debug-serial-jemalloc/nptest"
GANG_STR="gang_"
#---------------------------------------#

#clean up
cleanup;

#Parameters
MAXIMUM_PER_SCENARIO=10 #number of tests per scenario
MAXIMUM_CORES=32 #each test randomly choosen from 1 to m cores
MAXIMUM_JOBS_PER_TASK=8 #maximum jobs per task (each task may have from 1 to JOBS_PER_TASK vertices)
MAXIMUM_TASKS=3 #maximum tasks
MAXIMUM_PRECEDENCE_PER_JOB=3 #each job may have from 0 to 3 precedence contraints

#Initialise counters
i=0
NUMBER_OF_TASKS=2 #number of random tasks created
JOBS_PER_TASK=3 #each task may have from 1 to JOBS_PER_TASK vertices

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
      PRECEDENCE_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS".prec"$EXTENSION

      #generate random DAG tests
      python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $MAXIMUM_JOBS_PER_TASK -p $MAXIMUM_PRECEDENCE_PER_JOB

      #convert them DAG tasks to jobs
      python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT $PRECEDENCE_OUTPUT


      CORES=1
      while [ $CORES -le $MAXIMUM_CORES ]; do

          BOOL_TIMEOUT=false

          OUTPUT_BASE=$SIM_RESULTS_FOLDER"/"$BASE_STR$JOBS".rta"$EXTENSION
          OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION

          echo "Running base with " $CORES " cores"
          ./$BASE -m $CORES $JOBS_OUTPUT -r --precedence $PRECEDENCE_OUTPUT
          RTA_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS".rta"$EXTENSION

          if [ -f $RTA_OUTPUT ]; then
            mv $RTA_OUTPUT $OUTPUT_BASE

            #run simulation gang if base is not timeout -> gang needs a little more time (few seconds)
            echo "Running gang with " $CORES " cores"
            ./$GANG -m $CORES $JOBS_OUTPUT -r --precedence $PRECEDENCE_OUTPUT
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
          CORES=$[CORES+1]
        done
      i=$[i+1]
    done
    i=0
    JOBS_PER_TASK=$[JOBS_PER_TASK+1]
  done
  JOBS_PER_TASK=3
  NUMBER_OF_TASKS=$[NUMBER_OF_TASKS+1]
done
#clean up only if not failed (exit 1)
cleanup;