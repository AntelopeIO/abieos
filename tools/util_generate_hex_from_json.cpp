//
// Purpose: command line option to generate hex code from JSON
//   may be used to generate serialization tests cases in other languages and external packages
//

#include <abieos.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

// Main work done here four steps
//      1) create empty context
//      2) set the context
//      3) parse json to binary
//      4) create hex from bin
// abiContractName: enum with contract names
// schema: the name of the data type or reference to schema in the ABI contract
// json: the name and values
// verbose: flag to print out step by step messages
std::string generate_hex_from_json(const char* abi_definition, const char* contract_name, const char* schema, const char* json, bool verbose) {
    if (verbose) std::cerr << "Schema is: " << schema << " and json is " << json << std::endl << std::endl;

    // create empty context
    abieos_context_s* context = abieos_create();
    if (! context) throw std::runtime_error("unable to create context");
    if (verbose) std::cerr << "step 1 of 4: created empty ABI context" << std::endl;

    // set the transaction context.
    // first get the contract_id
    uint64_t contract_id = abieos_string_to_name(context, contract_name);
    if (contract_id == 0) {
        std::cerr << "Error: abieos_string_to_name " << abieos_get_error(context) << std::endl;
        throw std::runtime_error("unable to set context");
    }
    // use our id and set the ABI
    bool successSettingAbi = abieos_set_abi(context, contract_id, abi_definition);
    if (! successSettingAbi) {
        std::cerr << "Error: abieos_set_abi " << abieos_get_error(context) << std::endl;
        throw std::runtime_error("unable to set context");
    }
    if (verbose) std::cerr << "step 2 of 4: established context for transactions, packed transactions, and state history" << std::endl;

    // convert from json to binary. binary stored with context
    // get contract id returns integer for the ABI contract we passed in by name
    bool successJsonToBin = abieos_json_to_bin_reorderable(
        context,
        contract_id,
        schema,
        json
        );
    if (!successJsonToBin) {
        std::cerr << "failed in step 3: using context " << contract_name << std::endl;
        throw std::runtime_error(abieos_get_error(context));
    }
    if (verbose) std::cerr << "step 3 of 4: completed parsing json to binary" << std::endl;

    // now time to return the hex string
    std::string hex = abieos_get_bin_hex(context);
    if (verbose) std::cerr << "step 4 of 4: converted binary to hex" << std::endl << std::endl;
    return hex;
}

// prints usage
void help(const char* exec_name) {
    std::cerr << "Usage " << exec_name << ": -f -j JSON -x type [-v]";
    std::cerr << "\t-f file with ABI definition\n";
    std::cerr << "\t-v verbose, print out steps\n";
    std::cerr << "\t-j json: string to convert to hex\n";
    std::cerr << "\t-x type: a specific data type or schema section (example uint16, action, name, uint8[])\n";
    std::cerr << "\texample: generate_hex_from_json -f ./transaction.abi -x bool -j true\n" << std::endl;
}

// reads file returning string of contents
std::string retrieveFileContents(const std::string &filename ) {
    try {
        std::ifstream ifs(filename, std::ios::in);
        std::string string_of_file_contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return string_of_file_contents;
    } catch(std::filesystem::filesystem_error const& ex) {
        std::cerr << "unable to read ABI file at path: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    // static string for our contract id
    static const char* contract_name = "eosio";
    // input string to transform to hex code
    std::string json;
    // schema name ex: bool
    std::string type;
    // hex value returned ex: 01
    std::string hex;
    // file containing ABI definition
    std::string abiFileName;
    // string with contents of ABI file
    std::string abiDefinition;
    // default verbose setting
    bool verbose = false;
    int opt;

    try {
        while ((opt = getopt(argc, argv, "vf:j:x:")) != -1) {
            switch (opt) {
            case 'f': abiFileName = optarg; break;
            case 'v': verbose = true; break;
            case 'j': json = optarg; break;
            case 'x': type = optarg; break;
            case '?':
                exit(EXIT_FAILURE);
            default:
                exit(EXIT_FAILURE);
            }
        }

        // check for required params
        if (json.empty() || type.empty() || abiFileName.empty()) {
            help(*argv);
            exit(EXIT_FAILURE);
        }

        // this is an important step
        // here we grab the ABI definition and stuff it in a string
        abiDefinition = retrieveFileContents(abiFileName);

        hex = generate_hex_from_json(
            abiDefinition.c_str(),
            contract_name,
            type.c_str(),
            json.c_str(),
            verbose
        );
        if (hex.length() > 0) {
            std::cout << hex << std::endl;
        } else {
            std::cerr << "returned empty" << std::endl;
        }
        return 0;
    } catch (std::exception& e) {
        std::cerr << "Could not compute hex value: " << e.what() << std::endl;
        return 1;
    }
}