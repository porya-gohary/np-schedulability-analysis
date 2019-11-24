#!/bin/bash
# First test no precedence contraint from 1 to m cores!!!
i="0"
MAXIMUM=20 #number of tests
MAXIMUM_CORES=12 #each test from 1 to m cores
NUMBER_OF_TASKS=3 #number of random tasks created
MAXIMUM_JOBS_PER_TASK=6 #each task may have from 1 to 6 vertices

while [ $i -le $MAXIMUM ]; do
  echo "-------- Random test " $i "--------"
  TASK="task_"$i
  JOBS="test_"$i
  EXTENSION=".csv"

  GEN_OUTPUT=$TASK$EXTENSION
  JOBS_OUTPUT=$JOBS$EXTENSION

  #generate random DAG tests
  GENERATE="../random-tasks-gang.py"
  python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $MAXIMUM_JOBS_PER_TASK -p 1

  #convert them DAG tasks to jobs
  CONVERT="../dag-tasks-to-jobs-gang.py"
  python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT edge.csv --fixed 1

  #compare them
  SIM_RESULTS_FOLDER="sim_results"
  mkdir -p $SIM_RESULTS_FOLDER

  BASE="../../../BASE-np-schedulability-analysis/cmake-build-release/nptest"
  BASE_STR="base_"

  GANG="../../cmake-build-release/nptest"
  GANG_STR="gang_"

  CORES=1
  while [ $CORES -le $MAXIMUM_CORES ]; do
    echo "Running with " $CORES " cores"

    #run simulation base
    ./$BASE -r -m $CORES $JOBS_OUTPUT
    OUTPUT_BASE=$SIM_RESULTS_FOLDER"/"$BASE_STR$JOBS".rta"$EXTENSION
    mv $JOBS".rta"$EXTENSION $OUTPUT_BASE

    #run simulation gang
    ./$GANG -r -m $CORES $JOBS_OUTPUT
    OUTPUT_GANG=$SIM_RESULTS_FOLDER"/"$GANG_STR$JOBS".rta"$EXTENSION
    mv $JOBS".rta"$EXTENSION $OUTPUT_GANG

    DIFF=$(diff $OUTPUT_BASE $OUTPUT_GANG)
    if [ "$DIFF" != "" ]
    then
      echo "-------------FILES DIFFERS-----------"
      echo $DIFF
      exit 1
    fi

    CORES=$[CORES+1]
  done
  i=$[i+1]
done
#clean up only if not failed (exit 1)
rm ./$SIM_RESULTS_FOLDER/*.csv
rm task_*.csv
rm test_*.csv
rm edge.csv