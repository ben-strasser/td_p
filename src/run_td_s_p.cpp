#include <routingkit/constants.h>
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
		const unsigned sample_step = 10*60*1000;
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

		auto get_pruned_td_weight = [&](unsigned arc, unsigned departure_time){
			if(is_arc_allowed[arc]){
				return evaluate_plf(
					ArcPLF(
						arc, period,
						first_ipp_of_arc, ipp_departure_time, ipp_travel_time
					),
					departure_time % period
				);
			} else {
				return inf_weight;
			}
		};

		vector<unsigned>target_time(period/sample_step);

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

			long long t = -get_micro_time();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = false;
			for(unsigned w=0; w<time_window_count; ++w)
				allowed_path_list[w] = ch_query.reset(ch[w]).add_source(source_node).add_target(target_node).run().get_arc_path();
			for(auto&p:allowed_path_list)
				for(auto a:p)
					is_arc_allowed[a] = true;
				
			for(unsigned i=0; i<period/sample_step; ++i){
				dij.run(source_node, i*sample_step, target_node, get_pruned_td_weight);
				target_time[i] = dij.distance_to(target_node);
				if(target_time[i] == inf_weight)
					break;
			}
			t += get_micro_time();

			cout 
				<< "source node : " << source_node << '\n'
				<< "source time [ms since midnight] : " << source_time << '\n'
				<< "target node : " << target_node << '\n'
				<< "query running time [musec] : " << t << '\n';
			if(target_time[0] == inf_weight){
				cout << "No path" << endl;
			} else {
				cout << "departure_time,travel_time\n";
				for(unsigned i=0; i<period/sample_step; ++i){
					cout << (i*sample_step) << ',' << (target_time[i]-i*sample_step) << '\n';
				}
				cout << endl;
			}
		}
	}catch(exception&err){
		cerr << "Stopped on exception : " << err.what() << endl;
	}
}
