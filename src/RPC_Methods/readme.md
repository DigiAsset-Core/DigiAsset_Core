To add new rpc methods create a new file named methodname.cpp and start it with


```c++
//
// Created by yourname on dd/mm/yy.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * description of method
     */
    extern const Json::Value methodname(const Json::Value& params) {
        //contents here
    }
}
```


then create an methodname.html to include user documentation for that method