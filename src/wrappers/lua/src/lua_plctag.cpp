
extern "C" {
#include "platform.h"
}
#include "lib/libplctag.h"
#include "lib/tag.h"

#define SOL_CHECK_ARGUMENTS 1
#include "sol/sol.hpp"

#define LIB_PLC_TAG_VERSION 2.2

bool plc_tag_get_bool(int32_t id, int offset);
int plc_tag_set_bool(int32_t id, int offset, bool val);

std::string plc_tag_get_string(int32_t id, int offset);
int plc_tag_set_string(int32_t id, int offset, const std::string& val);

namespace lua_module {
	sol::table open_module(sol::this_state L) {
		sol::state_view lua(L);
		sol::table module = lua.create_table();

		module.new_enum("Status",
			"PENDING", PLCTAG_STATUS_PENDING,
			"OK", PLCTAG_STATUS_OK
		);

		module.new_enum("Error",
			"BAD_CONFIG", PLCTAG_ERR_BAD_CONNECTION,
			"BAD_CONNECTION", PLCTAG_ERR_BAD_CONNECTION,
			"BAD_DATA", PLCTAG_ERR_BAD_DATA,
			"BAD_DEVICE", PLCTAG_ERR_BAD_DEVICE,
			"BAD_GATEWAY", PLCTAG_ERR_BAD_GATEWAY,
			"BAD_PARAM", PLCTAG_ERR_BAD_PARAM,
			"BAD_REPLY", PLCTAG_ERR_BAD_REPLY,
			"BAD_STATUS", PLCTAG_ERR_BAD_STATUS,
			"CLOSE", PLCTAG_ERR_CLOSE,
			"CREATE", PLCTAG_ERR_CREATE,
			"DUPLICATE", PLCTAG_ERR_DUPLICATE,
			"ENCODE", PLCTAG_ERR_ENCODE,
			"MUTEX_DESTROY", PLCTAG_ERR_MUTEX_DESTROY,
			"MUTEX_INIT", PLCTAG_ERR_MUTEX_INIT,
			"MUTEX_LOCK", PLCTAG_ERR_MUTEX_LOCK,
			"MUTEX_UNLOCK", PLCTAG_ERR_MUTEX_UNLOCK,
			"NOT_ALLOWED", PLCTAG_ERR_NOT_ALLOWED,
			"NOT_FOUND", PLCTAG_ERR_NOT_FOUND,
			"NOT_IMPLEMENTED", PLCTAG_ERR_NOT_IMPLEMENTED,
			"NO_DATA", PLCTAG_ERR_NO_DATA,
			"NO_MATCH", PLCTAG_ERR_NO_MATCH,
			"NO_MEM", PLCTAG_ERR_NO_MEM,
			"NO_RESOURCES", PLCTAG_ERR_NO_RESOURCES,
			"NULL_PTR", PLCTAG_ERR_NULL_PTR,
			"OPEN", PLCTAG_ERR_OPEN,
			"OUT_OF_BOUNDS", PLCTAG_ERR_OUT_OF_BOUNDS,
			"READ", PLCTAG_ERR_READ,
			"REMOTE_ERR", PLCTAG_ERR_REMOTE_ERR,
			"THREAD_CREATE", PLCTAG_ERR_THREAD_CREATE,
			"THREAD_JOIN", PLCTAG_ERR_THREAD_JOIN,
			"TIMEOUT", PLCTAG_ERR_TIMEOUT,
			"TOO_LARGE", PLCTAG_ERR_TOO_LARGE,
			"TOO_SMALL", PLCTAG_ERR_TOO_SMALL,
			"UNSUPPORTED", PLCTAG_ERR_UNSUPPORTED,
			"WINSOCK", PLCTAG_ERR_WINSOCK,
			"WRITE", PLCTAG_ERR_WRITE,
			"PARTIAL", PLCTAG_ERR_PARTIAL,
			"BUSY", PLCTAG_ERR_BUSY
		);

		module.new_enum("DebugLevel",
			"NONE", PLCTAG_DEBUG_NONE,
			"ERROR", PLCTAG_DEBUG_ERROR,
			"WARN", PLCTAG_DEBUG_WARN,
			"INFO", PLCTAG_DEBUG_INFO,
			"DETAIL", PLCTAG_DEBUG_DETAIL,
			"SPEW", PLCTAG_DEBUG_SPEW
		);

		module.set_function("decode_error", plc_tag_decode_error );
		module.set_function("set_debug_level", plc_tag_set_debug_level );
		module.set_function("check_lib_version", plc_tag_check_lib_version );
		module.set_function("shutdown", plc_tag_shutdown );

		module.set_function("create", plc_tag_create );
		module.set_function("lock", plc_tag_lock );
		module.set_function("unlock", plc_tag_unlock );
		module.set_function("abort", plc_tag_abort );
		module.set_function("destroy", plc_tag_destroy );
		module.set_function("read", plc_tag_read );
		module.set_function("status", plc_tag_status );
		module.set_function("write", plc_tag_write );

		module.set_function("get_int_attribute", plc_tag_get_int_attribute );
		module.set_function("set_int_attribute", plc_tag_set_int_attribute );
		module.set_function("get_size", plc_tag_get_size );
		module.set_function("get_bool", plc_tag_get_bool );
		module.set_function("set_bool", plc_tag_set_bool );
		module.set_function("get_bit", plc_tag_get_bit );
		module.set_function("set_bit", plc_tag_set_bit );
		module.set_function("get_uint64", plc_tag_get_uint64 );
		module.set_function("set_uint64", plc_tag_set_uint64 );
		module.set_function("get_int64", plc_tag_get_int64 );
		module.set_function("set_int64", plc_tag_set_int64 );
		module.set_function("get_uint32", plc_tag_get_uint32 );
		module.set_function("set_uint32", plc_tag_set_uint32 );
		module.set_function("get_int32", plc_tag_get_int32 );
		module.set_function("set_int32", plc_tag_set_int32 );
		module.set_function("get_uint16", plc_tag_get_uint16 );
		module.set_function("set_uint16", plc_tag_set_uint16 );
		module.set_function("get_int16", plc_tag_get_int16 );
		module.set_function("set_int16", plc_tag_set_int16 );
		module.set_function("get_uint8", plc_tag_get_uint8 );
		module.set_function("set_uint8", plc_tag_set_uint8 );
		module.set_function("get_int8", plc_tag_get_int8 );
		module.set_function("set_int8", plc_tag_set_int8 );
		module.set_function("get_float64", plc_tag_get_float64 );
		module.set_function("set_float64", plc_tag_set_float64 );
		module.set_function("get_float32", plc_tag_get_float32 );
		module.set_function("set_float32", plc_tag_set_float32 );

		module.set_function("get_string", plc_tag_get_string );
		module.set_function("set_string", plc_tag_set_string );

		//utils
		module.set_function("sleep", sleep_ms);
		module.set_function("time", time_ms);

		// Sol2 will wrap the NULL to nil automatically
		//module.set("null", PLC_TAG_NULL);

		/**
		 * Module version
		 */
		module["VERSION"] = LIB_PLC_TAG_VERSION;

		return module;
	}
}

extern "C" int luaopen_plctag(lua_State *L) {
	return sol::stack::call_lua(L, 1, lua_module::open_module);
}
