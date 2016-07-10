//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdexcept>

namespace hirediscc {

class Exception : public std::exception {
public:
    Exception() = default;

    explicit Exception(int error);

    virtual char const * what() const override;
private:
    int error_;
};

}
