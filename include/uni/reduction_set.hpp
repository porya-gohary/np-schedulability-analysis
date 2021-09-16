#ifndef MERGE_SET_H
#define MERGE_SET_H

#include <iostream>
#include <ostream>
#include <cassert>
#include <algorithm>
#include <map>

#include "jobs.hpp"

namespace NP {

	namespace Uniproc {

		template<class Time> class Reduction_set {
			public:

			typedef std::vector<const Job<Time>*> Job_set;
			typedef std::unordered_map<JobID, Time> Job_map;
			typedef typename Job<Time>::Priority Priority;

			private:

			Interval<Time> cpu_availability;
			Job_set jobs;
			Job_set jobs_by_latest_arrival;
			Job_set jobs_by_earliest_arrival;
			Job_set jobs_by_wcet;
			Time latest_busy_time;
			Time latest_idle_time;
			Job_map latest_start_times;
			hash_value_t key;
			Priority max_priority;
			unsigned long num_interfering_jobs_added;

			public:

			Reduction_set(Interval<Time> cpu_availability, const Job_set &jobs)
			: cpu_availability{cpu_availability},
			jobs{jobs},
			jobs_by_latest_arrival{jobs},
			jobs_by_earliest_arrival{jobs},
			jobs_by_wcet{jobs},
			key{0},
			num_interfering_jobs_added{0}
			{
				std::sort(jobs_by_latest_arrival.begin(), jobs_by_latest_arrival.end(),
						  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->latest_arrival() < j->latest_arrival(); });

				std::sort(jobs_by_earliest_arrival.begin(), jobs_by_earliest_arrival.end(),
						  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->earliest_arrival() < j->earliest_arrival(); });

				std::sort(jobs_by_wcet.begin(), jobs_by_wcet.end(),
						  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->maximal_cost() < j->maximal_cost(); });

