#ifndef ERROR_HANDLER_MA_RULE_ENGINE_H
#define ERROR_HANDLER_MA_RULE_ENGINE_H

#include "ErrorHandler/MessageAnalyzer/ma_condition.h"
#include "ErrorHandler/MessageAnalyzer/ma_participants.h"
#include "ErrorHandler/MessageAnalyzer/ma_rule.h"
#include "ErrorHandler/MessageAnalyzer/ma_types.h"

#include <boost/thread.hpp>
#include "ErrorHandler/MessageAnalyzer/ma_timing_event.h"

namespace fhicl {
class ParameterSet;
}

namespace novadaq {
namespace errorhandler {

class ma_rule_engine
{
public:
	// c'tor
	ma_rule_engine(fhicl::ParameterSet const& pset, alarm_fn_t alarm, cond_match_fn_t cond_match);

	// public method, call to run the rule engine
	void feed(qt_mf_msg const& msg);

	// public accessor for cond map and rule map
	size_t cond_size() const { return cmap.size(); }
	size_t rule_size() const { return rmap.size(); }

	const std::vector<std::string>& cond_names() const { return cnames; }
	const std::vector<std::string>& rule_names() const { return rnames; }

	bool is_EHS() const { return EHS; }

	// get the raw configuration ParameterSet object
	fhicl::ParameterSet
	get_configuration() const
	{
		return pset;
	}

	// get condition fields
	const std::string&
	cond_description(std::string const& name) const
	{
		return find_cond_by_name(name).description();
	}

	const std::string&
	cond_sources(std::string const& name) const
	{
		return find_cond_by_name(name).sources_str();
	}

	const std::string&
	cond_regex(std::string const& name) const
	{
		return find_cond_by_name(name).regex();
	}

	int
	cond_msg_count(std::string const& name) const
	{
		return find_cond_by_name(name).get_msg_count();
	}

	// get rule fields
	const std::string&
	rule_description(std::string const& name) const
	{
		return find_rule_by_name(name).description();
	}

	const std::string&
	rule_expr(std::string const& name) const
	{
		return find_rule_by_name(name).cond_expr();
	}

	const std::vector<std::string>&
	rule_cond_names(std::string const& name) const
	{
		return find_rule_by_name(name).cond_names();
	}

	int
	rule_alarm_count(std::string const& name) const
	{
		return find_rule_by_name(name).get_alarm_count();
	}

	// set rule enable/disable status
	void enable_rule(std::string const& name, bool flag)
	{
		find_rule_by_name(name).enable(flag);
	}

	// enable/disable EHS
	void enable_EHS(bool flag)
	{
		EHS = flag;
	}

	// reset a rule to its ground state (reset alarms and domains)
	void reset_rule(std::string const& name)
	{
		find_rule_by_name(name).reset();
	}

	void reset_rules()
	{
		for (rule_map_t::iterator it = rmap.begin(); it != rmap.end(); ++it)
			it->second.reset();
	}

	// reset conditions
	void reset_cond(std::string const& name)
	{
		find_cond_by_name(name).reset();
	}

	void reset_conds()
	{
		for (cond_map_t::iterator it = cmap.begin(); it != cmap.end(); ++it)
			it->second.reset();
	}

	// reset all
	void reset()
	{
		reset_conds();
		reset_rules();
	}

	// participants
	void add_participant_group(std::string const& group)
	{
		ma_participants::instance().add_group(group);
	}

	void add_participant_group(std::string const& group, size_t size)
	{
		ma_participants::instance().add_group(group, size);
	}

	void add_participant(std::string const& group, std::string const& app)
	{
		ma_participants::instance().add_participant(group, app);
	}

	void add_participant(std::string const& app)
	{
		ma_participants::instance().add_participant(app);
	}

	size_t get_group_participant_count(std::string const& group) const
	{
		return ma_participants::instance().get_group_participant_count(group);
	}

	size_t get_participant_count() const
	{
		return ma_participants::instance().get_participant_count();
	}

private:
	// initialize the rule engine with configuration file
	void init_engine();

	// event worker
	void event_worker();

	// merge notification list from conditions
	void merge_notify_list(notify_list_t& n_list, conds_t const& c_list, notify_t type);

	// evaluates the domain / status of all rules in the notification list
	void evaluate_rules_domain(notify_list_t& notify_domain);
	void evaluate_rules(notify_list_t& notify_status);

	// find condition/rule with given name
	const ma_condition& find_cond_by_name(std::string const& name) const;
	ma_condition& find_cond_by_name(std::string const& name);

	const ma_rule& find_rule_by_name(std::string const& name) const;
	ma_rule& find_rule_by_name(std::string const& name);

private:
	// configuration
	fhicl::ParameterSet pset;

	// map of conditions
	cond_map_t cmap;
	std::vector<std::string> cnames;

	// map of rules
	rule_map_t rmap;
	std::vector<std::string> rnames;

	// callbacks
	alarm_fn_t alarm_fn;
	cond_match_fn_t cond_match_fn;

	// a list of scheduled events
	ma_timing_events events;

	// event thread
	boost::thread event_worker_t;

	// whether this engine is an Error Handler Supervisor
	bool EHS;
};

}  // end of namespace errorhandler
}  // end of namespace novadaq

#endif
