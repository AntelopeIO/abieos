//
// Created by Eric Passmore on 1/19/23.
// Purpose: command line option to generate hex code from JSON
//   may be used to generate deserialization tests cases in other languages and external packages
//

#include "../src/abieos.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

// Main work done here four steps
//      1) create empty context
//      2) set the context
//      4) create json from hex
// abiContractName: enum with contract names
// schema: the name of the data type or reference to schema in the ABI contract
// hex: hex encoded schema
// verbose: flag to print out step by step messages
std::string generate_json_from_hex(const char *abi_definition, const char *contract_name, const char *schema, const char *hex, bool verbose) {
    if (verbose) fprintf(stderr, "Schema is: %s and hex is %s\n\n",schema,hex);

    // create empty context
    abieos_context_s *context = abieos_create();
    if (! context) throw std::runtime_error("unable to create context");
    if (verbose) fprintf(stderr,"step 1 of 3: created empty ABI context\n");

    // set the transaction context.
    // first get the contract_id
    uint64_t contract_id = abieos_string_to_name(context, contract_name);
    if (contract_id == 0) {
        fprintf(stderr, "Error: abieos_string_to_name %s\n", abieos_get_error(context));
        throw std::runtime_error("unable to set context");
    }
    // use our id and set the ABI
    bool successSettingAbi = abieos_set_abi(context, contract_id, abi_definition);
    if (! successSettingAbi) {
        fprintf(stderr, "Error: abieos_set_abi %s\n", abieos_get_error(context));
        throw std::runtime_error("unable to set context");
    }
    if (verbose) fprintf(stderr,"step 2 of 3: established context for transactions, packed transactions, and state history\n");

    // convert hex to json
    std::string json = abieos_hex_to_json(
        context,
        contract_id,
        schema,
        hex
    );
    if (verbose) fprintf(stderr,"step 3 of 3: converted hex to json\n\n");
    return json;
}

// prints usage
void help(const char* exec_name) {
    fprintf(stderr, "Usage %s: -f -j JSON -x type [-v]\n", exec_name);
    fprintf(stderr, "\t-f file with ABI definition\n");
    fprintf(stderr, "\t-v verbose, print out steps\n");
    fprintf(stderr,"\t-h hex: string to convert to json\n");
    fprintf(stderr,"\t-x type: a specific data type or schema section (example uint16, action, name, uint8[])\n");
    fprintf(stderr, "\texample: generate_hex_from_json -f ./transaction.abi -x bool -j true\n\n");
}

// reads file returning string of contents
std::string retrieveFileContents(const std::string &filePath ) {
    try {
        size_t fileSize = static_cast<size_t>(std::filesystem::file_size(filePath));

        if (fileSize < 6) {
            fprintf(stderr, "Too Small: ABI file at path: %s does not have enough content\n", filePath.c_str());
            exit(EXIT_FAILURE);
        }

        std::ifstream ifs(filePath.c_str(), std::ios::in);
        std::vector<char> bytes(fileSize);
        ifs.read(bytes.data(), fileSize);

        return {bytes.data(), static_cast<size_t>(fileSize)};

    } catch(std::filesystem::filesystem_error const& ex) {
        fprintf(stderr, "unable to read ABI file at path: %s\n", filePath.c_str());
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    // static string for our contract id
    static const char *contract_name = "eosio";
    // string returned
    std::string json;
    // schema name ex: bool
    std::string type;
    // hex value ex: 01
    std::string hex;
    // file containing ABI definition
    std::string abiFileName;
    // string with contents of ABI file
    std::string abiDefinition;
    // default verbose setting
    bool verbose = false;
    int opt;

    try {
        while ((opt = getopt(argc, argv, "vf:h:x:")) != -1) {
            switch (opt) {
            case 'f': abiFileName = optarg; break;
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
        if (hex.empty() || type.empty() || abiFileName.empty()) {
            help(*argv);
            exit(EXIT_FAILURE);
        }

        // this is an important step
        // here we grab the ABI definition and stuff it in a string
        abiDefinition = retrieveFileContents(abiFileName);

        json = generate_json_from_hex(
            abiDefinition.c_str(),
            contract_name,
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