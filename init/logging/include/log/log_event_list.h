/*
 * Copyright (C) 2005-2016 The Linux Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <errno.h>
#include <stdint.h>

#ifdef __cplusplus
#include <string>
#endif

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* For manipulating lists of events. */

#define LINUX_MAX_LIST_NEST_DEPTH 8

/*
 * The opaque context used to manipulate lists of events.
 */
typedef struct linux_log_context_internal *linux_log_context;

/*
 * Elements returned when reading a list of events.
 */
typedef struct {
	LinuxEventLogType type;
	uint16_t complete;
	uint16_t len;
	union {
		int32_t int32;
		int64_t int64;
		char *string;
		float float32;
	} data;
} linux_log_list_element;

/*
 * Creates a context associated with an event tag to write elements to
 * the list of events.
 */
linux_log_context create_linux_logger(uint32_t tag);

/* All lists must be braced by a begin and end call */
/*
 * NB: If the first level braces are missing when specifying multiple
 *     elements, we will manufacture a list to embrace it for your API
 *     convenience. For a single element, it will remain solitary.
 */
int linux_log_write_list_begin(linux_log_context ctx);
int linux_log_write_list_end(linux_log_context ctx);

int linux_log_write_int32(linux_log_context ctx, int32_t value);
int linux_log_write_int64(linux_log_context ctx, int64_t value);
int linux_log_write_string8(linux_log_context ctx, const char *value);
int linux_log_write_string8_len(linux_log_context ctx, const char *value,
				size_t maxlen);
int linux_log_write_float32(linux_log_context ctx, float value);

/* Submit the composed list context to the specified logger id */
/* NB: LOG_ID_EVENTS and LOG_ID_SECURITY only valid binary buffers */
int linux_log_write_list(linux_log_context ctx, log_id_t id);

/*
 * Creates a context from a raw buffer representing a list of events to be read.
 */
linux_log_context create_linux_log_parser(const char *msg, size_t len);

linux_log_list_element linux_log_read_next(linux_log_context ctx);
linux_log_list_element linux_log_peek_next(linux_log_context ctx);

/* Reset writer context */
int linux_log_reset(linux_log_context ctx);

/* Reset reader context */
int linux_log_parser_reset(linux_log_context ctx, const char *msg, size_t len);

/* Finished with reader or writer context */
int linux_log_destroy(linux_log_context *ctx);

#ifdef __cplusplus
/* linux_log_list C++ helpers */
extern "C++" {
class linux_log_event_list {
    private:
	linux_log_context ctx;
	int ret;

	linux_log_event_list(const linux_log_event_list &) = delete;
	void operator=(const linux_log_event_list &) = delete;

    public:
	explicit linux_log_event_list(int tag) : ret(0)
	{
		ctx = create_linux_logger(static_cast<uint32_t>(tag));
	}
	~linux_log_event_list()
	{
		linux_log_destroy(&ctx);
	}

	int close()
	{
		int retval = linux_log_destroy(&ctx);
		if (retval < 0)
			ret = retval;
		return retval;
	}

	/* To allow above C calls to use this class as parameter */
	operator linux_log_context() const
	{
		return ctx;
	}

	/* return errors or transmit status */
	int status() const
	{
		return ret;
	}

	int begin()
	{
		int retval = linux_log_write_list_begin(ctx);
		if (retval < 0)
			ret = retval;
		return ret;
	}
	int end()
	{
		int retval = linux_log_write_list_end(ctx);
		if (retval < 0)
			ret = retval;
		return ret;
	}

	linux_log_event_list &operator<<(int32_t value)
	{
		int retval = linux_log_write_int32(ctx, value);
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(uint32_t value)
	{
		int retval =
			linux_log_write_int32(ctx, static_cast<int32_t>(value));
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(bool value)
	{
		int retval = linux_log_write_int32(ctx, value ? 1 : 0);
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(int64_t value)
	{
		int retval = linux_log_write_int64(ctx, value);
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(uint64_t value)
	{
		int retval =
			linux_log_write_int64(ctx, static_cast<int64_t>(value));
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(const char *value)
	{
		int retval = linux_log_write_string8(ctx, value);
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(const std::string &value)
	{
		int retval = linux_log_write_string8_len(ctx, value.data(),
							 value.length());
		if (retval < 0)
			ret = retval;
		return *this;
	}

	linux_log_event_list &operator<<(float value)
	{
		int retval = linux_log_write_float32(ctx, value);
		if (retval < 0)
			ret = retval;
		return *this;
	}

	int write(log_id_t id = LOG_ID_EVENTS)
	{
		/* facilitate -EBUSY retry */
		if ((ret == -EBUSY) || (ret > 0))
			ret = 0;
		int retval = linux_log_write_list(ctx, id);
		/* existing errors trump transmission errors */
		if (!ret)
			ret = retval;
		return ret;
	}

	int operator<<(log_id_t id)
	{
		write(id);
		linux_log_destroy(&ctx);
		return ret;
	}

	/*
   * Append<Type> methods remove any integer promotion
   * confusion, and add access to string with length.
   * Append methods are also added for all types for
   * convenience.
   */

	bool AppendInt(int32_t value)
	{
		int retval = linux_log_write_int32(ctx, value);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}

	bool AppendLong(int64_t value)
	{
		int retval = linux_log_write_int64(ctx, value);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}

	bool AppendString(const char *value)
	{
		int retval = linux_log_write_string8(ctx, value);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}

	bool AppendString(const char *value, size_t len)
	{
		int retval = linux_log_write_string8_len(ctx, value, len);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}

	bool AppendString(const std::string &value)
	{
		int retval = linux_log_write_string8_len(ctx, value.data(),
							 value.length());
		if (retval < 0)
			ret = retval;
		return ret;
	}

	bool Append(const std::string &value)
	{
		int retval = linux_log_write_string8_len(ctx, value.data(),
							 value.length());
		if (retval < 0)
			ret = retval;
		return ret;
	}

	bool AppendFloat(float value)
	{
		int retval = linux_log_write_float32(ctx, value);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}

	template <typename Tvalue> bool Append(Tvalue value)
	{
		*this << value;
		return ret >= 0;
	}

	bool Append(const char *value, size_t len)
	{
		int retval = linux_log_write_string8_len(ctx, value, len);
		if (retval < 0)
			ret = retval;
		return ret >= 0;
	}
};
}
#endif

#ifdef __cplusplus
}
#endif
