/*=========================================================================*\
* Unix domain socket
* LuaSocket toolkit
\*=========================================================================*/
#include "lua.h"
#include "lauxlib.h"

#include "unixtcp.h"
#include "unixudp.h"

/*-------------------------------------------------------------------------*\
* Modules and functions
\*-------------------------------------------------------------------------*/
static const luaL_Reg mod[] = {
    {"tcp", unixtcp_open},
    {"udp", unixudp_open},
    {NULL, NULL}
};

static void add_alias(lua_State *L, int index, const char *name, const char *target)
{
    lua_getfield(L, index, target);
    lua_setfield(L, index, name);
}

static int compat_socket_unix_call(lua_State *L)
{
    /* Look up socket.unix.tcp in the socket.unix table (which is the first
     * argument). */
    lua_getfield(L, 1, "tcp");

    /* Replace the stack entry for the socket.unix table with the
     * socket.unix.tcp function. */
    lua_replace(L, 1);

    /* Call socket.unix.tcp, passing along any arguments. */
    int n = lua_gettop(L);
    lua_call(L, n-1, LUA_MULTRET);

    /* Pass along the return values from socket.unix.tcp. */
    n = lua_gettop(L);
    return n;
}

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int luaopen_socket_unix(lua_State *L)
{
    int i;
    lua_newtable(L);
    int socket_unix_table = lua_gettop(L);

    for (i = 0; mod[i].name; i++)
        mod[i].func(L);

    /* Add forward compatibility aliases "stream" and "dgram" for the "tcp" and
     * "udp" functions. */
    add_alias(L, socket_unix_table, "stream", "tcp");
    add_alias(L, socket_unix_table, "dgram", "udp");

    /* Add a backwards compatibility function and a metatable setup to call it
     * for the old socket.unix() interface. */
    lua_pushcfunction(L, compat_socket_unix_call);
    lua_setfield(L, socket_unix_table, "__call");
    lua_pushvalue(L, socket_unix_table);
    lua_setmetatable(L, socket_unix_table);

    return 1;
}
