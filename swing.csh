#!/bin/csh
#set nonomatch = 1
set notyet = "nh nc pa wi"
foreach stateId (co fl ia mi mn nv oh az)
  set d = "/localssd/rcownie/openelections-data-${stateId}"
  if (-d $d) then
    echo "$d ..."
    votecount -a Candidate -s ${stateId} $d/2016/*_general*_precinct.csv >${stateId}_2016.txt
    votecount -a candidate -s ${stateId} $d/2016/*_general*_precinct.csv >>${stateId}_2016.txt
    votecount -o ${stateId}_2016.csv -s ${stateId} $d/2016/*_general*_precinct.csv
  endif
end
