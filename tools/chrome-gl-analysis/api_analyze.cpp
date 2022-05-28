#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/CPPUtil.h"

#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"

#include <nlohmann/json.hpp>

using namespace llvm;
using namespace std;
using namespace SVF;

struct APIInfo {
    string name;
    map<string, string> resolve;
};

struct LLVMFuncInfo {
    string classname;
    string funcName;
    int nrArgs;
};

/*
LLVMFuncInfo parseFuncFromDname(cppUtil::DemangledName &dname) {
    LLVMFuncInfo ret;
    return ret;
}
*/

using json = nlohmann::json;

cl::opt<string> apispec("api_spec", cl::desc("Specify WebGL API spec file (json formate)"), 
                        cl::value_desc("apispec"), cl::Required);

cl::opt<string> ir("ir", cl::desc("Specify WebGL API IR file"), 
                        cl::value_desc("IR file"), cl::Required);


int main(int argc, char **argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "Analyzing the llvm functions of each WebGL API\n");
 
    ifstream i(apispec);
    json spec;

    if (!i) {
        cout << "api_spec file not accessible: " << apispec << endl;
        return -1;
    }

    
    i >> spec;

    // int sver = spec["version"].get<int>();

    json &apis = spec["apis"];
    map<int, APIInfo> api_res;
    map<int, json> api_spec; 
    map<string, set<int>> name2ids;
    
    set<int> seenIds;

    for (int i = 0; i < apis.size(); i++) {
        json &api = apis[i];
        int id = api["id"].get<int>();
        string name = api["name"].get<string>();
/*
        json &args = api["args"];

        cout << "id:" << id << endl;
        cout << "name:" << name << endl;
        cout << "# args: " << args.size() << endl;
        cout << "=========================\n";
*/
        api_spec[i] = api;
        api_res[i] = {name, {}};

        name2ids[name].insert(id);
    }

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule({ir});

    for (auto it = svfModule->llvmFunBegin(); it != svfModule->llvmFunEnd(); it ++) {
        // cout << (*it)->getName().str() << endl;
        string mname = (*it)->getName().str();
        auto dname = cppUtil::demangle(mname);
        if (dname.className.find("blink::WebGLRendering") == 0) {
/*
            cout << "oname:" << mname << endl;
            cout << "className: " <<  dname.className << endl;
            cout << "funcName: " << dname.funcName << endl;
*/

            if (name2ids.find(dname.funcName) != name2ids.end()) {
                for (auto &id : name2ids[dname.funcName]) {
                    seenIds.insert(id);
                    api_res[id].resolve[mname] = dname.rawName;
                }
            }
/*
            cout << "isThunkFunc: " << dname.isThunkFunc << endl;
            cout << "-------------------------\n";
*/

        }
    }

    cout << "=========================\n"; 
    if (seenIds.size() == apis.size()) {
        cout << "all api names seen\n";
    } else {
        cout << "apis not seen:\n";
        for (int i = 0; i < apis.size(); i++) {
            if (seenIds.count(i) == 0) {
                cout << i << " not seen before" << endl;
            }
        }
    }

    cout << "=========================\n"; 

    for (int i = 0; i < apis.size(); i ++) {
        cout << "id: " << i << endl;
        APIInfo &info = api_res[i];
        cout << "name: " << info.name << endl;

        if (info.resolve.size() == 0) {
            cout << "id:" << i << ", api_name: " << info.name << " is not matched" << endl;
        } else if (info.resolve.size() > 1) {
            cout << "Multiple matches as follows: " << endl;
            for (auto it = info.resolve.begin(); it != info.resolve.end(); it++) {
                cout << "  - " << it->first << endl;
                cout << "      - " << it->second << endl;
            }

        }

        cout << "------------------------------------\n";
    }

    return 0;
}