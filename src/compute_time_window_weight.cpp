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
		unsigned bucket_begin, bucket_end;

		const unsigned period = 24*60*60*1000;
		vector<unsigned>first_ipp_of_arc;
		vector<unsigned>ipp_departure_time;
		vector<unsigned>ipp_travel_time;
	
		string weight_file;

		if(argc != 7){
			cerr << argv[0] << " window_begin window_end first_ipp_of_arc_file ipp_departure_time_file ipp_travel_time_file weight_file\n"
				<< "Usage: " << argv[0] << " "<<6*60*60*1000<<" "<<10*60*60*1000<<" td/{first_ipp_of_arc,ipp_departure_time,ipp_travel_time} output_weight_file\n"
				<< "window_begin and window_end are in milliseconds since the begin of the day." << endl;
			return 1;
		} else {
			cout << "Loading ... " << flush;
			bucket_begin = stoul(argv[1]);
			bucket_end = stoul(argv[2]);

			first_ipp_of_arc = load_vector<unsigned>(argv[3]);
			ipp_departure_time = load_vector<unsigned>(argv[4]);
			ipp_travel_time = load_vector<unsigned>(argv[5]);
			weight_file = argv[6];
			cout << "done" << endl;
		}

		cout << "Validity tests ... " << flush;
		if(bucket_end <= bucket_begin)
			throw runtime_error("bucket begin must be before bucket end");
		if(period < bucket_end)
			throw runtime_error("bucket end must be smaller than the period");
		check_if_arc_ipp_are_valid(period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time);
		cout << "done" << endl;

		cout << "Computing weights ... " << flush;

		vector<unsigned>weight = compute_time_window_avg_weights(
			bucket_begin, bucket_end,
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
