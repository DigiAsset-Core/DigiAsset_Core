To add new rpc methods create a new file named methodname.cpp and start it with


```c++
//
// Created by yourname on dd/mm/yy.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * description of method
        */
        extern const Response methodname(const Json::Value& params) {
            //contents here
            Json::Value result;
            
            
            
            //return response
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(n);   //optional defaults 0.
                                            //-1 will not cache at all
                                            //0 will cache until next block is added
                                            //1 or greater will cachec for that many blocks are added
            response.addInvalidateOnAddressChange(address); //optional sets to cache until this address changes
                                                            //can add as many addresses as needed
                                                            //cache will last the shorter of the 2 limits
            return response;
        }
        
    }
}
```


then create an methodname.html to include user documentation for that method