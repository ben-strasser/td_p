#include <routingkit/vector_io.h>
#include <routingkit/inverse_vector.h>
#include <routingkit/contraction_hierarchy.h>
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

		if(argc <= 6){
			cerr 
				<< "Usage : \n"
				<< argv[0] << " first_out head first_ipp_of_arc ipp_departure_time ipp_travel_time time_window_ch1 [time_window_ch2 [...]]" << endl;
			return 1;
		}else{
			cerr << "Loading ... " << flush;	
			first_out = load_vector<unsigned>(argv[1]);
			head = load_vector<unsigned>(argv[2]);
			first_ipp_of_arc = load_vector<unsigned>(argv[3]);
			ipp_departure_time = load_vector<unsigned>(argv[4]);
			ipp_travel_time = load_vector<unsigned>(argv[5]);

			ch.resize(argc-6);

			for(int i=6; i<argc; ++i)
				ch[i-6] = ContractionHierarchy::load_file(argv[i]);
			cerr << "done" << endl;
		}
		
		check_if_td_graph_is_valid(period, first_out, head, first_ipp_of_arc, ipp_departure_time, ipp_travel_time);

		const unsigned node_count = first_out.size()-1;
		const unsigned arc_count = head.size();
		const unsigned time_window_count = ch.size();

		for(auto&x:ch)
			if(x.node_count() != node_count)
				throw runtime_error("CH has wrong number of nodes");

		ContractionHierarchyQuery ch_query;
		ch_query.reset(ch[0]);

		vector<bool>is_arc_allowed(arc_count, false);
		vector<vector<unsigned>>allowed_path_list(time_window_count);

		Dijkstra dij(first_out, head);

		auto get_td_weight = [&](unsigned arc, unsigned departure_time){
			return evaluate_plf(
				ArcPLF(
					arc, period,
					first_ipp_of_arc, ipp_departure_time, ipp_travel_time
				),
				departure_time % period
			);
		};

		auto get_pruned_td_weight = [&](unsigned arc, unsigned departure_time){
			if(is_arc_allowed[arc])
				return get_td_weight(arc, departure_time);
			else 
				return inf_weight;
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

			long long baseline_timer = -get_micro_time();
			dij.run(source_node, source_time, target_node, get_td_weight);
			unsigned exact_target_time = dij.distance_to(target_node);
			vector<unsigned>exact_path = dij.arc_path_to(target_node);
			baseline_timer += get_micro_time();

			long long td_s_timer = -get_micro_time();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = false;
			for(unsigned w=0; w<time_window_count; ++w)
				allowed_path_list[w] = ch_query.reset(ch[w]).add_source(source_node).add_target(target_node).run().get_arc_path();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = true;
			dij.run(source_node, source_time, target_node, get_pruned_td_weight);
			unsigned td_s_target_time = dij.distance_to(target_node);
			vector<unsigned>td_s_path = dij.arc_path_to(target_node);
			td_s_timer  += get_micro_time();

			cout 
				<< "source node : " << source_node << '\n'
				<< "source time [ms since midnight] : " << source_time << '\n'
				<< "target node : " << target_node << '\n'
				<< "Dijkstra running time [musec] : " << baseline_timer << '\n'
				<< "TD-S query running time [musec] : " << td_s_timer  << '\n';
			if(exact_target_time == inf_weight){
				cout << "No path" << endl;
			} else {
				cout 
					<< "Exact target time [ms since midnight] : " << exact_target_time << '\n'
					<< "Exact travel time [ms since midnight] : " << (exact_target_time-source_time) << '\n'
					<< "Exact arc path :";
				for(auto a:exact_path)
					cout << ' ' << a;
				cout << endl;
				cout 
					<< "TD-S target time [ms since midnight] : " << td_s_target_time << '\n'
					<< "TD-S travel time [ms since midnight] : " << (td_s_target_time-source_time) << '\n'
					<< "TD-S arc path :";
				for(auto a:td_s_path)
					cout << ' ' << a;
				cout << endl;
			}
		}
	}catch(exception&err){
		cerr << "Stopped on exception : " << err.what() << endl;
	}
}
