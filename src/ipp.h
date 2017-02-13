#ifndef INTERPOLATION_POINT_H
#define INTERPOLATION_POINT_H

#include <routingkit/min_max.h>
#include <routingkit/constants.h>
#include <cassert>
#include <vector>

using RoutingKit::invalid_id;
using RoutingKit::inf_weight;

// ipp stands for interpolation point
// plf stands for piece-wise linear function
// Unless explicitly stated all plf map departure time onto travel time

struct IPP{
	unsigned departure_time;
	unsigned travel_time;
};

inline
bool operator==(IPP l, IPP r){
	return l.departure_time == r.departure_time;
}

inline
bool operator!=(IPP l, IPP r){
	return !(l==r);
}

inline
unsigned compute_travel_time_without_wrap_around(IPP before, IPP after, unsigned departure_time){
	assert(before.departure_time != after.departure_time);
	assert(before.departure_time < after.departure_time);

	assert(before.departure_time <= departure_time);
	assert(departure_time <= after.departure_time);
		
	unsigned pos = departure_time - before.departure_time;
	unsigned length = after.departure_time - before.departure_time;
	return (static_cast<unsigned long long>(before.travel_time)*(length - pos) + static_cast<unsigned long long>(after.travel_time)*pos) / static_cast<unsigned long long>(length);
}

inline
unsigned compute_travel_time_with_wrap_around(unsigned period, IPP before, IPP after, unsigned departure_time){
	assert(before.departure_time <= period);
	assert(after.departure_time <= period);
	assert(departure_time < period);

	if(after.departure_time <= before.departure_time)
		after.departure_time += period;
	if(departure_time < before.departure_time)
		departure_time += period;

	assert(departure_time <= after.departure_time);

	return compute_travel_time_without_wrap_around(before, after, departure_time);
}

inline
IPP shift_ipp_departure_time(unsigned period, IPP ipp){
	return IPP{ipp.departure_time + period, ipp.travel_time};
}

template<class GetDepartureTime, class GetTravelTime>
class LambdaPLF{
public:
	LambdaPLF(unsigned period_, unsigned ipp_count_, GetDepartureTime ipp_departure_time_, GetTravelTime ipp_travel_time_):
		period_(period_), ipp_count_(ipp_count_), ipp_departure_time_(std::move(ipp_departure_time_)), ipp_travel_time_(std::move(ipp_travel_time_)){

		#ifndef NDEBUG
		for(unsigned i=0; i<ipp_count_; ++i){
			ipp_departure_time_(i);
			ipp_travel_time_(i);
		}
		#endif
	}

	unsigned period()const{
		return period_;
	}

	unsigned ipp_count()const{
		return ipp_count_;
	}

	unsigned ipp_departure_time(unsigned i)const{
		assert(i < ipp_count());
		return ipp_departure_time_(i);
	}

	unsigned ipp_travel_time(unsigned i)const{
		assert(i < ipp_count());
		return ipp_travel_time_(i);
	}

private:
	unsigned period_;
	unsigned ipp_count_;
	GetDepartureTime ipp_departure_time_;
	GetTravelTime ipp_travel_time_;
};

template<class GetDepartureTime, class GetTravelTime>
LambdaPLF<GetDepartureTime, GetTravelTime> make_plf(unsigned period_, unsigned ipp_count_, GetDepartureTime ipp_departure_time_, GetTravelTime ipp_travel_time_){
	return {period_, ipp_count_, std::move(ipp_departure_time_), std::move(ipp_travel_time_)};
}

class ArcPLF{
public:
	ArcPLF(
		unsigned arc, 
		unsigned period, 
		const std::vector<unsigned>&first_ipp_of_arc, 
		const std::vector<unsigned>&ipp_departure_time, 
		const std::vector<unsigned>&ipp_travel_time
	):
		ipp_count_(first_ipp_of_arc[arc+1]-first_ipp_of_arc[arc]),
		period_(period),
		ipp_departure_time_(ipp_departure_time.cbegin()+first_ipp_of_arc[arc]),
		ipp_travel_time_(ipp_travel_time.cbegin()+first_ipp_of_arc[arc]){}
	
	unsigned period()const{
		return period_;
	}

	unsigned ipp_count()const{
		return ipp_count_;
	}

	unsigned ipp_departure_time(unsigned i)const{
		assert(i < ipp_count());
		return *(ipp_departure_time_ + i);
	}

