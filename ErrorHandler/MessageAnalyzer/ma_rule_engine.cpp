
#include "ErrorHandler/MessageAnalyzer/ma_rule_engine.h"

#include <cetlib/filepath_maker.h>
#include <fhiclcpp/make_ParameterSet.h>

using fhicl::ParameterSet;

using namespace novadaq::errorhandler;

ma_rule_engine::ma_rule_engine(fhicl::ParameterSet const& pset, alarm_fn_t alarm, cond_match_fn_t cond_match)
    : pset(pset)
    , cmap()
    , cnames()
    , rmap()
    , rnames()
    , alarm_fn(alarm)
    , cond_match_fn(cond_match)
    , events()
    , event_worker_t()
    , EHS(false)
{
	init_engine();
}

void ma_rule_engine::init_engine()
{
	// Error Handling Supervisor -- EHS
	EHS = pset.get<bool>("EHS", false);

	ParameterSet conds = pset.get<ParameterSet>("conditions", fhicl::ParameterSet());
	ParameterSet rules = pset.get<ParameterSet>("rules", fhicl::ParameterSet());

	cnames = conds.get_pset_names();
	rnames = rules.get_pset_names();

	// go through all conditions
	for (size_t i = 0; i < cnames.size(); ++i)
	{
		ParameterSet nulp;

		ParameterSet cond = conds.get<ParameterSet>(cnames[i]);
		ParameterSet rate = cond.get<ParameterSet>("rate", nulp);
		ParameterSet gran = cond.get<ParameterSet>("granularity", nulp);

		// decide whether "at_least" or "at_most"
		int occur{0}, occur_at_most{0}, occur_at_least{0};
		bool at_most = rate.get_if_present<int>("occur_at_most", occur_at_most);
		bool at_least = rate.get_if_present<int>("occur_at_least", occur_at_least);

		// compatible with the previous "occurence" keyword
		if (!at_least) at_least = rate.get_if_present<int>("occurence", occur_at_least);

		if (at_most && at_least)
		{
			throw std::runtime_error(
			    "rule_engine::init_engine() cannot have both 'occur_at_least' "
			    "and 'occur_at_most' in the rate entry, in condition " +
			    cnames[i]);
		}
		else if (!at_most && !at_least)
		{
			occur = 1;
			at_least = true;
		}
		else
		{
			occur = at_least ? occur_at_least : occur_at_most;
		}

		// construct the condition object
		ma_condition c(cond.get<std::string>("description", std::string()), cond.get<std::string>("severity"), cond.get<std::vector<std::string>>("source"),
		               cond.get<std::vector<std::string>>("category", std::vector<std::string>(1, "*")), cond.get<std::string>("regex"), cond.get<std::string>("test", std::string()),
		               cond.get<bool>("persistent", true), occur, at_least, rate.get<int>("timespan", 4372596)  // a random long time
		               ,
		               gran.get<bool>("per_source", false), gran.get<bool>("per_target", false), gran.get<unsigned int>("target_group", 1), events);

		// push the condition to the container, and parse the test function
		cond_map_t::iterator it = cmap.insert(std::make_pair(cnames[i], c)).first;

		// init after the condition has been inserted into the map
		it->second.init();
	}

	// go through all rules
	for (size_t i = 0; i < rnames.size(); ++i)
	{
		ParameterSet nulp;

		ParameterSet rule = rules.get<ParameterSet>(rnames[i]);

		// construct the rule object
		ma_rule r(rnames[i], rule.get<std::string>("description", std::string()), rule.get<bool>("repeat_alarm", false), rule.get<int>("holdoff", 0));

		// push the rule to the container
		rule_map_t::iterator it = rmap.insert(std::make_pair(rnames[i], r)).first;

		// parse the condition expression and alarm message
		// this is done in a two-step method (init the object, push into container
		// then parse) because the parse process involves updating the conditions
		// notification list which needs the pointer to the ma_rule object. There-
		// fore we push the ma_rule object into the container first, then do the
		// parse
		it->second.parse(rule.get<std::string>("expression"), rule.get<std::string>("message"), rule.get<ParameterSet>("action", nulp), &cmap);
	}

	// for all conditions sort their notification lists
	cond_map_t::iterator it = cmap.begin();
	for (; it != cmap.end(); ++it) it->second.sort_notify_lists();

	// timing event worker thread
	event_worker_t = boost::thread(&ma_rule_engine::event_worker, this);
}

