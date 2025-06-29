#!/bin/bash

perf stat \
  -e cycles \
  -e instructions \
  -e cache-references \
  -e cache-misses \
  -e branches \
  -e branch-misses \
  -e L1-dcache-loads \
  -e L1-dcache-load-misses \
  -e LLC-loads \
  -e LLC-load-misses \
  -e dTLB-loads \
  -e dTLB-load-misses \
  -e frontend_retired.itlb_miss \
  -e frontend_retired.dsb_miss \
  -e cycle_activity.stalls_mem_any \
  -e cycle_activity.cycles_mem_any \
  -e cycle_activity.stalls_l1d_miss \
  -e cycle_activity.stalls_l2_miss \
  -e uops_executed.core \
  -e uops_issued.any \
  -e idq_uops_not_delivered.core \
  -e int_misc.recovery_cycles \
  -e exe_activity.bound_on_loads \
  -e mem_load_retired.l1_hit \
  -e mem_load_retired.l2_miss \
  -e mem_load_retired.l3_miss \
  -e task-clock \
  -e page-faults \
  -e context-switches \
  -e cpu-migrations \
  "$@"
