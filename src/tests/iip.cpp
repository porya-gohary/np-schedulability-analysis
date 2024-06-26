#include "doctest.h"

#include <iostream>


#include "uni/space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();


// The example in Fig 1 of Nasri & Fohler (ECRTS 2016):
//    "Non-Work-Conserving Non-Preemptive Scheduling:
//     Motivations, Challenges, and Potential Solutions"
TEST_CASE("[IIP] P-RM example (Figure 1)")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task tau_1
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(1, 1), 10, 1, 1},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 1), 20, 1, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 1), 30, 1, 1},
		Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(1, 1), 40, 1, 1},
		Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(1, 1), 50, 1, 1},
		Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(1, 1), 60, 1, 1},

		// middle task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(8, 8), 30, 2, 2},
		Job<dtime_t>{2, Interval<dtime_t>(30, 30), Interval<dtime_t>(8, 8), 60, 2, 2},

		// the long task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(17, 17), 60, 3, 3}
	};

	SUBCASE("RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("P-RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(space.is_schedulable());
	}

	SUBCASE("P-RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore(jobs);
		CHECK(space.is_schedulable());
	}

}

// The example in Fig 2a of Nasri & Fohler (ECRTS 2016):
//    "Non-Work-Conserving Non-Preemptive Scheduling:
//     Motivations, Challenges, and Potential Solutions"
TEST_CASE("[IIP] P-RM negative example (Figure 2)")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task tau_1
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(3, 3), 10, 1, 1},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(3, 3), 20, 1, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(3, 3), 30, 1, 1},
		Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(3, 3), 40, 1, 1},
		Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(3, 3), 50, 1, 1},
		Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(3, 3), 60, 1, 1},

		// middle task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(6, 6), 12, 2, 2},
		Job<dtime_t>{2, Interval<dtime_t>(12, 12), Interval<dtime_t>(6, 6), 24, 2, 2},
		Job<dtime_t>{3, Interval<dtime_t>(24, 24), Interval<dtime_t>(6, 6), 36, 2, 2},
		Job<dtime_t>{4, Interval<dtime_t>(36, 36), Interval<dtime_t>(6, 6), 48, 2, 2},
		Job<dtime_t>{5, Interval<dtime_t>(48, 48), Interval<dtime_t>(6, 6), 60, 2, 2},

		// the long task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(8, 8), 60, 3, 3}
	};

	SUBCASE("RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("P-RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("P-RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

}

TEST_CASE("[IIP] P-RM example extra branch")
{
    Uniproc::State_space<dtime_t>::Workload jobs{
        Job<dtime_t>{0, Interval<dtime_t>( 1,  1), Interval<dtime_t>(1, 1), 3, 0, 0},
        Job<dtime_t>{1, Interval<dtime_t>(4, 4), Interval<dtime_t>(1, 1), 6, 0, 0},
        Job<dtime_t>{2, Interval<dtime_t>( 0,  0), Interval<dtime_t>(1, 2), 3, 1, 1},
        Job<dtime_t>{3, Interval<dtime_t>(2, 2), Interval<dtime_t>(3, 3), 6, 2, 2},
        };

    SUBCASE("RM, naive exploration") {
        auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
        CHECK(!space.is_schedulable());
        CHECK(space.number_of_states() == 5);
        CHECK(space.number_of_edges() == 4);
    }

    SUBCASE("RM, exploration with state-merging") {
        auto space = Uniproc::State_space<dtime_t>::explore(jobs);
        CHECK(!space.is_schedulable());
        CHECK(space.number_of_states() == 5);
        CHECK(space.number_of_edges() == 4);
    }

    SUBCASE("P-RM, naive exploration") {
        auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore_naively(jobs);
        CHECK(!space.is_schedulable());
        CHECK(space.number_of_states() == 7);
        CHECK(space.number_of_edges() == 6);
    }

    SUBCASE("P-RM, exploration with state-merging") {
        auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore(jobs);
        CHECK(!space.is_schedulable());
        CHECK(space.number_of_states() == 7);
        CHECK(space.number_of_edges() == 6);
    }

}

// The example in Fig 2b of Nasri & Fohler (ECRTS 2016):
//    "Non-Work-Conserving Non-Preemptive Scheduling:
//     Motivations, Challenges, and Potential Solutions"
TEST_CASE("[IIP] CW-EDF example (Figure 2)")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task tau_1
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(3, 3), 10, 10, 1},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(3, 3), 20, 20, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(3, 3), 30, 30, 1},
		Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(3, 3), 40, 40, 1},
		Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(3, 3), 50, 50, 1},
		Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(3, 3), 60, 60, 1},

		// middle task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(6, 6), 12, 12, 2},
		Job<dtime_t>{2, Interval<dtime_t>(12, 12), Interval<dtime_t>(6, 6), 24, 24, 2},
		Job<dtime_t>{3, Interval<dtime_t>(24, 24), Interval<dtime_t>(6, 6), 36, 36, 2},
		Job<dtime_t>{4, Interval<dtime_t>(36, 36), Interval<dtime_t>(6, 6), 48, 48, 2},
		Job<dtime_t>{5, Interval<dtime_t>(48, 48), Interval<dtime_t>(6, 6), 60, 60, 2},

		// the long task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(8, 8), 60, 60, 3}
	};

	SUBCASE("EDF, naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("EDF, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("CW-EDF, naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Critical_window_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(space.is_schedulable());
	}

	SUBCASE("CW-EDF, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Critical_window_IIP<dtime_t>>::explore(jobs);
		CHECK(space.is_schedulable());
	}

}


TEST_CASE("[IIP] CW-EDF extra example")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task tau_1
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(3, 3), 10, 10, 1},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(3, 3), 20, 20, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(3, 3), 30, 30, 1},
		Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(3, 3), 40, 40, 1},
		Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(3, 3), 50, 50, 1},
		Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(3, 3), 60, 60, 1},

		// middle task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(8, 8), 15, 15, 2},
		Job<dtime_t>{2, Interval<dtime_t>(15, 15), Interval<dtime_t>(8, 8), 30, 30, 2},
		Job<dtime_t>{3, Interval<dtime_t>(30, 30), Interval<dtime_t>(8, 8), 45, 45, 2},
		Job<dtime_t>{4, Interval<dtime_t>(45, 45), Interval<dtime_t>(8, 8), 60, 60, 2},

		// the long task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(8, 8), 60, 60, 3}
	};

	SUBCASE("EDF, naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("EDF, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("CW-EDF, naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Critical_window_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(space.is_schedulable());
	}

	SUBCASE("CW-EDF, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Critical_window_IIP<dtime_t>>::explore(jobs);
		CHECK(space.is_schedulable());
	}

}

TEST_CASE("[IIP] P-RM idle time")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(3, 3), 10, 1, 1},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(3, 3), 20, 1, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(3, 3), 30, 1, 1},

		// blocking job
		Job<dtime_t>{1, Interval<dtime_t>( 8,  8), Interval<dtime_t>(10, 10), 30, 2, 2},

		// low-priority job
		Job<dtime_t>{1, Interval<dtime_t>( 9,  9), Interval<dtime_t>(1, 1), 30, 3, 3},
	};

	SUBCASE("naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(space.is_schedulable());
	}

	SUBCASE("exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore(jobs);
		CHECK(space.is_schedulable());
	}

}
