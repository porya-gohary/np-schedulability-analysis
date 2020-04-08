#ifndef CONFIG_H
#define CONFIG_H

// DM : debug message -- disable for now
// #define DM(x) std::cerr << x
#define DM(x)

// #define CONFIG_COLLECT_SCHEDULE_GRAPH

// use rta only at depth level
// #define RTA_SAME_DEPTH

// base code fix new state
// #define FIX_NEW_STATE

// remove this to go back to the previous code!
#define GANG

// remove this to not output inf in rta file
// #define NEW_OUTPUT

#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
#define CONFIG_PARALLEL
#endif

#ifndef NDEBUG
#define TBB_USE_DEBUG 1
#endif

#endif