	unsigned ipp_travel_time(unsigned i)const{
		assert(i < ipp_count());
		return *(ipp_travel_time_ + i);
	}

private:
	unsigned ipp_count_, period_;
	std::vector<unsigned>::const_iterator ipp_departure_time_, ipp_travel_time_;
};

template<class PLF>
IPP get_ipp_of_plf(const PLF&plf, unsigned i){
	return {plf.ipp_departure_time(i), plf.ipp_travel_time(i)};
}

template<class PLF>
unsigned evaluate_plf(
	const PLF&plf,
	unsigned departure_time
){
	unsigned first_ipp = 0;
	unsigned last_ipp = plf.ipp_count()-1;

	if(first_ipp == last_ipp)
		return plf.ipp_travel_time(first_ipp);

	if(departure_time < plf.ipp_departure_time(first_ipp) || plf.ipp_departure_time(last_ipp) <= departure_time){
		return compute_travel_time_with_wrap_around(plf.period(), get_ipp_of_plf(plf, last_ipp), get_ipp_of_plf(plf, first_ipp), departure_time);
	} else {
		while(last_ipp - first_ipp > 1){
			unsigned mid = (first_ipp + last_ipp)/2;
			
			if(plf.ipp_departure_time(mid) < departure_time) {
				first_ipp = mid;
			} else if(plf.ipp_departure_time(mid) > departure_time) {
				last_ipp = mid;
			} else {
				return plf.ipp_travel_time(mid);
			}
		}
		return compute_travel_time_without_wrap_around(get_ipp_of_plf(plf, first_ipp), get_ipp_of_plf(plf, last_ipp), departure_time);
	}
}

template<class PLF>
unsigned evaluate_plf_with_stabing(
	const PLF&plf,
	unsigned departure_time,
	unsigned&stab_ipp
){
	assert(departure_time < plf.period());
	unsigned first_ipp = 0;
	unsigned last_ipp = plf.ipp_count()-1;

	if(first_ipp == last_ipp)
		return plf.ipp_travel_time(first_ipp);

	for(;;){
		unsigned after_stab_ipp = stab_ipp+1;
		if(after_stab_ipp == plf.ipp_count()){
			after_stab_ipp = 0;

			unsigned period = plf.period();

			if(departure_time <= plf.ipp_departure_time(0)){
				return compute_travel_time_without_wrap_around(
					get_ipp_of_plf(plf, after_stab_ipp), 
					shift_ipp_departure_time(period, get_ipp_of_plf(plf, stab_ipp)), 
					departure_time + period
				);
			}

			if(plf.ipp_departure_time(after_stab_ipp) <= departure_time){
				return compute_travel_time_without_wrap_around(
					get_ipp_of_plf(plf, after_stab_ipp), 
					shift_ipp_departure_time(period, get_ipp_of_plf(plf, stab_ipp)), 
					departure_time
				);
			}
		}else{
			if(plf.ipp_departure_time(stab_ipp) <= departure_time && departure_time <= plf.ipp_departure_time(after_stab_ipp)){
				return compute_travel_time_without_wrap_around(
					get_ipp_of_plf(plf, stab_ipp), 
					get_ipp_of_plf(plf, after_stab_ipp), 
					departure_time
				);
			}
		}
		stab_ipp = after_stab_ipp;
	}
}

template<class PLF>
unsigned long long integral_of_plf_without_wraparound_times_two(
	const PLF&plf,
	unsigned begin, unsigned end
){
	const unsigned period = plf.period();
	const unsigned ipp_count = plf.ipp_count();

	assert(begin <= period);
	assert(end <= period);
	assert(begin <= end);

	unsigned long long sum_times_two = 0;
	auto handle_piece = [&](__int128_t ax, __int128_t ay, __int128_t bx, __int128_t by){
		if(end <= ax)
			return;
		if(bx <= begin)
			return;

		auto dx = bx-ax;
		auto dy = by-ay;
		
		auto I_times_2dx = [=](__int128_t p){
			return p*(dy*p + 2*(by*dx-dy*bx));
		}; 

		if(ax < begin)
			ax = begin;
		if(end < bx)
			bx = end;
		sum_times_two += (I_times_2dx(bx) - I_times_2dx(ax))/(dx);
	};

	for(unsigned i=0; i<ipp_count-1; ++i){
		handle_piece(
			plf.ipp_departure_time(i), plf.ipp_travel_time(i), 
			plf.ipp_departure_time(i+1), plf.ipp_travel_time(i+1)
		);
	}
	handle_piece(
		plf.ipp_departure_time(ipp_count-1), plf.ipp_travel_time(ipp_count-1), 
		plf.ipp_departure_time(0)+period, plf.ipp_travel_time(0)
	);
	begin += period;
	end += period;
	handle_piece(
		plf.ipp_departure_time(ipp_count-1), plf.ipp_travel_time(ipp_count-1), 
		plf.ipp_departure_time(0)+period, plf.ipp_travel_time(0)
	);
	return sum_times_two;
}



