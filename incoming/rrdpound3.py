#!/usr/bin/env python
import os
import rrdtool
from time import time
import argparse
import multiprocessing

import Globals
from Products.ZenRRD.RRDUtil import RRDUtil
from Products.ZenUtils.Utils import zenPath

def run(args, processId):
  try:
    perfPath = "/".join([args.perf_path, str(processId)])
    
    create_cmd = 'RRA:AVERAGE:0.5:1:600\nRRA:AVERAGE:0.5:6:600\nRRA:AVERAGE:0.5:24:600\nRRA:AVERAGE:0.5:288:600\nRRA:MAX:0.5:6:600\nRRA:MAX:0.5:24:600\nRRA:MAX:0.5:288:600'
    rrd = RRDUtil(create_cmd, args.cycle_time)
    
    
    cycle_begin = time()
    dp_count = 0
    for device in map(str, range(args.device_count)):
        begin = time()
        for device_dp in map(str, range(args.device_datapoints)):
            dp_count += 1
            rrd.save(os.path.join(perfPath, device, device_dp), 42, 'GAUGE')

        for interface in map(str, range(args.component_count)):
            for interface_dp in map(str, range(args.component_datapoints)):
                dp_count += 1
                path = os.path.join(perfPath, device, 'os', 'interfaces', interface,
                    interface_dp)
                rrd.save(path, 42, 'DERIVE')
                rrdtool.fetch(rrd.performancePath(path) + '.rrd',
                    'AVERAGE', '-s', 'now-%d' % (args.cycle_time*2), '-e', 'now')

    cycle_duration = time() - cycle_begin
    return (cycle_duration, dp_count)
  except KeyboardInterrupt:
    return (0, 0)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cycles", default=10, type=int,
        help="Number of times to run benchmark [%(default)s]")
    
    parser.add_argument("--cycle-time", default=10, type=int,
        help="Data collection interval [%(default)s]")

    parser.add_argument("--device-count", default=1000, type=int,
        help="Number of devices to simulate") 
    parser.add_argument("--device-datapoints", default=10, type=int,
        help="Number of datapoints per device [%(default)s]")

    parser.add_argument("--component-count", default=50, type=int,
        help="Number of components per device [%(default)s]")
    parser.add_argument("--component-datapoints", default=7, type=int,
        help="Number of datapoints per component [%(default)s]")

    parser.add_argument("--processes", default=1, type=int,
        help="Number of datapoints per component [%(default)s]")

    parser.add_argument("--perf-path", default=zenPath("perf/perftest"), 
        help="Path to the perform the test on [%(default)s]")

    
    args = parser.parse_args()
    print "cycles:                %d" % args.cycles
    print "cycle time:            %d" % args.cycle_time
    print "device count:          %d" % args.device_count
    print "datapoints per device  %d" % args.device_datapoints
    print "component count:       %d" % args.component_count
    print "datapoints per device: %d" % args.component_datapoints
    print "processes:             %d" % args.processes
    print "perf path:             %s" % args.perf_path
 
    pool = multiprocessing.Pool(int(args.processes))
    try: 
    
        for cycle in range(int(args.cycles)):
            results = []
            start = time()
            for processId in range(0, args.processes):
                results.append(pool.apply_async(run, [args, processId]))
    
            datapoints = 0
        
            for result in results:
                runtime, datapointcount = result.get()
                datapoints += datapointcount
            end = time()
            elapsed = end - start
            print "%.3f (%.3f/sec)" % (elapsed, datapoints / elapsed)
    except KeyboardInterrupt:
        pool.terminate()
        print

if __name__ == '__main__':
    main()
    

