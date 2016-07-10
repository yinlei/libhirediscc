//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>

namespace hirediscc {

	class CommandArgs {
public:
    using Iterator = std::vector<std::string>::const_iterator;

    CommandArgs() = default;

    explicit CommandArgs(std::string const &arg) {
        argsList_.push_back(arg);
    }

    template <typename T>
    CommandArgs& operator<<(T const &arg) {
        argsList_.push_back(std::to_string(arg));
        return *this;
    }

    CommandArgs& operator<<(std::string const &arg) {
        argsList_.push_back(arg);
        return *this;
    }

    CommandArgs& operator<<(char const *arg) {
        argsList_.push_back(arg);
        return *this;
    }

    CommandArgs& operator<<(CommandArgs const &commandArgs) {
        argsList_.reserve(count() + commandArgs.count());
        argsList_.insert(std::end(argsList_),
            std::begin(commandArgs.argsList_),
            std::end(commandArgs.argsList_));
        return *this;
    }

    size_t count() const {
        return argsList_.size();
    }

    Iterator begin() const {
        return std::begin(argsList_);
    }

    Iterator end() const {
        return std::end(argsList_);
    }

private:
    std::vector<std::string> argsList_;
};


}
