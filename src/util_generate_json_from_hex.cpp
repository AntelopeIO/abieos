//
// Created by Eric Passmore on 1/19/23.
// Purpose: command line option to generate hex code from JSON
//   may be used to generate deserialization tests cases in other languages and external packages
//

#include "abieos.h"
#include <string>
#include <stdexcept>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

enum ABI_CONTRACT {TRANSACT_ABI, PACKED_TRANSACTION_ABI, STATE_HISTORY_ABI};

extern const char* const state_history_plugin_abi;

const char transactionAbi[] = R"({
    "version": "eosio::abi/1.0",
    "types": [
        {
            "new_type_name": "account_name",
            "type": "name"
        },
        {
            "new_type_name": "action_name",
            "type": "name"
        },
        {
            "new_type_name": "permission_name",
            "type": "name"
        }
    ],
    "structs": [
        {
            "name": "permission_level",
            "base": "",
            "fields": [
                {
                    "name": "actor",
                    "type": "account_name"
                },
                {
                    "name": "permission",
                    "type": "permission_name"
                }
            ]
        },
        {
            "name": "action",
            "base": "",
            "fields": [
                {
                    "name": "account",
                    "type": "account_name"
                },
                {
                    "name": "name",
                    "type": "action_name"
                },
                {
                    "name": "authorization",
                    "type": "permission_level[]"
                },
                {
                    "name": "data",
                    "type": "bytes"
                }
            ]
        },
        {
            "name": "extension",
            "base": "",
            "fields": [
                {
                    "name": "type",
                    "type": "uint16"
                },
                {
                    "name": "data",
                    "type": "bytes"
                }
            ]
        },
        {
            "name": "transaction_header",
            "base": "",
            "fields": [
                {
                    "name": "expiration",
                    "type": "time_point_sec"
                },
                {
                    "name": "ref_block_num",
                    "type": "uint16"
                },
                {
                    "name": "ref_block_prefix",
                    "type": "uint32"
                },
                {
                    "name": "max_net_usage_words",
                    "type": "varuint32"
                },
                {
                    "name": "max_cpu_usage_ms",
                    "type": "uint8"
                },
                {
                    "name": "delay_sec",
                    "type": "varuint32"
                }
            ]
        },
        {
            "name": "transaction",
            "base": "transaction_header",
            "fields": [
                {
                    "name": "context_free_actions",
                    "type": "action[]"
                },
                {
                    "name": "actions",
                    "type": "action[]"
                },
                {
                    "name": "transaction_extensions",
                    "type": "extension[]"
                }
            ]
        }
    ]
})";
const char packedTransactionAbi[] = R"({
    "version": "eosio::abi/1.0",
    "types": [
        {
            "new_type_name": "account_name",
            "type": "name"
        },
        {
            "new_type_name": "action_name",
            "type": "name"
        },
        {
            "new_type_name": "permission_name",
            "type": "name"
        }
    ],
    "structs": [
        {
            "name": "permission_level",
            "base": "",
            "fields": [
                {
                    "name": "actor",
                    "type": "account_name"
                },
                {
                    "name": "permission",
                    "type": "permission_name"
                }
            ]
        },
        {
            "name": "action",
            "base": "",
            "fields": [
                {
                    "name": "account",
                    "type": "account_name"
                },
                {
                    "name": "name",
                    "type": "action_name"
                },
                {
                    "name": "authorization",
                    "type": "permission_level[]"
                },
                {
                    "name": "data",
                    "type": "bytes"
                }
            ]
        },
        {
            "name": "extension",
            "base": "",
            "fields": [
                {
                    "name": "type",
                    "type": "uint16"
                },
                {
                    "name": "data",
                    "type": "bytes"
                }
            ]
        },
        {
            "name": "transaction_header",
            "base": "",
            "fields": [
                {
                    "name": "expiration",
                    "type": "time_point_sec"
                },
                {
                    "name": "ref_block_num",
                    "type": "uint16"
                },
                {
                    "name": "ref_block_prefix",
                    "type": "uint32"
                },
                {
                    "name": "max_net_usage_words",
                    "type": "varuint32"
                },
                {
                    "name": "max_cpu_usage_ms",
                    "type": "uint8"
                },
                {
                    "name": "delay_sec",
                    "type": "varuint32"
                }
            ]
        },
        {
            "name": "transaction",
            "base": "transaction_header",
            "fields": [
                {
                    "name": "context_free_actions",
                    "type": "action[]"
                },
                {
                    "name": "actions",
                    "type": "action[]"
                },
                {
                    "name": "transaction_extensions",
                    "type": "extension[]"
                }
            ]
        },
        {
            "name": "packed_transaction_v0",
            "base": "",
            "fields": [
                {
                    "name": "signatures",
                    "type": "signature[]"
                },
                {
                    "name": "compression",
                    "type": "uint8"
                },
                {
                    "name": "packed_context_free_data",
                    "type": "bytes"
                },
                {
                    "name": "packed_trx",
                    "type": "transaction"
                }
            ]
        }
    ]
})";

