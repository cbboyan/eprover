#!/bin/bash

DST="data:~/atp/bin"

function deploy()
{
   P=$1
   if [ -f PROVER/$P ]; then
      echo "deploy $P:"
      scp PROVER/$P $DST/$P-current
   fi
}

deploy eprover
deploy eprover-ho

