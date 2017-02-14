# TD-S

This is an experimental implementation of TD-S, a routing algorithm that takes time-dependent arc weights into account.
This repository contains only the TD-S code but not the data used in the TD-S paper. 
Unfortunately, access to this data is restricted by an NDA.
Without TD-data this code will not be of much use to you.

The TD-S code makes heavy use of RoutingKit. The TD-S makefiles assume that [RoutingKit](https://github.com/RoutingKit/RoutingKit) is installed system-wide. If it is only installed locally, then you will have to adjust the makefile.

# Input format

The input data is split across several files. Each file is the binary dump of an array of 32-bit unsigned integers or of 32-bit floating points. The files are:

* first_out
* head
* first_ipp_of_arc
* ipp_departure_time
* ipp_travel_time
* latitude
* longitude
 
All nodes have IDs between `0` and `node_count-1`.
Arcs are always directed. 
Every directed arc has an ID between `0` and `arc_count-1`.
Further, every interpolation point (IPP) has an ID between `0` and `ipp_count-1`.

The IDs of arcs with tail/source `x` are `first_out[x]`, `first_out[x]+1` ... `first_out[x+1]-1`.
The head/target node of an arc with ID `xy` is `head[xy]`.
Every arc is associated with a piece-wise linear function that has at least one interpolation point.
We assume that these functions are periodic with a period length of one day.
An arc with ID `xy` has the interpolation points with IDs `first_ipp_of_arc[xy]`, `first_ipp_of_arc[xy]+1` ... `first_ipp_of_arc[xy+1]-1`.
The departure time of an interpolation point with id `ipp` is stored in `ipp_departure_time[ipp]`. 
The travel time is stored in `ipp_travel_time[ipp]`. 
Both times are represented as milliseconds since the start of the day.
The interpolation points must be sorted by departure time.
`latitude[x]` is the latitude as floating point of the node with ID `x`.
`longitude[x]` is the longitude as floating point of the node with ID `x`.

# Preprocessing

There are two commandline tools to extract time-windows. 

```bash
compute_freeflow_weight first_ipp_of_arc_file ipp_departure_time_file ipp_travel_time_file output_weight_file
compute_time_window_weight window_begin window_end first_ipp_of_arc_file ipp_departure_time_file ipp_travel_time_file output_weight_file
```

CHs are build using RoutingKit's compute_contraction_hierarchy tool.

```bash
compute_contraction_hierarchy first_out head weight ch
```

## Freeflow Preprocessing

To run the Freeflow preprocessing execute

```bash
compute_freeflow_weight input/{first_ipp_of_arc,ipp_departure_time,ipp_travel_time} freeflow
compute_contraction_hierarchy input/{first_out,head} freeflow freeflow_ch
```

## TD-S+4 Preprocessing

To run the TD-S+4 preprocessing execute

```bash
mkdir -p win4
mkdir -p ch4
for W in 0_5 6_9 11_14 16_19
do
	B=${W%_*}
	E=${W##*_}
	compute_time_window_weight $[$B*60*60*1000] $[$E*60*60*1000] input/{first_ipp_of_arc,ipp_departure_time,ipp_travel_time} win4/$W
	compute_contraction_hierarchy input/{first_out,head} win4/$W ch4/$W
done
```	

## TD-S+9 Preprocessing

To run the TD-S+9 preprocessing execute

```bash
mkdir -p win9
mkdir -p ch9
for W in 0_240 350_370 410_430 470_490 600_720 720_840 960_1020 1020_1080 1140_1260
do
	B=${W%_*}
	E=${W##*_}
	compute_time_window_weight $[$B*60*1000] $[$E*60*1000] input/{first_ipp_of_arc,ipp_departure_time,ipp_travel_time} win9/$W
	compute_contraction_hierarchy input/{first_out,head} win9/$W ch9/$W
done
```	

## TD-S+D Preprocessing

Either use [FlowCutter](https://github.com/ben-strasser/flow-cutter-pace16) to compute a CCH contraction order or use the IntertialFlow implementation bundled with RoutingKit. Using RoutingKit you run

```bash
compute_nested_dissection_order input/{first_out,head,latitude,longitude} cch_order
```

You must also run the TD-S+4 or TD-S+9 preprocessing.

# Running TD-S

To run TD-S use the `run_td_s` command. The Freeflow heuristic is a special case of TD+S.

`run_td_s` promts you for a source stop, a source time, and a target stop on the commandline. If you enter this information, it will dump various statistics of this query onto the standard output.

## Running Freeflow

To run Freeflowexecute

```bash
run_td_s input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} freeflow_ch
```

## Running TD-S+4

To run TD-S+4 execute

```bash
run_td_s input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} ch4/*
```

## Running TD-S+9

To run TD-S+9 execute

```bash
run_td_s input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} ch9/*
```

# Running TD-S+P

To run TD-S+P use the `run_td_s_p` command.

`run_td_s_p` promts you for a source stop and a target stop on the commandline. If you enter this information, it will dump various statistics of this query onto the standard output.

## Running Freeflow

```bash
run_td_s_p input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} freeflow_ch
```

## Running TD-S+4

```bash
run_td_s_p input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} ch4/*
```

## Running TD-S+9

```bash
run_td_s_p input/{first_out,head,first_ipp_of_arc,ipp_departure_time,ipp_travel_time} ch9/*
```

# Running TD-S+D

