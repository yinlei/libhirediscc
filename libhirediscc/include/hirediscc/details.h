//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <vector>
#include <string>

struct redisReply;
struct redisContext;

namespace hirediscc {

namespace details {

void deleteRedisReply(redisReply *reply);
int32_t getRedisReplyType(redisReply *reply);
void deserializeRedisReply(redisReply *reply, std::string &result);
void deserializeRedisReply(redisReply *reply, int64_t &result);
void deserializeRedisReply(redisReply *reply, std::vector<redisReply*> &result);
redisReply *excute(redisContext* context);

}

}