				latest_busy_time = compute_latest_busy_time();
				latest_idle_time = compute_latest_idle_time();
				latest_start_times = compute_latest_start_times();
				max_priority = compute_max_priority();
				initialize_key();
			}

			Job_set get_jobs() const {
				return jobs;
			}

			Time get_latest_busy_time() const {
				return latest_busy_time;
			}

			Time get_latest_idle_time() const {
				return latest_idle_time;
			}

			Job_map get_latest_start_times() const {
				return latest_start_times;
			}

			Time get_latest_start_time(const Job<Time> &job) const {
				auto iterator = latest_start_times.find(job.get_id());
				return iterator == latest_start_times.end() ? -1 : iterator->second;
			}

			bool has_potential_deadline_misses() const {
				for (const Job<Time>* job : jobs) {
					if (job->exceeds_deadline(get_latest_start_time(*job) + job->maximal_cost())) {
						return true;
					}
				}

				return false;
			}

			bool can_interfere(const Job<Time> &job) const {
				auto pos = std::find_if(jobs.begin(), jobs.end(),
										[&job] (const Job<Time>* j) { return j->get_id() == job.get_id(); } );

				// A job can't interfere with itself
				if(pos != jobs.end()) {
					return false;
				}

				// rx_min <= l_M
				if (job.earliest_arrival() <= latest_idle_time) {
					return true;
				}

				Time min_wcet = min_lower_priority_wcet(job);
				Time max_arrival = jobs_by_latest_arrival.back()->latest_arrival();

				if (!job.priority_exceeds(max_priority) && job.earliest_arrival() >= max_arrival) {
					return false;
				}

				// There exists a J_i s.t. rx_min <= s_i and p_x < p_i
				for (const Job<Time>* j : jobs) {
					if (job.earliest_arrival() <= get_latest_start_time(*j) && job.higher_priority_than(*j)) {
						return true;
					}
				}

				return false;
			}

			void add_job(const Job<Time>* jx) {
				num_interfering_jobs_added++;
				jobs.push_back(jx);
				insert_sorted(jobs_by_latest_arrival, jx,
							  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->latest_arrival() < j->latest_arrival(); });
				insert_sorted(jobs_by_earliest_arrival, jx,
							  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->earliest_arrival() < j->earliest_arrival(); });
				insert_sorted(jobs_by_wcet, jx,
							  [](const Job<Time>* i, const Job<Time>* j) -> bool { return i->maximal_cost() < j->maximal_cost(); });

				latest_busy_time = compute_latest_busy_time();
				latest_idle_time = compute_latest_idle_time();
				latest_start_times = compute_latest_start_times();
				key = key ^ jx->get_key();

				if (!jx->priority_at_least(max_priority)) {
					max_priority = jx->get_priority();
				}
			}

			Time earliest_start_time() const {
				return std::max(cpu_availability.min(), (*jobs_by_latest_arrival.begin())->earliest_arrival());
			}

			Time earliest_finish_time() const {
				Time t = cpu_availability.min();

				for(const Job<Time>* j : jobs_by_earliest_arrival) {
					t = std::max(t, j->earliest_arrival()) + j->least_cost();
				}

				return t;
			}

			Time earliest_finish_time(const Job<Time> &job) const {
				return std::max(cpu_availability.min(), job.earliest_arrival()) + job.least_cost();
			}

			Time latest_start_time() const {
				return std::max(cpu_availability.max(), (*jobs_by_latest_arrival.begin())->latest_arrival());
			}

			Time latest_finish_time(const Job<Time> &job) const {
				return get_latest_start_time(job) + job.maximal_cost();
			}

			Time get_min_wcet() const {
				return jobs_by_wcet.front()->maximal_cost();
			}

			hash_value_t get_key() const {
				return key;
			}

			unsigned long get_num_interfering_jobs_added() const {
				return num_interfering_jobs_added;
			}

			private:

			Time compute_latest_busy_time() {
				Time t = cpu_availability.max();

				for(const Job<Time>* j : jobs_by_latest_arrival) {
					t = std::max(t, j->latest_arrival()) + j->maximal_cost();
				}

				return t;
			}

			Job_map compute_latest_start_times() {
				Job_map start_times{};
				for (const Job<Time>* j : jobs) {
					start_times.emplace(j->get_id(), compute_latest_start_time(*j));
				}

				return start_times;
			}

			Time compute_latest_start_time(const Job<Time> &i) {
				const Job<Time>* blocking_job = nullptr;

				for (const Job<Time>* j : jobs_by_earliest_arrival) {
					if (j->get_id() == i.get_id()) {
						continue;
					}

					if (i.higher_priority_than(*j) && (blocking_job == nullptr || blocking_job->maximal_cost() < j->maximal_cost())) {
						blocking_job = j;
					}
				}

				Time blocking_time = blocking_job == nullptr ? 0 : std::max<Time>(0, blocking_job->maximal_cost() - Time_model::constants<Time>::epsilon());
				Time latest_start_time = std::max(cpu_availability.max(), i.latest_arrival() + blocking_time);

				for (const Job<Time>* j : jobs_by_earliest_arrival) {
					if (j->get_id() == i.get_id()) {
						continue;
					}

					if (j->earliest_arrival() <= latest_start_time && j->higher_priority_than(i)) {
						latest_start_time += j->maximal_cost();
					} else if (j->earliest_arrival() > latest_start_time) {
						break;
					}
				}

				return std::min(latest_start_time, latest_busy_time - i.maximal_cost());
			}

			Time compute_latest_idle_time() {
				Time idle_time{-1};
				const Job<Time>* idle_job = nullptr;

				auto first_job_after_eft = std::find_if(jobs_by_latest_arrival.begin(), jobs_by_latest_arrival.end(),
														[this] (const Job<Time>* j) { return j->latest_arrival() > cpu_availability.min(); } );

				// No j with r_max > A1_min, so no idle time before the last job in job_set
				if (first_job_after_eft == jobs_by_latest_arrival.end()) {
					return idle_time;
				}

				for (const Job<Time>* i : jobs_by_latest_arrival) {
					// compute the earliest time at which the set of all jobs with r^max < r_i^max can complete
					Time t = cpu_availability.min();

					for (const Job<Time>* j : jobs_by_earliest_arrival) {
						if ((j->latest_arrival() < i->latest_arrival())) {
							t = std::max(t, j->earliest_arrival()) + j->least_cost();
						}

						if (t >= i->latest_arrival()) {
							break;
						}
					}

					if (t < i->latest_arrival()) {
						if (idle_job == nullptr || i->latest_arrival() > idle_job->latest_arrival()) {
							idle_job = i;
						}
					}
				}

				if (idle_job == nullptr) {
					return idle_time;
				}

				if (idle_job->latest_arrival() == (*jobs_by_latest_arrival.begin())->latest_arrival()) {
					return idle_time;
				} else {
					return idle_job->latest_arrival() - Time_model::constants<Time>::epsilon();
				}
			}

			Priority compute_max_priority() const {
				Priority max_prio {};

				for (const Job<Time>* j : jobs) {
					if (!j->priority_exceeds(max_prio)) {
						max_prio = j->get_priority();
					}
				}

				return max_prio;
			}

			// Returns the smallest wcet among the jobs with a lower priority than job
			Time min_lower_priority_wcet(const Job<Time> &job) const {
				auto pos = std::find_if(jobs_by_wcet.begin(), jobs_by_wcet.end(),
										[&job] (const Job<Time>* j) { return job.higher_priority_than(*j); } );

				if (pos == jobs_by_wcet.end()) {
					return 0;
				} else {
					return (*pos)->maximal_cost();
				}
			}

			void initialize_key() {
				for (const Job<Time>* j : jobs) {
					key = key ^ j->get_key();
				}
			}


		};

		template<typename T, typename Compare>
		typename std::vector<T>::iterator insert_sorted(std::vector<T> &vec, const T &item, Compare comp) {
			return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, comp), item);
		}

		template<class Time> class Reduction_set_statistics {

			public:

			bool reduction_success;

			unsigned long num_jobs, num_interfering_jobs_added;

			std::vector<Time> priorities;

			Reduction_set_statistics(bool reduction_success, Reduction_set<Time> &reduction_set)
			: reduction_success{reduction_success},
			  num_jobs{reduction_set.get_jobs().size()},
			  num_interfering_jobs_added{reduction_set.get_num_interfering_jobs_added()},
			  priorities{}
			{
				for (const Job<Time>* j : reduction_set.get_jobs()) {
					priorities.push_back(j->get_priority());
				}
			}
		};
	}
}

#endif
