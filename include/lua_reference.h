#pragma once
#include "lua_function.h"

namespace luakit {
    //reference
    struct reference {
    public:
        reference(lua_State* L) : m_L(L) {
            m_index = luaL_ref(m_L, LUA_REGISTRYINDEX);
        }
        reference(reference& ref) noexcept {
            m_L = ref.m_L;
            m_index = ref.m_index;
            ref.m_index = LUA_NOREF;
        }
        reference(reference&& ref) noexcept {
            m_L = ref.m_L;
            m_index = ref.m_index;
            ref.m_index = LUA_NOREF;
        }
        ~reference() {
            if (m_index != LUA_REFNIL && m_index != LUA_NOREF) {
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_index);
            }
        }
        void push_stack() const {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_index);
        }

        template <typename sequence_type, typename T>
        sequence_type to_sequence() {
            lua_guard g(m_L);
            sequence_type ret;
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_index);
            if (lua_istable(m_L, -1)) {
                lua_pushnil(m_L);
                while (lua_next(m_L, -2) != 0) {
                    ret.push_back(lua_to_native<T>(m_L, -1));
                    lua_pop(m_L, 1);
                }
            }
            return ret;
        }

        template <typename associate_type, typename T, typename V>
        associate_type to_associate() {
            lua_guard g(m_L);
            sequence_type ret;
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_index);
            if (lua_istable(m_L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    ret.insert(std::make_pair(lua_to_native<K>(L, -2), lua_to_native<V>(L, -1)));
                    lua_pop(L, 1);
                }
            }
            return ret;
        }

    protected:
        lua_State*  m_L = nullptr;
        uint32_t    m_index = LUA_NOREF;
    };

    using variadic_results = std::vector<reference>;

    template <> int native_to_lua(lua_State* L, variadic_results vr) {
        for (auto r : vr) {
            r.push_stack();
        }
        return (int)vr.size();
    }

    template <> int native_to_lua(lua_State* L, reference r) {
        r.push_stack();
        return 1;
    }

    template <> reference lua_to_native(lua_State* L, int i) {
        return reference(L);
    }

    template <typename sequence_type, typename T>
    void lua_new_reference(lua_State* L, sequence_type v) {
        int index = 1;
        lua_newtable(L);
        for (auto item : v) {
            native_to_lua<T>(L, item);
            lua_seti(L, -2, index++);
        }
    }

    template <typename associate_type, typename K, typename V>
    void lua_new_reference(lua_State* L, associate_type v) {
        lua_newtable(L);
        for (auto item : v) {
            native_to_lua<K>(L, item.first);
            native_to_lua<V>(L, item.second);
            lua_settable(L, -3);
        }
    }
}