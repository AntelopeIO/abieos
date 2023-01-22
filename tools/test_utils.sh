#!/usr/bin/env bash

Transaction_ABI="https://raw.githubusercontent.com/eosnetworkfoundation/mandel-eosjs/main/src/transaction.abi.json"

######################################
PREFIX="Hex from JSON:"
if [ -f ../build/tools/generate_hex_from_json ]; then
  # no args test ###############
  if ../build/tools/generate_hex_from_json 2>>/dev/null ; then
    echo "error: no arguments and still success ../build/tools/generate_hex_from_json"
    exit 1
  fi
  echo "${PREFIX} Passed No Args Test"

  # download ABI ################
  curl ${Transaction_ABI} --output ../test/transaction.abi.json 2>>/dev/null
  # Bool test ###################
  true_byte=$(../build/tools/generate_hex_from_json -f ../test/transaction.abi.json -x bool -j true)
  if [ "${true_byte}" != "01" ] ; then
    echo "error: ${PREFIX} expected byte 01 from bool true but got ${true_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Bool Test"

  # Int test ####################
  two_byte=$(../build/tools/generate_hex_from_json -f ../test/transaction.abi.json -x int8 -j 2)
  if [ "${two_byte}" != "02" ] ; then
    echo "error: ${PREFIX} expected byte 02 from 2 int8 but got ${two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Int8 as 2 Test"
  # Int test ####################
  neg_two_byte=$(../build/tools/generate_hex_from_json -f ../test/transaction.abi.json -x int8 -j -2)
  if [ "${neg_two_byte}" != "FE" ] ; then
    echo "error: ${PREFIX} expected byte FE from -2 int8 but got ${neg_two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Int8 as -2 Test"
  # name test ###################
  name_byte=$(../build/tools/generate_hex_from_json -f ../test/transaction.abi.json -x name -j '"bigliontest1"')
  if [ "${name_byte}" != "103256795217993B" ] ; then
    echo "error: ${PREFIX} expected byte 103256795217993B from name but got ${neg_two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Name Test"
else
  echo "Not Found ../build/tools/generate_hex_from_json"
  exit 1
fi


###################################################


PREFIX="JSON from HEX:"
if [ -f ../build/tools/generate_json_from_hex ]; then
    # no args test ###############
  if ../build/tools/generate_json_from_hex 2>>/dev/null ; then
    echo "error: no arguments and still success ../build/tools/generate_json_from_hex"
    exit 1
  fi
  echo "${PREFIX} Passed No Args Test"

  # download ABI ##################
  curl ${Transaction_ABI} --output ../test/transaction.abi.json 2>>/dev/null
  # Bool test #####################
  true_byte=$(../build/tools/generate_json_from_hex -f ../test/transaction.abi.json -x bool -h 01)
  if [ "${true_byte}" != "true" ] ; then
    echo "error: ${PREFIX} expected true from bool 01 but got ${true_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Bool Test"

  # Int test #####################
  two_byte=$(../build/tools/generate_json_from_hex -f ../test/transaction.abi.json -x int8 -h 02)
  if [ "${two_byte}" != "2" ] ; then
    echo "error: ${PREFIX} expected 2 from 02 int8 but got ${two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Int8 as 02 Test"
  # Int test #####################
  neg_two_byte=$(../build/tools/generate_json_from_hex -f ../test/transaction.abi.json -x int8 -h FE)
  if [ "${neg_two_byte}" != "-2" ] ; then
    echo "error: ${PREFIX} expected -2 from FE int8 but got ${neg_two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Int8 as FE Test"
  # name test #####################
  name_byte=$(../build/tools/generate_json_from_hex -f ../test/transaction.abi.json -x name -h 103256795217993B)
  if [ "${name_byte}" != '"bigliontest1"' ] ; then
    echo "error: ${PREFIX} expected \"bigliontest1\" from name but got ${neg_two_byte}"
    exit 1
  fi
  echo "${PREFIX} Passed Name Test"
else
  echo "Not Found ../build/tools/generate_json_from_hex"
  exit 1
fi

# clean up
[ -f ../test/transaction.abi.json ] && rm ../test/transaction.abi.json
echo "Success all tests passed"
