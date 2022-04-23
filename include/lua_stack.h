#pragma once
#include "lua_base.h"

namespace luakit {

    template <typename T>
    T lua_to_object(lua_State* L, int idx);
    template <typename T>
    void lua_push_object(lua_State* L, T obj);

    //将lua栈顶元素转换成C++对象
    template <typename T>
    T lua_to_native(lua_State* L, int i) {
        if constexpr (std::is_same_v<T, bool>) {
            return lua_toboolean(L, i) != 0;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            const char* str = lua_tostring(L, i);
            return str == nullptr ? "" : str;
        }
        else if constexpr (std::is_integral_v<T>) {
            return (T)lua_tointeger(L, i);
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return (T)lua_tonumber(L, i);
        }
        else if constexpr (std::is_pointer_v<T>) {
            using type = std::remove_volatile_t<std::remove_pointer_t<T>>;
            if constexpr (std::is_same_v<type, const char>) {
                return lua_tostring(L, i);
            }
            else {
                return lua_to_object<T>(L, i);
            }
        }
    }

    //C++对象压到lua堆顶
    template <typename T>
    int native_to_lua(lua_State* L, T v) {
        if constexpr (std::is_same_v<T, bool>) {
            lua_pushboolean(L, v);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            lua_pushstring(L, v.c_str());
        }
        else if constexpr (std::is_integral_v<T>) {
            lua_pushinteger(L, (lua_Integer)v);
        }
        else if constexpr (std::is_floating_point_v<T>) {
            lua_pushnumber(L, v);
        }
        else if constexpr (std::is_enum<T>::value) {
            lua_pushinteger(L, (lua_Integer)v);
        }
        else if constexpr (std::is_pointer_v<T>) {
            using type = std::remove_cv_t<std::remove_pointer_t<T>>;
            if constexpr (std::is_same_v<type, char>) {
                if (v != nullptr) {
                    lua_pushstring(L, v);
                }
                else {
                    lua_pushnil(L);
                }
            }
            else {
                lua_push_object(L, v);
            }
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }

    int lua_normal_index(lua_State* L, int idx) {
        int top = lua_gettop(L);
        if (idx < 0 && -idx <= top)
            return idx + top + 1;
        return idx;
    }

    template <typename T>
    void lua_push_object(lua_State* L, T obj) {
        if (obj == nullptr) {
            lua_pushnil(L);
            return;
        }

        lua_getfield(L, LUA_REGISTRYINDEX, "__objects__");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);

            lua_newtable(L);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);

            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, "__objects__");
        }

        // stack: __objects__
        const char* pkey = lua_get_object_key<T>(obj);
        if (lua_getfield(L, -1, pkey) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushstring(L, "__pointer__");
            lua_pushlightuserdata(L, obj);
            lua_rawset(L, -3);

            // stack: __objects__, tab
            const char* meta_name = lua_get_meta_name<T>();
            luaL_getmetatable(L, meta_name);
            lua_setmetatable(L, -2);

            // stack: __objects__, tab
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, pkey);
        }
        lua_remove(L, -2);
    }

    template <typename T>
    void lua_detach_object(lua_State* L, T obj) {
        if (obj == nullptr)
            return;

        lua_getfield(L, LUA_REGISTRYINDEX, "__objects__");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return;
        }

        // stack: __objects__
        const char* pkey = lua_get_object_key<T>(obj);
        if (lua_getfield(L, -1, pkey) != LUA_TTABLE) {
            lua_pop(L, 2);
            return;
        }

        // stack: __objects__, __shadow_object__
        lua_pushstring(L, "__pointer__");
        lua_pushnil(L);
        lua_rawset(L, -3);

        lua_pushnil(L);
        lua_rawsetp(L, -3, obj);
        lua_pop(L, 2);
    }

    template <typename T>
    T lua_to_object(lua_State* L, int idx) {
        T obj = nullptr; 
        idx = lua_normal_index(L, idx);
        if (lua_istable(L, idx)) {
            lua_pushstring(L, "__pointer__");
            lua_rawget(L, idx);
            obj = (T)lua_touserdata(L, -1);
            lua_pop(L, 1);
        }
        return obj;
    }

    template<typename... arg_types>
    void native_to_lua_mutil(lua_State* L, arg_types&&... args) {
        int _[] = { 0, (native_to_lua(L, args), 0)... };
    }

    template<size_t... integers, typename... var_types>
    void lua_to_native_mutil(lua_State* L, std::tuple<var_types&...>& vars, std::index_sequence<integers...>&&) {
        int _[] = { 0, (std::get<integers>(vars) = lua_to_native<var_types>(L, (int)integers - (int)sizeof...(integers)), 0)... };
    }
}