// figure out the ABI Contract to use
uint64_t get_contract_id(ABI_CONTRACT abiContractName) {
    uint64_t contractId;

    switch (abiContractName) {
    case TRANSACT_ABI: contractId = 0; break;
    case PACKED_TRANSACTION_ABI: contractId = 1; break;
    case STATE_HISTORY_ABI: contractId = 2; break;
    default: throw std::runtime_error("unknown ABI contract name in get_contract");
    }

    return contractId;
}

// Main work done here four steps
//      1) create empty context
//      2) set the context
//      4) create json from hex
// abiContractName: enum with contract names
// schema: the name of the data type or reference to schema in the ABI contract
// hex: hex encoded schema
// verbose: flag to print out step by step messages
std::string generate_json_from_hex(ABI_CONTRACT abiContractName, const char *schema, const char *hex, bool verbose) {
    if (verbose) fprintf(stderr, "Schema is: %s and hex is %s\n\n",schema,hex);

    // create empty context
    abieos_context_s *context = abieos_create();
    if (! context) throw std::runtime_error("unable to create context");
    if (verbose) fprintf(stderr,"step 1 of 3: created empty ABI context\n");

    // set the transaction contexts. set for each type we support.
    bool successTransactionAbi = abieos_set_abi(context, 0, transactionAbi);
    bool successPackedTransactionAbi = abieos_set_abi(context, 1, packedTransactionAbi);
    // state history abi defined in ship.abi.cpp
    bool successStateHistoryAbi = abieos_set_abi(context, 2, state_history_plugin_abi);
    if (! (successTransactionAbi && successPackedTransactionAbi && successStateHistoryAbi )) {
        throw std::runtime_error("unable to set context");
    }
    if (verbose) fprintf(stderr,"step 2 of 3: established context for transactions, packed transactions, and state history\n");

    // convert hex to json
    std::string json = abieos_hex_to_json(
        context,
        get_contract_id(abiContractName),
        schema,
        hex
    );
    if (verbose) fprintf(stderr,"step 3 of 3: converted hex to json\n\n");
    return json;
}

// prints usage
void help(const char* exec_name) {
    fprintf(stderr, "Usage %s: [-t|-p|-s] [-v] -j JSON -x type\n", exec_name);
    fprintf(stderr, "\t-t abi transaction: default\n");
    fprintf(stderr, "\t-p abi packed transaction\n");
    fprintf(stderr, "\t-s state history\n");
    fprintf(stderr, "\t-v verbose, print out steps\n");
    fprintf(stderr,"\t-h hex: string to convert to json\n");
    fprintf(stderr,"\t-x type: a specific data type or schema section (example uint16, action, name, uint8[])\n");
    fprintf(stderr, "\texample: generate_hex_from_json -x bool -j true\n\n");
}

int main(int argc, char *argv[]) {
    std::string json;
    std::string type;
    std::string hex;
    ABI_CONTRACT abiContractName = TRANSACT_ABI;
    bool verbose = false;
    int opt;

    try {
        while ((opt = getopt(argc, argv, "tpsvh:x:")) != -1) {
            switch (opt) {
            case 't': abiContractName = TRANSACT_ABI; break;
            case 'p': abiContractName = PACKED_TRANSACTION_ABI; break;
            case 's': abiContractName = STATE_HISTORY_ABI; break;
            case 'v': verbose = true; break;
            case 'h': hex = optarg; break;
            case 'x': type = optarg; break;
            case '?':
                if ('c' == optopt) {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                }
                exit(EXIT_FAILURE);
            default:
                exit(EXIT_FAILURE);
            }
        }

        // check for required params
        if (hex.empty() || type.empty()) {
            help(*argv);
            exit(EXIT_FAILURE);
        }

        json = generate_json_from_hex(
            abiContractName,
            type.c_str(),
            hex.c_str(),
            verbose
        );
        if (json.length() > 0) {
            printf("%s\n",json.c_str());
        } else {
            fprintf(stderr,"no json value\n");
        }
        return 0;
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
        return 1;
    }
}