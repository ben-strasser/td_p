#include "ipp.h"
#include "verify.h"

#include <routingkit/vector_io.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>

using namespace std;
using namespace RoutingKit;

int main(int argc, char*argv[]){
	try{
		const unsigned period = 24*60*60*1000;
		vector<unsigned>first_ipp_of_arc;
		vector<unsigned>ipp_departure_time;
		vector<unsigned>ipp_travel_time;
	
		string weight_file;

		if(argc != 5){
			cerr << argv[0] << " first_ipp_of_arc_file ipp_departure_time_file ipp_travel_time_file weight_file\n"
				<< "Usage: " << argv[0] << " td/{first_ipp_of_arc,ipp_departure_time,ipp_travel_time} output_weight_file" << endl;
			return 1;
		} else {
			first_ipp_of_arc = load_vector<unsigned>(argv[1]);
			ipp_departure_time = load_vector<unsigned>(argv[2]);
			ipp_travel_time = load_vector<unsigned>(argv[3]);
			weight_file = argv[4];
			cout << "done" << endl;
		} 

		cout << "Validity tests ... " << flush;
		check_if_arc_ipp_are_valid(period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time);
		cout << "done" << endl;

		cout << "Computing weights ... " << flush;
		vector<unsigned>weight = compute_min_weights(
			period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time
		);
		cout << "done" << endl;

		cout << "Saving ... " << flush;
		save_vector(weight_file, weight);
		cout << "done" << endl;
		
	}catch(exception&err){
		cerr << "Stopped on exception : " << err.what() << endl;
	}
}
