#ifndef JOBS_HPP
#define JOBS_HPP

#include <ostream>
#include <vector>
#include <algorithm> // for find
#include <functional> // for hash
#include <exception>

#include "time.hpp"

//remove this to go back to the previous code!
#define GANG

#ifdef GANG
#define SINGLE_CORE 1
#endif

namespace NP {

	typedef std::size_t hash_value_t;

	struct JobID {
		unsigned long job;
		unsigned long task;

		JobID(unsigned long j_id, unsigned long t_id)
		: job(j_id), task(t_id)
		{
		}

		bool operator==(const JobID& other) const
		{
			return this->task == other.task && this->job == other.job;
		}

		friend std::ostream& operator<< (std::ostream& stream, const JobID& id)
		{
			stream << "T" << id.task << "J" << id.job;
			return stream;
		}
	};

#ifdef GANG
	//a class to hold S_min and S_max for each job
    struct Scores {
        unsigned long s_min;
        unsigned long s_max;

        Scores(unsigned long min, unsigned long max)
                : s_min(min), s_max(max)
        {
        }

        bool operator==(const Scores& other) const
        {
            return this->s_min == other.s_min && this->s_max == other.s_max;
        }

        friend std::ostream& operator<< (std::ostream& stream, const Scores& s)
        {
            stream << "S_min:" << s.s_min << ", S_max:" << s.s_max;
            return stream;
        }
    };
#endif

