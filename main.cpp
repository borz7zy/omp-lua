#include <vector>
#include <filesystem>
#include <string>
#include <optional>
#include <variant>

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "luaconf.h"
#include "lualib.h"
}

// Required for most of open.mp.
#include <sdk.hpp>

// Include the vehicle component information.
#include <Server/Components/Vehicles/vehicles.hpp>

struct LuaStateInfo
{
    lua_State *L;
    const char *script;
};

// This should use an abstract interface if it is to be passed to other components.  Like the files
// in `<Server/Components/>` you would share only this base class and keep the implementation
// private.
class OmpLua final : public IComponent,
                     public PlayerConnectEventHandler,
                     public PlayerSpawnEventHandler,
                     public PlayerStreamEventHandler,
                     public PlayerTextEventHandler,
                     public PlayerShotEventHandler,
                     public PlayerChangeEventHandler,
                     public PlayerDamageEventHandler,
                     public PlayerClickEventHandler,
                     public PlayerCheckEventHandler,
                     public PlayerUpdateEventHandler
{
private:
    std::vector<std::string> sideFiles_;
    std::optional<std::string> mainscriptFile_;

    void scanSidescripts(const std::filesystem::path &directory)
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            return;
        }
        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                sideFiles_.emplace_back(entry.path().string());
            }
        }
    }

    std::optional<std::string> scanMainscripts(const std::filesystem::path &directory)
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            return std::nullopt;
        }
        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                return entry.path().string();
            }
        }
        return std::nullopt;
    }

    // Hold a reference to the main server core.
    ICore *core_ = nullptr;

    std::vector<LuaStateInfo> sideScripts_;
    lua_State *L_ = nullptr;

    using LuaValue = std::variant<int, double, std::string, bool>;

    void pushLuaValue(lua_State *L, const LuaValue &value)
    {
        std::visit([L](auto &&arg)
                   {
                   using T = std::decay_t<decltype(arg)>;
                   if constexpr (std::is_same_v<T, int>)
                   {
                       lua_pushinteger(L, arg);
                   }
                   else if constexpr (std::is_same_v<T, double>)
                   {
                       lua_pushnumber(L, arg);
                   }
                   else if constexpr (std::is_same_v<T, std::string>)
                   {
                       lua_pushstring(L, arg.c_str());
                   }
                   else if constexpr (std::is_same_v<T, bool>)
                   {
                       lua_pushboolean(L, arg);
                   } },
                   value);
    }
    bool callLuaFunction(lua_State *L, const std::string &funcName, const std::vector<LuaValue> &args)
    {
        if (!L)
            return false;

        lua_getglobal(L, funcName.c_str());
        if (lua_isfunction(L, -1))
        {
            for (const auto &arg : args)
            {
                pushLuaValue(L, arg);
            }

            if (lua_pcall(L, args.size(), 0, 0) != LUA_OK)
            {
                const char *errorMsg = lua_tostring(L, -1);
                if (core_ != nullptr)
                {
                    core_->printLn("OMP LUA ERROR: %s", (errorMsg ? errorMsg : "Unknown error"));
                }
                lua_pop(L, 1);
                return false;
            }
        }

        return false;
    }

    template <typename... Args>
    void callLua(const std::string &funcName, Args &&...args)
    {
        std::vector<LuaValue> arguments = {std::forward<Args>(args)...};

        for (const auto &script : sideScripts_)
        {
            if (!script.L)
                continue;

            callLuaFunction(script.L, funcName, arguments);
        }
        callLuaFunction(L_, funcName, arguments);
    }

    int native_printOMP(lua_State *L)
    {
        if (!L)
            return 0;

        int nargs = lua_gettop(L);
        std::string output;

        for (int i = 1; i <= nargs; ++i)
        {
            if (lua_isstring(L, i))
            {
                output += lua_tostring(L, i);
            }
            else if (lua_isnumber(L, i))
            {
                output += std::to_string(lua_tonumber(L, i));
            }
            else if (lua_isboolean(L, i))
            {
                output += lua_toboolean(L, i) ? "true" : "false";
            }
            else if (lua_isnil(L, i))
            {
                output += "nil";
            }
            else
            {
                output += "[unknown]";
            }

            if (i < nargs)
                output += " ";
        }

        if (core_ != nullptr)
        {
            core_->printLn("OMP LUA: %s", output.c_str());
        }

        return 1;
    }

