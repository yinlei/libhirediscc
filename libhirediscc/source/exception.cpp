//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <hiredis.h>

#include <unordered_map>
#include <hirediscc/exception.h>

namespace hirediscc {

Exception::Exception(int error)
    : error_(error){
}

char const * Exception::what() const {
    static std::unordered_map<int, std::string> const lookupTable {
        { 
            REDIS_ERR_IO,
            "There was an I/O error while creating the connection, "
            "trying to write to the socket or read from the socket."
        },
        {
            REDIS_ERR_EOF,
            "The server closed the connection which resulted in an empty read."
        },
        {
            REDIS_ERR_OTHER, "Any other error."
        },
        { 
            REDIS_ERR_PROTOCOL,
            "There was an error while parsing the protocol."
        }
    };
    auto itr = lookupTable.find(error_);
    if (itr != lookupTable.end())
        return (*itr).second.c_str();
    return "";
}

}
