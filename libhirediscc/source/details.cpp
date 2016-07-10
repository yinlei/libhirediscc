//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <hiredis.h>

#include <hirediscc/exception.h>
#include <hirediscc/details.h>

namespace hirediscc {

namespace details {

void deleteRedisReply(redisReply * reply) {
    if (reply != nullptr)
        ::freeReplyObject(reply);
}

int32_t getRedisReplyType(redisReply *reply) {
    return int32_t(reply->type);
}

void deserializeRedisReply(redisReply * reply, std::string &result) {
    result.assign(reply->str, reply->len);
}

void deserializeRedisReply(redisReply * reply, int64_t &result) {
    result = reply->integer;
}

void deserializeRedisReply(redisReply * reply, std::vector<redisReply*> &result) {
    result.reserve(reply->elements);
    for (size_t i = 0; i < reply->elements; ++i)
        result.push_back(reply->element[i]);
}

redisReply * excute(redisContext * context) {
    redisReply *r = nullptr;
    auto ret = ::redisGetReply(context, reinterpret_cast<void**>(&r));
    if (ret != REDIS_OK) {
        throw Exception(ret);
    }
    return r;
}

}
}
