#include <regex>
#include <nlohmann/json.hpp>

#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/CPPUtil.h"

#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"

#include "util.h"

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


// #define DBG 1

using json = nlohmann::json;

static cl::opt<string> apispec("api_spec", cl::desc("Specify WebGL API spec file (json formate)"),
                        cl::value_desc("apispec"), cl::Required);

static cl::opt<string> ir("ir", cl::desc("Specify WebGL API IR file"),
                        cl::value_desc("IR file"), cl::Required);

static cl::opt<string> dump_api("dump_api", cl::desc("dump all the functions with some name"),
                        cl::value_desc("api name"));

bool parseApiArgTypes(string& raw, vector<string> &res) {
    auto lp1 = raw.find("(");
    auto lp2 = raw.rfind("(");
    auto rp1 = raw.rfind(")");
    auto rp2 = raw.find(")");

    res.clear();

    if (lp1 == string::npos || \
        rp1 == string::npos || \
        (lp1 != lp2) || (rp1 != rp2)) {
        return false;
    }
    auto s = lp1 + 1;
    auto e = rp1 - 1;
    int i;

    while (s < e) {
        int lb = 0;
        i = s;
        while (i < e) {
            if (raw[i] == '<') {
                lb ++;
            } else if (raw[i] == '>') {
                lb --;
            } else if (raw[i] == ',') {
                if (lb == 0) {
                    break;
                }
            }

            i++;
        }

        int l = i - s;
        if (i == e)
            l += 1;

        string t = trim(raw.substr(s, l));

        // 1. getBufferParameter
        // getProgramParameter etc
        // has an extra blink::ScriptState as the first argument
        // we ignore them

        // 2. makeXRCompatible
        // texImage2D etc
        // take an extra argument of type
        // blink::ExceptionState& (the last argument)
        // we remove it here

        // 2. texImagexD etc
        // take an extra argument of type
        // blink::ExecutionContext* (the first argument)
        // we remove it here

        if (0 != t.compare("blink::ScriptState*") &&
            0 != t.compare("blink::ExceptionState&") &&
            0 != t.compare("blink::ExecutionContext*"))
            res.push_back(t);

        s = i + 1;
    }

    return true;
}


bool match(struct cppUtil::DemangledName &dname, json &spec, int version) {
#ifdef DBG
    cout << "----------- begin match ---------\n";
    cout << "rawName:" << rawName << endl;
#endif

    string &rawName = dname.rawName;
    bool ret = true;
    vector<string> res;
    if (!parseApiArgTypes(rawName, res)) {
        cout << "failed to parse the arg types" << endl;
    } else {

#ifdef DBG
        cout << "parsed result: " << endl;
        for (int i = 0; i < res.size(); i ++) {
            cout << " - arg " << i << endl;
            cout << "  - type " << res[i] << endl;
        }
#endif

    }

    json &args = spec["args"];

#ifdef DBG
    cout << "spec of << " << spec["name"].get<string>()  "(id=" << spec["id"].get<int>() <<  ") :" << endl;
    for (int i = 0; i < args.size(); i ++) {
        cout << " - arg " << i << endl;
        cout << "  - name: " << args[i]["name"] << endl;
        cout << "  - arg_type: " << args[i]["arg_type"] << endl;
    }
    cout << "----------- end match ---------\n";
#endif

    if (res.size() != args.size()) {
        ret = false;
    } else {

        cout << "matching spec of << " << spec["name"].get<string>()
             << "(id=" << spec["id"].get<int>() <<  ") :" << endl;
        for (int i = 0; i < args.size(); i ++) {

            string &farg = res[i];
            string sarg = args[i]["arg_type"].get<string>();

            cout << "  matching arg " << i << "`"
                 << sarg
                 << "` -> `" << farg
                 << "`" << endl;

            // handle ArrayBufferView
            // and ArrayBuffer args
            auto pt = sarg.rfind("OrNull");
            if (pt != string::npos) {
                string t = sarg.substr(0, pt);
                if (farg.find(t) == string::npos) {
                    ret = false;
                    break;
                }

                // ArrayBuffer does not match ArrayBufferView
                auto vt = t.rfind("View");
                if (vt == string::npos && farg.find(t + "View") != string::npos) {
                    ret = false;
                    break;
                }
            }

            // handle GLsizexxptr
            if (sarg.find("GL") == 0 && sarg.rfind("ptr") != string::npos) {
                if (farg != "long") {
                    ret = false;
                    break;
                }
            }

            // handle ImageData
            if (sarg == "ImageData" && farg.find(sarg) == string::npos) {
                ret = false;
                break;
            }

            // handle texImagenD
            // HTMLImageElement
            // HTMLVideoElement
            if (sarg.find("HTML") == 0 && sarg.find("HTMLCanvas") == string::npos &&
                farg.find(sarg) == string::npos) {
                ret = false;
                break;
            }

            // Handle HTMLCanvasElement
            if (sarg == "HTMLCanvasElement" && farg.find("VideoFrame") == string::npos) {
                ret = false;
                break;
            }

            // Handle OffscreenCanvas
            if (sarg == "OffscreenCanvas" && farg.find("CanvasRenderingContextHost") == string::npos) {
                ret = false;
                break;
            }


            if (sarg == "ImageBitmap" && farg.find("ImageBitmap") == string::npos) {
                ret = false;
                break;
            }


            // handle array vs sequence
            if (sarg.find("Array") != string::npos && farg.find("Array") == string::npos) {
                ret = false;
                break;
            }

            // GLfloatSequence
            if (sarg.find("Sequence") != string::npos && farg.find("Vector") == string::npos) {
                ret = false;
                break;
            }
        }
    
        if (version == 1 && dname.className.find("WebGL2") != string::npos) {
            ret = false;
        }
    }

    return ret;
}

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

    int sver = spec["version"].get<int>();

    json &apis = spec["apis"];
    map<int, APIInfo> api_res;
    // map<int, json> api_spec;
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

        // api_spec[i] = api;
        api_res[i] = {name, {}};

        name2ids[name].insert(id);
    }

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule({ir});

    for (auto it = svfModule->llvmFunBegin(); it != svfModule->llvmFunEnd(); it ++) {
        // cout << (*it)->getName().str() << endl;
        string mname = (*it)->getName().str();
        auto dname = cppUtil::demangle(mname);
        if (dname.className.find("blink::WebGL") == 0) {
/*
            cout << "oname:" << mname << endl;
            cout << "className: " <<  dname.className << endl;
            cout << "funcName: " << dname.funcName << endl;
*/

            if (dump_api != "" ) {
                static int i = 0;
                if ( dname.funcName == dump_api) {
                    cout << "i=" << i++ << endl;
                    cout << "oname:" << mname << endl;
                    cout << "className: " <<  dname.className << endl;
                    cout << "funcName: " << dname.funcName << endl;
                    cout << "dmangedName: " << dname.rawName << endl;
                    cout << "======================================\n";
                }
            } else {

                if (name2ids.find(dname.funcName) != name2ids.end()) {
                    for (auto &id : name2ids[dname.funcName]) {
                        seenIds.insert(id);

                        if (match(dname, apis[id], sver))
                            api_res[id].resolve[mname] = dname.rawName;
                    }
                }
            }

/*
            cout << "isThunkFunc: " << dname.isThunkFunc << endl;
            cout << "-------------------------\n";
*/

        }
    }

    if (dump_api != "") {
        return 0;
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

        cout << "------------------------------------\n\n\n\n";
    }

    return 0;
}
