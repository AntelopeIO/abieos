//
// Created by Eric Passmore on 1/14/23.
// Purpose: command line option to generate hex code from JSON
//   may be used to generate deserialization tests cases in other languages and external packages
//

#include "abieos.h"
#include "abieos.hpp"
#include <string>
#include <stdexcept>
#include <unistd.h>

enum {TRANSACT_ABI, PACKED_TRANSACTION_ABI} abi_context = TRANSACT_ABI;

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

// Main work done here
std::string generate_hex_from_json(const char *json, size_t json_size) {
    std::string hex = "ABC";
    printf("inside generate func with json %s with size %zu\n",json, json_size);
    return hex;
}

// prints usage
void help(const char* exec_name) {
    fprintf(stderr, "Usage %s: [-t|-p] -j JSON \n", exec_name);
    fprintf(stderr, "\t-t abi transaction or -p abi packed transaction: defaults to -t abi transaction\n");
    fprintf(stderr, "\t json string limited to 256 bytes\n\n");
}

// makes sure input string conforms to 256 byte limit
// might as well calc string size
size_t find_json_length(char *json) {
    size_t true_length = 0;
    while(*(json + true_length) != '\0') {
        ++true_length;
        // already incremented position is beyond last check
        if (true_length > 255) {
            json[true_length] = '\0';
            break;
        }
    }
    return true_length;
}

int main(int argc, char *argv[]) {
    try {
        char *json = nullptr;

        int opt;
        while ((opt = getopt(argc, argv, "tpj:")) != -1) {
            switch (opt) {
            case 't': abi_context = TRANSACT_ABI; break;
            case 'p': abi_context = PACKED_TRANSACTION_ABI; break;
            case 'j': json = optarg; break;
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

        if (json == nullptr) {
            help(*argv);
            exit(EXIT_FAILURE);
        }
        size_t json_size = find_json_length(json);
        generate_hex_from_json(json, json_size);
        return 0;
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
        return 1;
    }
}