template<class PLF>
unsigned long long integral_of_plf(
	const PLF&plf,
	unsigned begin, unsigned end
){
	assert(begin <= plf.period());
	assert(end <= plf.period());

	if(begin <= end)
		return integral_of_plf_without_wraparound_times_two(plf, begin, end)/2;
	else
		return (integral_of_plf_without_wraparound_times_two(plf, begin, plf.period())+
			integral_of_plf_without_wraparound_times_two(plf, 0, end))/2;
}

template<class PLF>
unsigned long long minimum_of_plf(
	const PLF&plf
){
	unsigned x = plf.ipp_travel_time(0);
	for(unsigned i=1; i<plf.ipp_count(); ++i)
		RoutingKit::min_to(x, plf.ipp_travel_time(i));
	return x;
}

template<class PLF>
unsigned long long maximum_of_plf(
	const PLF&plf
){
	unsigned x = plf.ipp_travel_time(0);
	for(unsigned i=1; i<plf.ipp_count(); ++i)
		RoutingKit::max_to(x, plf.ipp_travel_time(i));
	return x;
}

inline
std::vector<unsigned>compute_time_window_avg_weights(
	unsigned window_begin, unsigned window_end,
	unsigned period, const std::vector<unsigned>&first_ipp_of_arc, const std::vector<unsigned>&ipp_departure_time, const std::vector<unsigned>&ipp_travel_time
){
	unsigned arc_count = first_ipp_of_arc.size()-1;
	assert(window_begin != window_end);

	unsigned window_length;
	if(window_begin < window_end)
		window_length = window_end - window_begin;
	else
		window_length = (period + window_end) - window_begin;
		
	std::vector<unsigned>weight(arc_count);
	for(unsigned i=0; i<arc_count; ++i)
		weight[i] = integral_of_plf(ArcPLF(i, period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time), window_begin, window_end) / window_length;
	return weight; // NVRO
}

inline
std::vector<unsigned>compute_min_weights(
	unsigned period, const std::vector<unsigned>&first_ipp_of_arc, const std::vector<unsigned>&ipp_departure_time, const std::vector<unsigned>&ipp_travel_time
){
	unsigned arc_count = first_ipp_of_arc.size()-1;
	assert(window_begin != window_end);
		
	std::vector<unsigned>weight(arc_count);
	for(unsigned i=0; i<arc_count; ++i)
		weight[i] = minimum_of_plf(ArcPLF(i, period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time));
	return weight; // NVRO
}

inline
std::vector<unsigned>compute_max_weights(
	unsigned period, const std::vector<unsigned>&first_ipp_of_arc, const std::vector<unsigned>&ipp_departure_time, const std::vector<unsigned>&ipp_travel_time
){
	unsigned arc_count = first_ipp_of_arc.size()-1;
	assert(window_begin != window_end);
		
	std::vector<unsigned>weight(arc_count);
	for(unsigned i=0; i<arc_count; ++i)
		weight[i] = maximum_of_plf(ArcPLF(i, period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time));
	return weight; // NVRO
}

inline
std::vector<unsigned>compute_time_point_weights(
	unsigned time_point,
	unsigned period, const std::vector<unsigned>&first_ipp_of_arc, const std::vector<unsigned>&ipp_departure_time, const std::vector<unsigned>&ipp_travel_time
){
	unsigned arc_count = first_ipp_of_arc.size()-1;
	std::vector<unsigned>weight(arc_count);
	for(unsigned i=0; i<arc_count; ++i)
		weight[i] = evaluate_plf(ArcPLF(i, period, first_ipp_of_arc, ipp_departure_time, ipp_travel_time), time_point);
	return weight; // NVRO
}


#endif

