#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/CPPUtil.h"

#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"

using namespace llvm;
using namespace std;
using namespace SVF;

int main(int argc, char **argv) {
    int arg_num = 0;
    char **arg_value = new char*[argc];
    std::vector<std::string> moduleNameVec;

    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Analyzing the llvm functions of each WebGL API\n");   

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);


    for (auto it = svfModule->llvmFunBegin(); it != svfModule->llvmFunEnd(); it ++) {
        // cout << (*it)->getName().str() << endl;
        string mname = (*it)->getName().str();
        auto dname = cppUtil::demangle(mname);
        if (dname.className.find("blink::WebGLRendering") == 0) {
            cout << "oname:" << mname << endl;
            cout << "className: " <<  dname.className << endl;
            cout << "funcName: " << dname.funcName << endl;
            cout << "isThunkFunc: " << dname.isThunkFunc << endl;
            cout << "-------------------------\n";
        }
    }

    delete[] arg_value;
    return 0;
}