	template<class Time> class Job {

	public:
		typedef std::vector<Job<Time>> Job_set;
		typedef Time Priority; // Make it a time value to support EDF

	private:
		Interval<Time> arrival;
#ifdef GANG
        //Define s_min,s_max
        Scores scores;
        //0 position corresponds to s_min
        std::vector<Interval<Time>> costs;
#else
		Interval<Time> cost;
#endif
		Time deadline;
		Priority priority;
		JobID id;
		hash_value_t key;


		void compute_hash() {
			auto h = std::hash<Time>{};
			key = h(arrival.from());
			key = (key << 4) ^ h(id.task);
			key = (key << 4) ^ h(arrival.until());
#ifdef GANG
			//accumulate min costs only for hash
			key = (key << 4) ^ h(get_add_cost_min());
#else
			key = (key << 4) ^ h(cost.from());
#endif
			key = (key << 4) ^ h(deadline);
#ifdef GANG
            //accumulate max costs only for hash
			key = (key << 4) ^ h(get_add_cost_max());
#else
			key = (key << 4) ^ h(cost.upto());
#endif
			key = (key << 4) ^ h(id.job);
			key = (key << 4) ^ h(priority);
#ifdef GANG
			//adding s_min and s_max to hash of a job
            key = (key << 4) ^ h(scores.s_min);
            key = (key << 4) ^ h(scores.s_max);
#endif
		}

	public:

#ifdef GANG
        Job(unsigned long id,
            Interval<Time> arr, std::vector<Interval<Time>> &acosts,
            Time dl, Priority prio,
            unsigned long s_min, unsigned long s_max,
            unsigned long tid = 0)
                : arrival(arr),
                  costs(acosts),
                  deadline(dl), priority(prio), id(id, tid),scores(s_min, s_max)
        {
            compute_hash();
        }

		Job(unsigned long id,
			Interval<Time> arr, Interval<Time> cost,
			Time dl, Priority prio,
			unsigned long tid = 0)
		    : arrival(arr),
		    deadline(dl), priority(prio), id(id, tid)
		    ,scores(SINGLE_CORE, SINGLE_CORE)
		{
            costs.emplace_back(cost);
			compute_hash();
		}
#else
		Job(unsigned long id,
			Interval<Time> arr, Interval<Time> cost,
			Time dl, Priority prio,
			unsigned long tid = 0)
		: arrival(arr), cost(cost),
		  deadline(dl), priority(prio), id(id, tid)
		{
			compute_hash();
		}
#endif

		hash_value_t get_key() const
		{
			return key;
		}

		Time earliest_arrival() const
		{
			return arrival.from();
		}

		Time latest_arrival() const
		{
			return arrival.until();
		}

		const Interval<Time>& arrival_window() const
		{
			return arrival;
		}

#ifdef GANG
        //Generalise return cost depend on s
        Time least_cost(unsigned long s = SINGLE_CORE) const
        {
            //costs[0] corresponds to s_min
            assert((s - scores.s_min) >= 0 && ((s - scores.s_min) < costs.size()));
            return costs[s - scores.s_min].from();
        }
        //Generalise return cost depend on s
        Time maximal_cost(unsigned long s = SINGLE_CORE) const
        {
            //costs[0] corresponds to s_min
            assert((s - scores.s_min) >= 0 && ((s - scores.s_min) < costs.size()));
            return costs[s - scores.s_min].upto();
        }
        //Generalise return cost depend on s
        const Interval<Time>& get_cost(unsigned long s = SINGLE_CORE) const
        {
            //costs[0] corresponds to s_min
            assert((s - scores.s_min) >= 0 && ((s - scores.s_min) < costs.size()));
            return costs[s - scores.s_min];
        }

        //used only in compute hash of a job to accumulate one value of costs min
        Time get_add_cost_min() const
        {
		    Time acc = 0;
		    for(auto i: costs)
            {
		        acc += i.min();
            }
		    return acc;
        }
        //used only in compute hash of a job to accumulate one value of costs max
        Time get_add_cost_max() const
        {
            Time acc = 0;
            for(auto i: costs)
            {
                acc += i.max();
            }
            return acc;
        }
#else
		Time least_cost() const
		{
			return cost.from();
		}

		Time maximal_cost() const
		{
			return cost.upto();
		}

		const Interval<Time>& get_cost() const
		{
			return cost;
		}
#endif

		Priority get_priority() const
		{
			return priority;
		}

		Time get_deadline() const
		{
			return deadline;
		}

		bool exceeds_deadline(Time t) const
		{
			return t > deadline
			       && (t - deadline) >
			          Time_model::constants<Time>::deadline_miss_tolerance();
		}

		JobID get_id() const
		{
			return id;
		}

		unsigned long get_job_id() const
		{
			return id.job;
		}

		unsigned long get_task_id() const
		{
			return id.task;
		}

#ifdef GANG
        Scores get_scores() const
		{
			return scores;
		}

        unsigned long get_s_min() const
        {
            return scores.s_min;
        }

        unsigned long get_s_max() const
        {
            return scores.s_max;
        }
#endif

		bool is(const JobID& search_id) const
		{
			return this->id == search_id;
		}

		bool higher_priority_than(const Job &other) const
		{
			return priority < other.priority
			       // first tie-break by task ID
			       || (priority == other.priority
			           && id.task < other.id.task)
			       // second, tie-break by job ID
			       || (priority == other.priority
			           && id.task == other.id.task
			           && id.job < other.id.job);
		}

		bool priority_at_least_that_of(const Job &other) const
		{
			return priority <= other.priority;
		}

		bool priority_exceeds(Priority prio_level) const
		{
			return priority < prio_level;
		}

		bool priority_at_least(Priority prio_level) const
		{
			return priority <= prio_level;
		}

		Interval<Time> scheduling_window() const
		{
			// inclusive interval, so take off one epsilon
			return Interval<Time>{
			                earliest_arrival(),
			                deadline - Time_model::constants<Time>::epsilon()};
		}

		static Interval<Time> scheduling_window(const Job& j)
		{
			return j.scheduling_window();
		}

#ifdef GANG
        //in order to print all costs and s!
		friend std::ostream& operator<< (std::ostream& stream, const Job& j)
		{
			stream << "Job{" << j.id.task << ", " << j.id.job << ", " << j.arrival << ", ";
            for (auto i : j.costs)
			    stream << i << ", ";
			stream << j.deadline << ", " << j.priority
			       << ", " << j.scores << "}";
			return stream;
		}
#else
        friend std::ostream& operator<< (std::ostream& stream, const Job& j)
        {
            stream << "Job{" << j.id.job << ", " << j.arrival << ", "
                   << j.cost << ", " << j.deadline << ", " << j.priority
                   << ", " << j.id.task << "}";
            return stream;
        }
#endif

	};

	template<class Time>
	bool contains_job_with_id(const typename Job<Time>::Job_set& jobs,
	                          const JobID& id)
	{
		auto pos = std::find_if(jobs.begin(), jobs.end(),
		                        [id] (const Job<Time>& j) { return j.is(id); } );
		return pos != jobs.end();
	}

	class InvalidJobReference : public std::exception
	{
		public:

		InvalidJobReference(const JobID& bad_id)
		: ref(bad_id)
		{}

		const JobID ref;

		virtual const char* what() const noexcept override
		{
			return "invalid job reference";
		}

	};

	template<class Time>
	const Job<Time>& lookup(const typename Job<Time>::Job_set& jobs,
	                                 const JobID& id)
	{
		auto pos = std::find_if(jobs.begin(), jobs.end(),
		                        [id] (const Job<Time>& j) { return j.is(id); } );
		if (pos == jobs.end())
			throw InvalidJobReference(id);
		return *pos;
	}

}

namespace std {
	template<class T> struct hash<NP::Job<T>>
	{
		std::size_t operator()(NP::Job<T> const& j) const
		{
			return j.get_key();
		}
	};

	template<> struct hash<NP::JobID>
	{
		std::size_t operator()(NP::JobID const& id) const
		{
			hash<unsigned long> h;
			return (h(id.job) << 4) ^ h(id.task);
		}

	};
}

#endif
