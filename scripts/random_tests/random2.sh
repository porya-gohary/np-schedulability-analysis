#!/bin/bash
# Second test with precedence contraint from 1 to m cores!!!
i="0"
MAXIMUM=10
MAXIMUM_CORES=10
NUMBER_OF_TASKS=3
MAXIMUM_JOBS_PER_TASK=6
MAXIMUM_PRECEDENCE=2

while [ $i -le $MAXIMUM ]; do
  echo "-------- Random test " $i "--------"
  TASK="task_"$i
  JOBS="test_"$i
  EXTENSION=".csv"

  GEN_OUTPUT=$TASK$EXTENSION
  JOBS_OUTPUT=$JOBS$EXTENSION
  PRECEDENCE_OUTPUT=$JOBS"-pre"$EXTENSION

  #generate random DAG tests
  GENERATE="../random_tasks.py"
  python3 $GENERATE --save $GEN_OUTPUT -t $NUMBER_OF_TASKS -j $MAXIMUM_JOBS_PER_TASK -p $MAXIMUM_PRECEDENCE

  #convert them DAG tasks to jobs
  CONVERT="../dag-tasks-to-jobs.py"
  python3 $CONVERT $GEN_OUTPUT $JOBS_OUTPUT $PRECEDENCE_OUTPUT

  #compare them
  SIM_RESULTS_FOLDER="sim_results"
  mkdir -p $SIM_RESULTS_FOLDER

  BASE="../../../BASE-np-schedulability-analysis/cmake-build-debug/nptest"
  BASE_STR="base_"

  GANG="../../cmake-build-debug/nptest"
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
#rm $SIM_RESULTS_FOLDER"/sim"*.csv