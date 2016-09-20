/*
 * widgets/webview/javascript.c - webkit webview javascript functions
 *
 * Copyright © 2010-2012 Mason Larobina <mason.larobina@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <JavaScriptCore/JavaScript.h>
#include <stdlib.h>

#include "common/luajs.h"
#include "common/msg.h"
#include "common/luaserialize.h"

void
run_javascript_finished(const guint8 *msg, guint length)
{
    lua_State *L = globalconf.L;
    gint top = lua_gettop(L);
    gint n = lua_deserialize_range(L, msg, length);
    g_assert_cmpint(n, >=, 2);
    g_assert_cmpint(n, <=, 4);

    gpointer cb = lua_touserdata(L, -n + 1);

    if (n == 4) { /* Nil return value and Error */
        g_assert(lua_isnil(L, -2));
        g_assert(lua_isstring(L, -1));
    }

    if (n >= 3 && cb) {
        luaH_object_push(L, cb);
        luaH_dofunction(L, n-2, 0);
        luaH_object_unref(L, cb);
    }

    lua_settop(L, top);
}

static gint
luaH_webview_eval_js(lua_State *L)
{
    gpointer cb = NULL;
    webview_data_t *d = luaH_checkwvdata(L, 1);
    const gchar *script = luaL_checkstring(L, 2);
    const gchar *usr_source = NULL;
    gchar *source = NULL;
    bool no_return = false;

    gint top = lua_gettop(L);
        luaH_checktable(L, 3);

        /* source filename to use in error messages and webinspector */
        if (luaH_rawfield(L, 3, "source") && lua_isstring(L, -1))
            usr_source = lua_tostring(L, -1);

        if (luaH_rawfield(L, 3, "no_return"))
            no_return = lua_toboolean(L, -1);

        if (luaH_rawfield(L, 3, "callback")) {
            luaH_checkfunction(L, -1);
            cb = luaH_object_ref(L, -1);
        }

        lua_settop(L, top);

    if (!usr_source)
        source = luaH_callerinfo(L);

    lua_pushboolean(L, no_return);
    lua_pushinteger(L, webkit_web_view_get_page_id(d->view));
    lua_pushstring(L, script);
    lua_pushstring(L, usr_source ? g_strdup(usr_source) : source);
    lua_pushlightuserdata(L, cb);
    msg_send_lua(d->ipc, MSG_TYPE_eval_js, L, -5, -1);
    lua_pop(L, 5);

    return FALSE;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