public:
    // Visit https://open.mp/uid to generate a new unique ID.
    PROVIDE_UID(0x46EEFEA7E0B81CAE);

    // When this component is destroyed we need to tell any linked components this it is gone.
    ~OmpLua()
    {
        if (L_ != nullptr)
        {
            lua_close(L_);
            L_ = nullptr;
        }
    }

    void onIncomingConnection(IPlayer &player, StringView ipAddress, unsigned short port) override
    {
        // public OnIncomingConnection(playerid, ip_address[], port)
        callLua("OnIncomingConnection", player.getID(), ipAddress.data(), port);
    }
    void onPlayerConnect(IPlayer &player) override
    {
        // public OnPlayerConnect(playerid)
        callLua("OnPlayerConnect", player.getID());
    }
    void onPlayerDisconnect(IPlayer &player, PeerDisconnectReason reason) override {}
    void onPlayerClientInit(IPlayer &player) override {}
    bool onPlayerRequestSpawn(IPlayer &player) override { return true; }
    void onPlayerSpawn(IPlayer &player) override {}
    void onPlayerStreamIn(IPlayer &player, IPlayer &forPlayer) override {}
    void onPlayerStreamOut(IPlayer &player, IPlayer &forPlayer) override {}
    bool onPlayerText(IPlayer &player, StringView message) override { return true; }
    bool onPlayerCommandText(IPlayer &player, StringView message) override { return false; }
    bool onPlayerShotMissed(IPlayer &player, const PlayerBulletData &bulletData) override { return true; }
    bool onPlayerShotPlayer(IPlayer &player, IPlayer &target, const PlayerBulletData &bulletData) override { return true; }
    bool onPlayerShotVehicle(IPlayer &player, IVehicle &target, const PlayerBulletData &bulletData) override { return true; }
    bool onPlayerShotObject(IPlayer &player, IObject &target, const PlayerBulletData &bulletData) override { return true; }
    bool onPlayerShotPlayerObject(IPlayer &player, IPlayerObject &target, const PlayerBulletData &bulletData) override { return true; }
    void onPlayerScoreChange(IPlayer &player, int score) override {}
    void onPlayerNameChange(IPlayer &player, StringView oldName) override {}
    void onPlayerInteriorChange(IPlayer &player, unsigned newInterior, unsigned oldInterior) override {}
    void onPlayerStateChange(IPlayer &player, PlayerState newState, PlayerState oldState) override {}
    void onPlayerKeyStateChange(IPlayer &player, uint32_t newKeys, uint32_t oldKeys) override {}
    void onPlayerDeath(IPlayer &player, IPlayer *killer, int reason) override {}
    void onPlayerTakeDamage(IPlayer &player, IPlayer *from, float amount, unsigned weapon, BodyPart part) override {}
    void onPlayerGiveDamage(IPlayer &player, IPlayer &to, float amount, unsigned weapon, BodyPart part) override {}
    void onPlayerClickMap(IPlayer &player, Vector3 pos) override {}
    void onPlayerClickPlayer(IPlayer &player, IPlayer &clicked, PlayerClickSource source) override {}
    void onClientCheckResponse(IPlayer &player, int actionType, int address, int results) override {}
    bool onPlayerUpdate(IPlayer &player, TimePoint now) override { return true; }

    // Implement the main component API.
    StringView componentName() const override
    {
        return "OmpLua";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(1, 0, 0, 0);
    }

    void onLoad(ICore *c) override
    {
        // Cache core, player pool here
        core_ = c;

        L_ = (L_ == nullptr) ? luaL_newstate() : L_;
        if (L_ == nullptr)
        {
            core_->printLn("OMP LUA: Lua state for main script load error!");
        }
        luaL_openlibs(L_);

        scanSidescripts("./sidescripts");
        mainscriptFile_ = scanMainscripts("./mainscripts");

        for (const auto &file : sideFiles_)
        {
            lua_State *SC = luaL_newstate();
            if (SC == nullptr)
            {
                core_->printLn("OMP LUA: Lua state for side script load error!");
            }
            luaL_openlibs(SC);

            if (luaL_dofile(SC, file.c_str()) != LUA_OK)
            {
                const char *errorMsg = lua_tostring(SC, -1);
                core_->printLn("%s", errorMsg ? errorMsg : "OMP LUA: Unknown Lua error");
                lua_pop(SC, 1);
            }

            lua_pushlightuserdata(SC, this);
            lua_pushcclosure(SC, [](lua_State *L) -> int
                             {
            OmpLua *self = static_cast<OmpLua*>(lua_touserdata(L, lua_upvalueindex(1)));
            return self->native_printOMP(L); }, 1);
            lua_setglobal(SC, "printOMP");

            LuaStateInfo sclsi;
            sclsi.L = SC;
            sclsi.script = file.c_str();
            sideScripts_.push_back(sclsi);
        }
        if (mainscriptFile_.has_value())
        {
            if (luaL_dofile(L_, mainscriptFile_->c_str()) != LUA_OK)
            {
                const char *errorMsg = lua_tostring(L_, -1);
                core_->printLn("%s", errorMsg ? errorMsg : "OMP LUA: Unknown Lua error");
                lua_pop(L_, 1);
            }
            lua_pushlightuserdata(L_, this);
            lua_pushcclosure(L_, [](lua_State *L) -> int
                             {
            OmpLua *self = static_cast<OmpLua*>(lua_touserdata(L, lua_upvalueindex(1)));
            return self->native_printOMP(L); }, 1);
            lua_setglobal(L_, "printOMP");
        }
        else
        {
            core_->printLn("OMP LUA: mainscript not found!");
        }
        core_->printLn("OMP LUA loaded.");
    }

    void onInit(IComponentList *components) override
    {
        // Cache components, add event handlers here.
    }

    void onReady() override
    {
        // Fire events here at earliest.
    }

    void onFree(IComponent *component) override
    {
    }

    void free() override
    {
        // Deletes the component.
        delete this;
    }

    void reset() override
    {
        // Resets data when the mode changes.
    }
};

// Automatically called when the compiled binary is loaded.
COMPONENT_ENTRY_POINT()
{
    return new OmpLua();
}