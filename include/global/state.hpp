#include <iostream>
#include <ostream>
#include <cassert>
#include <algorithm>

#include <set>

#include "util.hpp"
#include "index_set.hpp"
#include "jobs.hpp"
#include "cache.hpp"

#include "config.h"

namespace NP {

	namespace Global {

		typedef std::size_t Job_index;
		typedef std::vector<Job_index> Job_precedence_set;

		template<class Time> class Schedule_state
		{
			public:

			// initial state -- nothing yet has finished, nothing is running
			Schedule_state(unsigned int num_processors)
			: scheduled_jobs()
			, num_jobs_scheduled(0)
			, core_avail{num_processors, Interval<Time>(Time(0), Time(0))}
			, lookup_key{0x9a9a9a9a9a9a9a9aUL}
			{
				assert(core_avail.size() > 0);
			}

			// transition: new state by scheduling a job in an existing state,
			//             by replacing a given running job.
			Schedule_state(
				const Schedule_state& from,
				Job_index j,
				const Job_precedence_set& predecessors,
				Interval<Time> start_times,
				Interval<Time> finish_times,
				hash_value_t key
#ifdef GANG
                , unsigned int p = SINGLE_CORE
#endif
            )
			: num_jobs_scheduled(from.num_jobs_scheduled + 1)
			, scheduled_jobs{from.scheduled_jobs, j}
			, lookup_key{from.lookup_key ^ key}
			{
			    //gang -> est for p cores
				auto est = start_times.min();
				auto lst = start_times.max();
                //gang -> eft for p cores
				auto eft = finish_times.min();
                //gang -> lft for p cores
				auto lft = finish_times.max();

				DM("est: " << est << std::endl
				<< "lst: " << lst << std::endl
				<< "eft: " << eft << std::endl
				<< "lft: " << lft << std::endl);

#ifdef GANG
				//initialise CA with p times lft elements
				//initialise PA with p times eft elements
                std::vector<Time> ca(p,lft), pa(p,eft);

                // skip p elements in from.core_avail
                for (unsigned int i = p; i < from.core_avail.size(); i++) {
					pa.push_back(std::max(est, from.core_avail[i].min()));
#ifndef FIX_NEW_STATE_GANG
					ca.push_back(std::max(est, from.core_avail[i].max()));
#endif
				}

#ifdef FIX_NEW_STATE_GANG
                unsigned int sum_px = 0;
#endif
                // update scheduled jobs
                // map is already sorted to make it easier to merge
                //TODO optimize structure
                bool added_j = false;
                for (const auto& rj : from.certain_jobs) {
                    auto x = rj.first;
                    //rj->second.first = Interval<Time> EFT,LFT
                    auto x_eft = rj.second.first.min();
                    auto x_lft = rj.second.first.max();

                    if (contains(predecessors, x)) {
#ifdef FIX_NEW_STATE_GANG
                        sum_px += rj.second.second;
#else
                        //old analysis
                        if (lst < x_lft) {
                            //rj->second.second = minimum p
                            for(auto l=1; l <= rj.second.second;l++) {
                                auto pos = std::find(ca.begin(), ca.end(), x_lft);
                                if (pos != ca.end())
                                    *pos = lst;
                            }
                        }
#endif
                    } else if (lst <= x_eft) {
                        if (!added_j && rj.first > j) {
                            // right place to add j
                            certain_jobs.emplace(j, std::make_pair(finish_times, p));
                            added_j = true;
                        }
                        certain_jobs.emplace(rj);
                    }
                }
                // if we didn't add it yet, add it at the back
                if (!added_j)
                    //TODO: emplace -> insert element only if key is never added before added can safely removed
                    certain_jobs.emplace(j, std::make_pair(finish_times, p));

#ifdef FIX_NEW_STATE_GANG

                DM("sum_p : " << sum_px << std::endl);

                auto m_pred = std::max(p, sum_px);
                DM("m_pred : " << m_pred << std::endl);

                for (unsigned int i = m_pred; i < from.core_avail.size(); i++) {
                    ca.push_back(std::max(est, from.core_avail[i].max()));
                }

                for (unsigned int i = p; i < m_pred; i++) {
                    ca.push_back(std::min(lst, std::max(est, from.core_avail[i].max())));
                }
#endif

#else
                std::vector<Time> ca, pa;

				pa.push_back(eft);
				ca.push_back(lft);

				// skip first element in from.core_avail
				for (int i = 1; i < from.core_avail.size(); i++) {
					pa.push_back(std::max(est, from.core_avail[i].min()));
#ifndef FIX_NEW_STATE
					ca.push_back(std::max(est, from.core_avail[i].max()));
#endif
				}

#ifdef FIX_NEW_STATE
                //sum of p predececors
                unsigned int sum_px = 0;
#endif

				// update scheduled jobs
				// keep it sorted to make it easier to merge
				bool added_j = false;
				for (const auto& rj : from.certain_jobs) {
					auto x = rj.first;
					auto x_eft = rj.second.min();
					auto x_lft = rj.second.max();
					if (contains(predecessors, x)) {
#ifdef FIX_NEW_STATE
                        sum_px++; //only one core!
#else
						if (lst < x_lft) {
							auto pos = std::find(ca.begin(), ca.end(), x_lft);
							if (pos != ca.end())
								*pos = lst;
						}
#endif
					} else if (lst <= x_eft) {
						if (!added_j && rj.first > j) {
							// right place to add j
							certain_jobs.emplace_back(j, finish_times);
							added_j = true;
						}
						certain_jobs.emplace_back(rj);
					}
				}
				// if we didn't add it yet, add it at the back
				if (!added_j)
					certain_jobs.emplace_back(j, finish_times);

#ifdef FIX_NEW_STATE
				unsigned int m_pred = std::max(1u,sum_px);
                //2nd union
                for (auto i = 1; i < m_pred; i++) {
                    ca.push_back(std::min(lst, std::max(est, from.core_avail[i].max())));
                }
                //3rd union
                for (auto i = m_pred; i < from.core_avail.size(); i++) {
                    ca.push_back(std::max(est, from.core_avail[i].max()));
                }
#endif

#endif

				// sort in non-decreasing order
				std::sort(pa.begin(), pa.end());
				std::sort(ca.begin(), ca.end());

				for (int i = 0; i < from.core_avail.size(); i++) {
					DM(i << " -> " << pa[i] << ":" << ca[i] << std::endl);
					core_avail.emplace_back(pa[i], ca[i]);
				}

				assert(core_avail.size() > 0);
				DM("*** new state: constructed " << *this << std::endl);
			}

			hash_value_t get_key() const
			{
				return lookup_key;
			}

			bool same_jobs_scheduled(const Schedule_state &other) const
			{
				return scheduled_jobs == other.scheduled_jobs;
			}

			bool can_merge_with(const Schedule_state<Time>& other) const
			{
				assert(core_avail.size() == other.core_avail.size());

				if (get_key() != other.get_key())
					return false;
				if (!same_jobs_scheduled(other))
					return false;
				for (int i = 0; i < core_avail.size(); i++)
					if (!core_avail[i].intersects(other.core_avail[i]))
						return false;
				return true;
			}

			bool try_to_merge(const Schedule_state<Time>& other)
			{
				if (!can_merge_with(other))
					return false;

				for (int i = 0; i < core_avail.size(); i++)
					core_avail[i] |= other.core_avail[i];

#ifdef GANG
                // map to collect joint certain jobs
                std::map<Job_index, std::pair<Interval<Time>, unsigned int>> new_cj;

                // walk both sorted job lists to see if we find matches
                auto it = certain_jobs.begin();
                auto jt = other.certain_jobs.begin();
                while (it != certain_jobs.end() &&
                       jt != other.certain_jobs.end()) {
                    if (it->first == jt->first) {
                        // same job
                        new_cj.emplace(it->first,
                                //merge EFT,LFT
                                std::make_pair(it->second.first | jt->second.first,
                                        //merge p (minimum of both)
                                        std::min(it->second.second,jt->second.second)));
                        it++;
                        jt++;
                    } else if (it->first < jt->first)
                        it++;
                    else
                        jt++;
                }
                // move new certain jobs into the state
                //TODO: investigate if it is safe??? new_cj is deleted!!!
                certain_jobs.swap(new_cj);

#else
                // vector to collect joint certain jobs
				std::vector<std::pair<Job_index, Interval<Time>>> new_cj;

				// walk both sorted job lists to see if we find matches
				auto it = certain_jobs.begin();
				auto jt = other.certain_jobs.begin();
				while (it != certain_jobs.end() &&
				       jt != other.certain_jobs.end()) {
					if (it->first == jt->first) {
						// same job
						new_cj.emplace_back(it->first, it->second | jt->second);
						it++;
						jt++;
					} else if (it->first < jt->first)
						it++;
					else
						jt++;
				}
				// move new certain jobs into the state
				certain_jobs.swap(new_cj);
#endif
				DM("+++ merged " << other << " into " << *this << std::endl);

				return true;
			}

			const unsigned int number_of_scheduled_jobs() const
			{
				return num_jobs_scheduled;
			}

#ifdef GANG
            //return number of available processors depending on core_avail size
            const unsigned int num_processors() const
            {
                assert(core_avail.size() > 0);
                return core_avail.size();
            }

            //return core availability for p processors
            //if p is greater than the number of available processors then
            //return infinity -> 1 if no argument is called
            Interval<Time> core_availability(unsigned long p = SINGLE_CORE) const
            {
                assert(core_avail.size() > 0);
                assert((p-1) >= 0);
                return (p-1) <= core_avail.size() ? core_avail[p-1]:
                       Interval<Time>{Time_model::constants<Time>::infinity(), Time_model::constants<Time>::infinity()};;
            }
#else
			Interval<Time> core_availability() const
			{
				assert(core_avail.size() > 0);
				return core_avail[0];
			}
#endif

#ifdef GANG
            //return the finishing time depending on index j
            //certain jobs is now a map not a vector
            bool get_finish_times(Job_index j, Interval<Time> &ftimes) const
            {
                auto rj = certain_jobs.find(j);
                if( rj == certain_jobs.end())
                {
                    return false;
                }
                else
                {
                    ftimes = rj->second.first;
                    return true;
                }
            }
#else
			bool get_finish_times(Job_index j, Interval<Time> &ftimes) const
			{
				for (const auto& rj : certain_jobs) {
					// check index
					if (j == rj.first) {
						ftimes = rj.second;
						return true;
					}
					// Certain_jobs is sorted in order of increasing job index.
					// If we see something larger than 'j' we are not going
					// to find it. For large processor counts, it might make
					// sense to do a binary search instead.
					if (j < rj.first)
						return false;
				}
				return false;
			}
#endif

			const bool job_incomplete(Job_index j) const
			{
				return !scheduled_jobs.contains(j);
			}

			const bool job_ready(const Job_precedence_set& predecessors) const
			{
				for (auto j : predecessors)
					if (!scheduled_jobs.contains(j))
						return false;
				return true;
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Schedule_state<Time>& s)
			{
				stream << "Global::State(";
				for (const auto& a : s.core_avail)
					stream << "[" << a.from() << ", " << a.until() << "] ";
				stream << "(";
#ifdef GANG
				//print certainly running jobs
                for (auto rj = s.certain_jobs.begin(); rj != s.certain_jobs.end(); rj++)
                    stream << rj->first << "";
#else
				for (const auto& rj : s.certain_jobs)
					stream << rj.first << "";
#endif
				stream << ") " << s.scheduled_jobs << ")";
				stream << " @ " << &s;
				return stream;
			}

			void print_vertex_label(std::ostream& out,
				const typename Job<Time>::Job_set& jobs) const
			{
				for (const auto& a : core_avail)
					out << "[" << a.from() << ", " << a.until() << "] ";
				out << "\\n";
				bool first = true;
				out << "{";
#ifdef GANG
				//print certainly running jobs in the graph
                for (auto rj = certain_jobs.begin(); rj != certain_jobs.end(); rj++) {
                    if (!first)
                        out << ", ";
                    out << "T" << jobs[rj->first].get_task_id()
                        << "J" << jobs[rj->first].get_job_id() << ":"
                        << rj->second.first.min() << "-" << rj->second.first.max() << ":" << rj->second.second;
                    first = false;
                }
#else
				for (const auto& rj : certain_jobs) {
					if (!first)
						out << ", ";
					out << "T" << jobs[rj.first].get_task_id()
					    << "J" << jobs[rj.first].get_job_id() << ":"
					    << rj.second.min() << "-" << rj.second.max();
					first = false;
				}
#endif
				out << "}";
			}

			private:

			const unsigned int num_jobs_scheduled;

			// set of jobs that have been dispatched (may still be running)
			const Index_set scheduled_jobs;

#ifdef GANG
            // imprecise set of certainly running jobs including p
            std::map<Job_index,std::pair<Interval<Time>, unsigned int>> certain_jobs;
#else
			// imprecise set of certainly running jobs
			std::vector<std::pair<Job_index, Interval<Time>>> certain_jobs;
#endif

			// system availability intervals
			std::vector<Interval<Time>> core_avail;

			const hash_value_t lookup_key;

			// no accidental copies
			Schedule_state(const Schedule_state& origin)  = delete;
		};

	}
}
