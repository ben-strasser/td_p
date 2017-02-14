#include <routingkit/vector_io.h>
#include <routingkit/inverse_vector.h>
#include <routingkit/contraction_hierarchy.h>
#include <routingkit/customizable_contraction_hierarchy.h>
#include <routingkit/timer.h>

#include "ipp.h"
#include "dijkstra.h"
#include "verify.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <random>
using namespace std;
using namespace RoutingKit;

int main(int argc, char*argv[]){
	try{

		vector<ContractionHierarchy>ch;
		const unsigned period = 24*60*60*1000;
		vector<unsigned>first_out, head, first_ipp_of_arc, ipp_departure_time, ipp_travel_time;

		vector<unsigned>cch_order;

		if(argc <= 7){
			cerr 
				<< "Usage : \n"
				<< argv[0] << " first_out head first_ipp_of_arc ipp_departure_time ipp_travel_time cch_order time_window_ch1 [time_window_ch2 [...]]" << endl;
			return 1;
		}else{
			cerr << "Loading ... " << flush;	
			first_out = load_vector<unsigned>(argv[1]);
			head = load_vector<unsigned>(argv[2]);
			first_ipp_of_arc = load_vector<unsigned>(argv[3]);
			ipp_departure_time = load_vector<unsigned>(argv[4]);
			ipp_travel_time = load_vector<unsigned>(argv[5]);
			cch_order = load_vector<unsigned>(argv[6]);

			ch.resize(argc-7);

			for(int i=7; i<argc; ++i)
				ch[i-7] = ContractionHierarchy::load_file(argv[i]);
			cerr << "done" << endl;
		}
		
		check_if_td_graph_is_valid(period, first_out, head, first_ipp_of_arc, ipp_departure_time, ipp_travel_time);

		const unsigned node_count = first_out.size()-1;
		const unsigned arc_count = head.size();
		const unsigned time_window_count = ch.size();

		for(auto&x:ch)
			if(x.node_count() != node_count)
				throw runtime_error("CH has wrong number of nodes");

		if(cch_order.size() != node_count)
			throw runtime_error("CCH order has wrong size");

		if(!is_permutation(cch_order))
			throw runtime_error("CCH order is no permutation");

		vector<unsigned>freeflow(arc_count);
		for(unsigned arc=0; arc<arc_count; ++arc){
			freeflow[arc] = minimum_of_plf(
				ArcPLF(
					arc, period,
					first_ipp_of_arc, ipp_departure_time, ipp_travel_time
				)
			);
		}

		ContractionHierarchyQuery ch_query;
		ch_query.reset(ch[0]);


		CustomizableContractionHierarchy cch(cch_order, invert_inverse_vector(first_out), head);
		vector<unsigned>current_weight(arc_count);
		CustomizableContractionHierarchyMetric metric(cch, current_weight);
		CustomizableContractionHierarchyQuery cch_query(metric);

		Dijkstra dij(first_out, head);

		vector<bool>is_arc_slowed(arc_count, false);
		
		auto generate_realtime_congestion = [&](unsigned seed, const vector<unsigned>&arc_path){
			minstd_rand random_generator;
			random_generator.seed(seed);

			if(arc_path.empty())
				return;
			for(unsigned congestion=0; congestion < 3; ++congestion){
				unsigned arc = random_generator() % arc_path.size();
				unsigned length = 0;
				while(arc < arc_path.size() && length < 4*60*1000){
					is_arc_slowed[arc_path[arc]] = true;
					length += freeflow[arc_path[arc]];
					++arc;
				}
			}
		};

		auto clear_realtime_congestion = [&]{
			fill(is_arc_slowed.begin(), is_arc_slowed.end(), false);
		};

		vector<bool>is_arc_allowed(arc_count, false);
		vector<vector<unsigned>>allowed_path_list(time_window_count+1);

		auto get_only_predicted_weight = [&](unsigned arc, unsigned departure_time){
			return evaluate_plf(
				ArcPLF(
					arc, period,
					first_ipp_of_arc, ipp_departure_time, ipp_travel_time
				),
				departure_time % period
			);
		};

		unsigned current_timepoint;

		auto get_predicted_and_realtime_weight = [&](unsigned arc, unsigned departure_time)->unsigned{
			unsigned predicted_travel_time = get_only_predicted_weight(arc, departure_time);

			unsigned time_since_now = departure_time - current_timepoint;
			const unsigned fade_time = 1*60*60*1000;

			if(!is_arc_slowed[arc] || time_since_now >= fade_time){
				return predicted_travel_time;
			} else {

				unsigned current_travel_time = 5*predicted_travel_time;

				unsigned fade_travel_time = get_only_predicted_weight(arc, departure_time+fade_time);
				
				if(fade_travel_time >= current_travel_time){
					return predicted_travel_time;
				}else{
					if(fade_time < current_travel_time - fade_travel_time)
						return predicted_travel_time;

					unsigned break_time = fade_time - (current_travel_time - fade_travel_time);
					if(time_since_now < break_time)
						return current_travel_time;
					else
						return current_travel_time - (time_since_now - break_time);
				}
			}
		};

		auto get_pruned_predicted_and_realtime_weight = [&](unsigned arc, unsigned departure_time){
			if(is_arc_allowed[arc])
				return get_predicted_and_realtime_weight(arc, departure_time);
			else 
				return inf_weight;
		};

		auto compute_target_time_along_path = [&](unsigned source_time, const vector<unsigned>&path){
			unsigned target_time = source_time;
			for(auto arc:path)
				target_time += get_predicted_and_realtime_weight(arc, target_time);
			return target_time;
		};

		auto update_cch = [&]{
			for(unsigned arc=0; arc<arc_count; ++arc){
				current_weight[arc] = get_predicted_and_realtime_weight(arc, current_timepoint);
			}
			metric.customize();
		};

		cout << "Ready" << endl;

		for(;;){
			unsigned source_node, source_time, target_node;
			cin >> source_node >> source_time >> target_node;

			if(source_node > node_count){
				cout << "source node invalid" << endl;
				continue;
			}
			if(target_node > node_count){
				cout << "target node invalid" << endl;
				continue;
			}
			if(source_time > period){
				cout << "source time invalid" << endl;
				continue;
			}

			long long predicted_baseline_timer = -get_micro_time();
			dij.run(source_node, source_time, target_node, get_only_predicted_weight);
			vector<unsigned>predicted_exact_path = dij.arc_path_to(target_node);
			predicted_baseline_timer += get_micro_time();

			generate_realtime_congestion(source_time, predicted_exact_path);

			long long cch_update_timer = -get_micro_time();
			update_cch();
			cch_update_timer += get_micro_time();

			long long predicted_and_realtime_baseline_timer = -get_micro_time();
			dij.run(source_node, source_time, target_node, get_predicted_and_realtime_weight);
			unsigned predicted_and_realtime_exact_target_time = dij.distance_to(target_node);
			vector<unsigned>predicted_and_realtime_exact_path = dij.arc_path_to(target_node);
			predicted_and_realtime_baseline_timer += get_micro_time();

			unsigned predicted_path_heuristic_target_time = compute_target_time_along_path(source_time, predicted_exact_path);

			long long td_s_d_timer = -get_micro_time();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = false;
			for(unsigned w=0; w<time_window_count; ++w)
				allowed_path_list[w] = ch_query.reset(ch[w]).add_source(source_node).add_target(target_node).run().get_arc_path();
			allowed_path_list[time_window_count] = cch_query.reset().add_source(source_node).add_target(target_node).run().get_arc_path();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = true;
			dij.run(source_node, source_time, target_node, get_pruned_predicted_and_realtime_weight);
			unsigned td_s_d_target_time = dij.distance_to(target_node);
			vector<unsigned>td_s_d_path = dij.arc_path_to(target_node);
			td_s_d_timer  += get_micro_time();

			cout 
				<< "source node : " << source_node << '\n'
				<< "source time [ms since midnight] : " << source_time << '\n'
				<< "target node : " << target_node << '\n'
				<< "Dijkstra baseline running time [musec] : " << predicted_and_realtime_baseline_timer << '\n'
				<< "TD-S+D query running time [musec] : " << td_s_d_timer  << '\n'
				<< "CCH update time [musec] : " << cch_update_timer << '\n';
			if(predicted_and_realtime_exact_target_time == inf_weight){
				cout << "No path" << endl;
			} else {
				cout 
					<< "Exact target time [ms since midnight] : " << predicted_and_realtime_exact_target_time << '\n'
					<< "Exact travel time [ms since midnight] : " << (predicted_and_realtime_exact_target_time-source_time) << '\n'
					<< "Exact arc path :";
				for(auto a:predicted_and_realtime_exact_path)
					cout << ' ' << a;
				cout << endl;

				cout 
					<< "Predicted Path target time [ms since midnight] : " << predicted_path_heuristic_target_time << '\n'
					<< "Predicted Path travel time [ms since midnight] : " << (predicted_path_heuristic_target_time-source_time) << '\n'
					<< "Predicted Path arc path :";
				for(auto a:predicted_exact_path)
					cout << ' ' << a;
				cout << endl;

				cout 
					<< "TD-S+D target time [ms since midnight] : " << td_s_d_target_time << '\n'
					<< "TD-S+D travel time [ms since midnight] : " << (td_s_d_target_time-source_time) << '\n'
					<< "TD-S+D arc path :";
				for(auto a:td_s_d_path)
					cout << ' ' << a;
				cout << endl;
			}

			clear_realtime_congestion();
		}
	}catch(exception&err){
		cerr << "Stopped on exception : " << err.what() << endl;
	}
}
