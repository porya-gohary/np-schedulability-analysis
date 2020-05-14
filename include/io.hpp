#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <utility>

#include "interval.hpp"
#include "time.hpp"
#include "jobs.hpp"
#include "precedence.hpp"
#include "aborts.hpp"

namespace NP {

	inline void skip_over(std::istream& in, char c)
	{
		while (in.good() && in.get() != (int) c)
			/* skip */;
	}

	inline bool skip_one(std::istream& in, char c)
	{
		if (in.good() && in.peek() == (int) c) {
			in.get(); /* skip */
			return true;
		} else
			return false;
	}

	inline void skip_all(std::istream& in, char c)
	{
		while (skip_one(in, c))
			/* skip */;
	}

	inline bool more_data(std::istream& in)
	{
		in.peek();
		return !in.eof();
	}

    //return true if a field separator is found
    inline bool next_field(std::istream& in)
    {
        // eat up any trailing spaces
        skip_all(in, ' ');
        // eat up field separator
        return skip_one(in, ',');
    }

    //return true if a field separator is found
    inline bool next_field_colon(std::istream& in)
    {
        // eat up any trailing spaces
        skip_all(in, ' ');
        // eat up field separator
        return skip_one(in, ':');
    }

	inline void next_line(std::istream& in)
	{
		skip_over(in, '\n');
	}

	inline JobID parse_job_id(std::istream& in)
	{
		unsigned long jid, tid;

		in >> tid;
		next_field(in);
		in >> jid;
		return JobID(jid, tid);
	}

	inline Precedence_constraint parse_precedence_constraint(std::istream &in)
	{
		std::ios_base::iostate state_before = in.exceptions();
		in.exceptions(std::istream::failbit | std::istream::badbit);

		// first two columns
		auto from = parse_job_id(in);

		next_field(in);

		// last two columns
		auto to = parse_job_id(in);

		in.exceptions(state_before);

		return Precedence_constraint(from, to);
	}

	inline Precedence_constraints parse_dag_file(std::istream& in)
	{
		Precedence_constraints edges;

		// skip column headers
		next_line(in);

		// parse all rows
		while (more_data(in)) {
			// each row contains one precedence constraint
			edges.push_back(parse_precedence_constraint(in));
			next_line(in);
		}

		return edges;
	}

	template<class Time> Job<Time> parse_job(std::istream& in)
	{
		unsigned long tid, jid;

		std::ios_base::iostate state_before = in.exceptions();

		Time arr_min, arr_max, cost_min, cost_max, dl, prio;
        std::vector<Interval<Time>> costs;
        //temporary vectors
        std::vector<Time> costs_min;
        std::vector<Time> costs_max;
        //if s_min and s_max is not found set to 1
        unsigned long s_min = SINGLE_CORE, s_max = SINGLE_CORE;

		in.exceptions(std::istream::failbit | std::istream::badbit);

		in >> tid;
		next_field(in);
		in >> jid;
		next_field(in);
		in >> arr_min;
		next_field(in);
		in >> arr_max;

        //Generalization to get all costs
        //eat comma
        next_field(in);
        in >> cost_min;
        costs_min.emplace_back(cost_min);
        //eat colon
        //get all costs min
        while(next_field_colon(in))
        {
            in >> cost_min;
            costs_min.emplace_back(cost_min);
        }
        //eat comma
        next_field(in);
        in >> cost_max;
        costs_max.emplace_back(cost_max);
        //eat colon
        //get all costs max
        while(next_field_colon(in))
        {
            in >> cost_max;
            costs_max.emplace_back(cost_max);
        }

        //check the size of costs min must be equal to costs max
        assert(costs_min.size() == costs_max.size());

        for(auto i=0;i<costs_min.size();i++)
            costs.emplace_back(costs_min[i],costs_max[i]);
        next_field(in);
        in >> dl;
        next_field(in);
        in >> prio;

        //parse s_min,s_max if exists
		if(next_field(in))
		    in >> s_min;
        if(next_field(in))
            in >> s_max;

        in.exceptions(state_before);

        //check for correctness
		assert(s_max > 0 && s_min > 0);
		assert(s_max >= s_min);
		//costs must be equal to the difference of s_min and s_max
		assert("Parse file error: Execution times does not match the number of parallelism, " && costs.size() == ((s_max-s_min)+SINGLE_CORE) );
		//display warning if the execution times between more cores are larger than less cores
		for(int i=1;i<costs.size();i++)
        {
		    if ((costs[i-1].from() < costs[i].from()) || (costs[i-1].upto() < costs[i].upto()))
            {
		        std::cerr << "Warning T" << std::to_string(tid) << "J" << jid <<
		        " Execution times between more cores are larger than less cores" << std::endl;
		        break;
            }
        }

		//create the Job
		//Only one constructor with S_min and S_max to 1
		return Job<Time>{jid, Interval<Time>{arr_min, arr_max},
                              costs, dl, prio, s_min, s_max, tid};
	}

	template<class Time>
	typename Job<Time>::Job_set parse_file(std::istream& in)
	{
		// first row contains a comment, just skip it
		next_line(in);

		typename Job<Time>::Job_set jobs;

		while (more_data(in)) {
			jobs.push_back(parse_job<Time>(in));
			// Mung any trailing whitespace or extra columns
			next_line(in);
		}

		return jobs;
	}

	template<class Time>
	Abort_action<Time> parse_abort_action(std::istream& in)
	{
		unsigned long tid, jid;
		Time trig_min, trig_max, cleanup_min, cleanup_max;

		std::ios_base::iostate state_before = in.exceptions();

		in.exceptions(std::istream::failbit | std::istream::badbit);

		in >> tid;
		next_field(in);
		in >> jid;
		next_field(in);
		in >> trig_min;
		next_field(in);
		in >> trig_max;
		next_field(in);
		in >> cleanup_min;
		next_field(in);
		in >> cleanup_max;

		in.exceptions(state_before);

		return Abort_action<Time>{JobID{jid, tid},
		                          Interval<Time>{trig_min, trig_max},
		                          Interval<Time>{cleanup_min, cleanup_max}};
	}


	template<class Time>
	std::vector<Abort_action<Time>> parse_abort_file(std::istream& in)
	{
		// first row contains a comment, just skip it
		next_line(in);

		std::vector<Abort_action<Time>> abort_actions;

		while (more_data(in)) {
			abort_actions.push_back(parse_abort_action<Time>(in));
			// Mung any trailing whitespace or extra columns
			next_line(in);
		}

		return abort_actions;
	}


}

#endif
