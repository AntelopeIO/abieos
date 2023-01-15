//
// Created by Eric Passmore on 1/14/23.
// Purpose: command line option to generate hex code from JSON
//   may be used to generate deserialization tests cases in other languages and external packages
//

#include "abieos.h"
#include <string>
#include <stdexcept>
#include <unistd.h>

enum ABI_CONTRACT {TRANSACT_ABI, PACKED_TRANSACTION_ABI, STATE_HISTORY_ABI};

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

// Main work done here
// abiContractName: enum with contract names
// schema: the name of the data type or reference to schema in the ABI contract
// json: the name and values
std::string generate_hex_from_json(ABI_CONTRACT abiContractName, const char *schema, const char *json) {
    printf("Schema is: %s and json is %s\n\n",schema,json);
    schema = "error_message"; json= R"({"error_code":"1","error_msg":"foo one"})";

    // create empty context
    abieos_context_s *context = abieos_create();
    if (! context) throw std::runtime_error("unable to create context");

    // convert from json to binary. binary stored with context
    // get contract id returns integer for the ABI contract we passed in by name
    bool success = abieos_json_to_bin(context, get_contract_id(abiContractName), schema, json);
    if (!success) throw std::runtime_error("abieos json to bin returned failure");

    // now time to return the hex string
    return abieos_get_bin_hex(context);
}

// prints usage
void help(const char* exec_name) {
    fprintf(stderr, "Usage %s: [-t|-p|-s] -j JSON -x type\n", exec_name);
    fprintf(stderr, "\t-t abi transaction: default\n");
    fprintf(stderr, "\t-p abi packed transaction\n");
    fprintf(stderr, "\t-s state history\n");
    fprintf(stderr,"\t-j json: string to convert to hex\n");
    fprintf(stderr,"\t-x type: a specific data type or schema section (example uint16, action, name, uint8[])\n");
    fprintf(stderr, "\t json string limited to 256 bytes\n\n");
}

int main(int argc, char *argv[]) {
    std::string json;
    std::string type;
    std::string hex;
    ABI_CONTRACT abiContractName = TRANSACT_ABI;
    int opt;

    try {
        while ((opt = getopt(argc, argv, "tpsj:x:")) != -1) {
            switch (opt) {
            case 't': abiContractName = TRANSACT_ABI; break;
            case 'p': abiContractName = PACKED_TRANSACTION_ABI; break;
            case 's': abiContractName = STATE_HISTORY_ABI; break;
            case 'j': json = optarg; break;
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

        if (json.empty() || type.empty()) {
            help(*argv);
            exit(EXIT_FAILURE);
        }

        hex = generate_hex_from_json(abiContractName, type.c_str(), json.c_str());
        if (hex.length() > 0) {
            printf("%s\n",hex.c_str());
        } else {
            printf("no hex value\n");
        }
        return 0;
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
        return 1;
    }
}