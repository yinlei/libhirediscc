//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <hirediscc/details.h>

namespace hirediscc {

#pragma region Reply

template <typename T, typename R>
class ReplyBase {
public:
	static ReplyBase<T, R> const NullType;

	enum Type {
		String = 1,
		Array,
		Interger,
		Null,
		Status,
		Error
	};

	ReplyBase()
		: type_(Null)
		, reply_(nullptr) {
	}

	ReplyBase(Type type)
		: type_(type)
		, reply_(nullptr) {
	}

	explicit ReplyBase(redisReply *reply)
		: type_(Null)
		, reply_(reply) {
		type_ = static_cast<Type>(details::getRedisReplyType(reply));
	}

	~ReplyBase() {
		details::deleteRedisReply(reply_);
	}

	R value() {
		return static_cast<T*>(this)->value();
	}

	void deserialize(redisReply *reply) {
		return static_cast<T*>(this)->deserialize(reply);
	}

	bool hasError() const noexcept {
		return false;
	}

	bool isInterger() const noexcept {
		return type_ == Interger;
	}

	bool isError() const noexcept {
		return type_ == Error;
	}

	bool isString() const noexcept {
		return type_ == String;
	}

	bool isStatus() const noexcept {
		return type_ == Status;
	}
protected:
	redisReply *reply_;
	Type type_;
};

template <typename T, typename R>
ReplyBase<T, R> const ReplyBase<T, R>::NullType;

class ReplyInterger : public ReplyBase<ReplyInterger, int64_t> {
public:
	ReplyInterger()
		: ReplyBase() {
	}

	explicit ReplyInterger(redisReply *reply)
		: ReplyBase(reply) {
		deserialize(reply);
	}

	ReplyInterger(ReplyInterger &&other) {
		*this = std::move(other);
	}

	ReplyInterger& operator=(ReplyInterger &&other) {
		if (this != &other) {
			reply_ = other.reply_;
			type_ = other.type_;
			value_ = other.value_;
			other.reply_ = nullptr;
			other.type_ = Null;
			other.value_ = 0;
		}
		return *this;
	}

	int64_t value() const {
		return value_;
	}

	void deserialize(redisReply *reply) {
		assert(isInterger());
		details::deserializeRedisReply(reply, value_);
	}

private:
	int64_t value_;
};

class ReplyString : public ReplyBase<ReplyString, std::string> {
public:
	using ValueType = std::string;

	ReplyString()
		: ReplyBase(String) {
	}

	explicit ReplyString(redisReply *reply)
		: ReplyBase(reply) {
		deserialize(reply);
	}

	ReplyString(ReplyString &&other) {
		*this = std::move(other);
	}

	ReplyString& operator=(ReplyString &&other) {
		if (this != &other) {
			reply_ = other.reply_;
			type_ = other.type_;
			value_ = std::move(other.value_);
			other.reply_ = nullptr;
			other.type_ = Null;
		}
		return *this;
	}

	std::string value() const {
		return value_;
	}

	void deserialize(redisReply *reply) {
		assert(isString() || isError() || isStatus());
		details::deserializeRedisReply(reply, value_);
	}
private:
	std::string value_;
};

template <typename T>
class ReplyArray : public ReplyBase<ReplyArray<T>, std::vector<T>> {
public:
	ReplyArray()
		: ReplyBase(Array) {
	}

	explicit ReplyArray(redisReply *reply)
		: ReplyBase(reply) {
		deserialize(reply);
	}

	ReplyArray(ReplyArray &&other) {
		*this = std::move(other);
	}

	ReplyArray& operator=(ReplyArray &&other) {
		if (this != &other) {
			reply_ = other.reply_;
			type_ = other.type_;
			value_ = std::move(other.value_);
			other.reply_ = nullptr;
			other.type_ = Null;
		}
		return *this;
	}

	std::vector<typename T::ValueType> value() const {
		std::vector<typename T::ValueType> values;
		values.reserve(value_.size());
		for (auto &v : value_)
			values.push_back(v.value());
		return values;
	}

	void deserialize(redisReply *reply) {
		std::vector<redisReply*> result;
		details::deserializeRedisReply(reply, result);

		auto itr = result.begin();
		value_.resize(result.size());

		for (auto &v : value_) {
			v.deserialize(*itr);
			++itr;
		}
	}
private:
	std::vector<T> value_;
};

#pragma endregion Reply

}
