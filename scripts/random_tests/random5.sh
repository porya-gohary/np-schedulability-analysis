#!/bin/bash
# Third test BASE - m = 1 (global one processor), GANG = m

# Constants
#---------------------------------------#
SIM_RESULTS_FOLDER="sim_results"
GENERATE_TASKS_FOLDER="generate_tasks"
WRONG_FOLDER="wrong"
mkdir -p $SIM_RESULTS_FOLDER
mkdir -p $GENERATE_TASKS_FOLDER
mkdir -p $WRONG_FOLDER
mkdir -p $WRONG_FOLDER/$SIM_RESULTS_FOLDER
mkdir -p $WRONG_FOLDER/$GENERATE_TASKS_FOLDER

LOG_FILE="log.txt"

function cleanup() {
    #clean up
    rm ./$SIM_RESULTS_FOLDER/*.csv
    rm ./$GENERATE_TASKS_FOLDER/*.csv
}

GENERATE="../random-tasks.py"
CONVERT="../dag-tasks-to-jobs-gang.py"

BASE="../../../BASE-np-schedulability-analysis/cmake-build-debug-serial-jemalloc/nptest"
BASE_STR="base_"

GANG="../../cmake-build-debug-serial-jemalloc/nptest"
GANG_STR="gang_"

#---------------------------------------#

#clean up
cleanup;

#Parameters
MAXIMUM_PER_SCENARIO=100 #number of tests per scenario
MAXIMUM_CORES=20 #each test randomly choosen from 1 to m cores
MAXIMUM_JOBS_PER_TASK=5 #maximum jobs per task (each task may have from 1 to JOBS_PER_TASK vertices)
MAXIMUM_TASKS=4 #maximum tasks
MAXIMUM_PRECEDENCE_PER_JOB=3 #each job may have from 0 to 3 precedence contraints
# timeout used in bash in order to not produce at all results -> just to avoid very very large tasksets for now
# only used if base code is timeout not in gang code, if base is ran less than the timeout gang must run too

#Initialise
i=0
NUMBER_OF_TASKS=2 #number of random tasks created
JOBS_PER_TASK=3 #each task may have from 1 to JOBS_PER_TASK vertices
while [ $NUMBER_OF_TASKS -le $MAXIMUM_TASKS ]; do
  while [ $JOBS_PER_TASK -le $MAXIMUM_JOBS_PER_TASK ]; do
      #clean up
      cleanup;
      echo "-------- Scenario = Tasks:" $NUMBER_OF_TASKS " ,Jobs:" $JOBS_PER_TASK, "Precedence:" $MAXIMUM_PRECEDENCE_PER_JOB " --------"
      while [ $i -lt $MAXIMUM_PER_SCENARIO ]; do
          echo "-------- Random test " $i "--------"
          TASK="task_"$i
          JOBS="test_"$i
          EXTENSION=".csv"

          GEN_OUTPUT=$GENERATE_TASKS_FOLDER"/"$TASK$EXTENSION
          JOBS_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS$EXTENSION
          PRECEDENCE_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS".prec"$EXTENSION

          #choose random cores 2-MAXIMUM_CORES
          CORES_BASE=$(( ( RANDOM % $MAXIMUM_CORES )  + 1 ))
          CORES=$[CORES_BASE * CORES_BASE];

          #generate random DAG tests
          python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $JOBS_PER_TASK -p $MAXIMUM_PRECEDENCE_PER_JOB

          #convert them DAG tasks to jobs
          python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT $PRECEDENCE_OUTPUT --fixed $CORES_BASE

          #force only test with precedence contraints
          #lines=$(wc -l $PRECEDENCE_OUTPUT | cut -f1 -d' ')
          #if [ $lines -le 2 ]; then
          #    continue;
          #fi

          BOOL_TIMEOUT=false

          OUTPUT_BASE=$SIM_RESULTS_FOLDER"/"$BASE_STR$JOBS".rta"$EXTENSION
          OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION
          SCHEDULABLE_BASE=0
          SCHEDULABLE_GANG=0
          RTA_OUTPUT=$GENERATE_TASKS_FOLDER"/"$JOBS".rta"$EXTENSION
          #run simulation base
          echo "Running base with " $CORES_BASE " core(s)"
          OUT=$(./$BASE -m $CORES_BASE $JOBS_OUTPUT -r --precedence $PRECEDENCE_OUTPUT 2>> $LOG_FILE)
          echo $OUT

          #update schedulable if a file is created (assert)
          if [ -f $RTA_OUTPUT ]; then
            if [ -z "$OUT" ]; then
              continue ;
            fi
            SCHEDULABLE_BASE=$(echo $OUT | awk -F, '{print $2}' | tr -d '[:space:]')
            if [ $SCHEDULABLE_BASE = 0 ]; then
                continue;
            fi
            mv $RTA_OUTPUT $OUTPUT_BASE
          else
            #file is not created based on assertions!
            BOOL_TIMEOUT=true
            continue;
          fi

          #run simulation gang if base is not timeout -> gang needs a little more time (few seconds)
          echo "Running gang with " $CORES " cores, fixed = " $CORES_BASE
          OUT=$(./$GANG -m $CORES $JOBS_OUTPUT -r --precedence $PRECEDENCE_OUTPUT 2>> $LOG_FILE)
          echo $OUT
          if [ -f $RTA_OUTPUT ]; then
              SCHEDULABLE_GANG=$(echo $OUT | awk -F, '{print $2}' | tr -d '[:space:]')
              mv $RTA_OUTPUT $OUTPUT_GANG
          fi

          if [ $BOOL_TIMEOUT == false ]; then
            DIFF=$(diff $OUTPUT_BASE $OUTPUT_GANG)
            if [ "$DIFF" != "" ]
            then
              echo "-------------FILES DIFFERS-----------"
              echo "$DIFF"

              #copy not identical results!!!
              cp $OUTPUT_BASE $WRONG_FOLDER/$OUTPUT_BASE
              cp $OUTPUT_GANG $WRONG_FOLDER/$OUTPUT_GANG
              cp $JOBS_OUTPUT $WRONG_FOLDER/$JOBS_OUTPUT
              cp $PRECEDENCE_OUTPUT $WRONG_FOLDER/$PRECEDENCE_OUTPUT
              cp $GEN_OUTPUT $WRONG_FOLDER/$GEN_OUTPUT
              exit 1; #FAILED EXIT SCRIPT

            fi
          fi

        i=$[i+1]
    done
    i=0;
    JOBS_PER_TASK=$[JOBS_PER_TASK+1]
  done
  JOBS_PER_TASK=3
  NUMBER_OF_TASKS=$[NUMBER_OF_TASKS+1]
done

#last clean up only if not failed (exit 1)
cleanup;