void ma_rule_engine::event_worker()
{
	while (true)
	{
		// get current second
		time_t now = time(0);

		// loop for all past due events, and execute them
		{
			// scoped lock
			boost::mutex::scoped_lock lock(events.lock);

			conds_t status;
			event_queue_t& eq = events.event_queue();

			while (!eq.empty() && eq.top().timestamp() < now)
			{
				ma_timing_event const& e = eq.top();
				e.condition().event(e.source_idx(), e.target_idx(), e.timestamp(), status);
				eq.pop();
			}

			// build notify list
			notify_list_t notify_status;
			merge_notify_list(notify_status, status, STATUS_NOTIFY);

			// rules->evaluate
			evaluate_rules(notify_status);
		}

		sleep(1);
	}
}

void ma_rule_engine::feed(qt_mf_msg const& msg)
{
	// reaction starters
	conds_t status;
	conds_t source;
	conds_t target;

	// loop through conditions
	{
		cond_map_t::iterator it = cmap.begin();
		for (; it != cmap.end(); ++it)
			if (it->second.match(msg, status, source, target))
				cond_match_fn(it->first);  // callback fn for condition match
	}

	// notification mechanism

	// merge notification lists from reaction starters
	notify_list_t notify_status;
	notify_list_t notify_domain;

	merge_notify_list(notify_status, status, STATUS_NOTIFY);
	merge_notify_list(notify_domain, source, SOURCE_NOTIFY);
	merge_notify_list(notify_domain, target, TARGET_NOTIFY);

	// update domains
	evaluate_rules_domain(notify_domain);

	// loop to update status
	evaluate_rules(notify_status);
}

void ma_rule_engine::evaluate_rules_domain(notify_list_t& notify_domain)
{
	notify_list_t::iterator it = notify_domain.begin();
	for (; it != notify_domain.end(); ++it)
		(*it)->evaluate_domain();
}

void ma_rule_engine::evaluate_rules(notify_list_t& notify_status)
{
	notify_list_t::iterator it = notify_status.begin();
	for (; it != notify_status.end(); ++it)
	{
		if ((*it)->evaluate())
		{
			// alarm message
			alarm_fn((*it)->name(), (*it)->get_alarm_message());

			// actions
			if (EHS)
			{
				int now_reset_rule = (*it)->act();
				if (now_reset_rule > 0)
				{
					this->reset_rule((*it)->name());
					alarm_fn((*it)->name(), "reseting this rule!");
				}
			}
		}
	}
}

void ma_rule_engine::merge_notify_list(notify_list_t& n_list, conds_t const& c_list, notify_t type)
{
	conds_t::const_iterator it = c_list.begin();
	for (; it != c_list.end(); ++it)
	{
		notify_list_t notify((*it)->get_notify_list(type));
		n_list.merge(notify);
		n_list.unique();
	}
}

const ma_condition&
ma_rule_engine::find_cond_by_name(std::string const& name) const
{
	cond_map_t::const_iterator it = cmap.find(name);
	if (it == cmap.end())
		throw std::runtime_error("rule_engine::find_cond_by_name() name not found");
	return it->second;
}

ma_condition&
ma_rule_engine::find_cond_by_name(std::string const& name)
{
	cond_map_t::iterator it = cmap.find(name);
	if (it == cmap.end())
		throw std::runtime_error("rule_engine::find_cond_by_name() name not found");
	return it->second;
}

const ma_rule&
ma_rule_engine::find_rule_by_name(std::string const& name) const
{
	rule_map_t::const_iterator it = rmap.find(name);
	if (it == rmap.end())
		throw std::runtime_error("rule_engine::find_rule_by_name() name not found");
	return it->second;
}

ma_rule&
ma_rule_engine::find_rule_by_name(std::string const& name)
{
	rule_map_t::iterator it = rmap.find(name);
	if (it == rmap.end())
		throw std::runtime_error("rule_engine::find_rule_by_name() name not found");
	return it->second;
}
