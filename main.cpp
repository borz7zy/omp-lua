#include <vector>
#include <filesystem>
#include <string>
#include <optional>
#include <variant>
#include <map>

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
    std::optional<std::string> mainscriptFile_;

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

    lua_State *L_ = nullptr;

    std::map<int, IPlayer *> playerMap_;

    using LuaValue = std::variant<int, unsigned int, double, std::string, bool>;

    void pushLuaValue(lua_State *&L, const LuaValue &value)
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

    LuaValue popLuaValue(lua_State *&L, int index)
    {
        if (lua_isinteger(L, index))
        {
            return static_cast<int>(lua_tointeger(L, index));
        }
        else if (lua_isnumber(L, index))
        {
            return lua_tonumber(L, index);
        }
        else if (lua_isstring(L, index))
        {
            return lua_tostring(L, index);
        }
        else if (lua_isboolean(L, index))
        {
            return lua_toboolean(L, index);
        }
        else
        {
            return nullptr; // Or handle error appropriately
        }
    }

    bool callLuaFunction(lua_State *&L, const std::string &funcName, const std::vector<LuaValue> &args, std::vector<LuaValue> &outResults)
    {
        if (!L)
            return false;

        lua_getglobal(L, funcName.c_str());
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return false;
        }

        for (const auto &arg : args)
        {
            pushLuaValue(L, arg);
        }

        if (lua_pcall(L, args.size(), LUA_MULTRET, 0) != LUA_OK)
        {
            const char *errorMsg = lua_tostring(L, -1);
            if (core_ != nullptr)
            {
                core_->printLn("OMP LUA ERROR: %s", (errorMsg ? errorMsg : "Unknown error"));
            }
            lua_pop(L, 1);
            return false;
        }

        int numResults = lua_gettop(L);

        for (int i = numResults; i > 0; --i)
        {
            outResults.push_back(popLuaValue(L, -i));
        }

        lua_settop(L, 0);

        return true;
    }

    template <typename... Args>
    std::vector<LuaValue> callLua(const std::string &funcName, Args &&...args)
    {
        std::vector<LuaValue> arguments{LuaValue(std::forward<Args>(args))...};
        std::vector<LuaValue> results;
        callLuaFunction(L_, funcName, arguments, results);
        return results;
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
        playerMap_[player.getID()] = &player;
        // public OnIncomingConnection(playerid, ip_address[], port)
        callLua("OnIncomingConnection", player.getID(), ipAddress.data(), port);
    }
    void onPlayerConnect(IPlayer &player) override
    {
        // public OnPlayerConnect(playerid)
        callLua("OnPlayerConnect", player.getID());
    }
    void onPlayerDisconnect(IPlayer &player, PeerDisconnectReason reason) override
    {
        // public OnPlayerDisconnect(playerid, reason)
        callLua("OnPlayerDisconnect", player.getID(), int(reason));
        playerMap_.erase(player.getID());
    }
    void onPlayerClientInit(IPlayer &player) override
    {
        // TODO
    }
    bool onPlayerRequestSpawn(IPlayer &player) override
    {
        // public OnPlayerRequestSpawn(playerid)
        auto result = callLua("OnPlayerRequestSpawn", player.getID());
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    void onPlayerSpawn(IPlayer &player) override
    {
        // public OnPlayerSpawn(playerid)
        callLua("OnPlayerSpawn", player.getID());
    }
    void onPlayerStreamIn(IPlayer &player, IPlayer &forPlayer) override
    {
        // public OnPlayerStreamIn(playerid, forplayerid)
        callLua("OnPlayerStreamIn", player.getID());
    }
    void onPlayerStreamOut(IPlayer &player, IPlayer &forPlayer) override
    {
        // public OnPlayerStreamOut(playerid, forplayerid)
        callLua("OnPlayerStreamOut", player.getID());
    }
    bool onPlayerText(IPlayer &player, StringView message) override
    {
        // public OnPlayerText(playerid, text[])
        auto result = callLua("OnPlayerText", player.getID(), message.data());
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    bool onPlayerCommandText(IPlayer &player, StringView message) override
    {
        // public OnPlayerCommandText(playerid, cmdtext[])
        auto result = callLua("OnPlayerCommandText", player.getID(), message.data());
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return false;
    }
    bool onPlayerShotMissed(IPlayer &player, const PlayerBulletData &bulletData) override
    {
        // public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
        auto result = callLua("OnPlayerWeaponShot",
                              player.getID(),
                              bulletData.weapon, int(bulletData.hitType), bulletData.hitID,
                              bulletData.offset.x, bulletData.offset.y, bulletData.offset.z);
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    bool onPlayerShotPlayer(IPlayer &player, IPlayer &target, const PlayerBulletData &bulletData) override
    {
        // public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
        auto result = callLua("OnPlayerWeaponShot",
                              player.getID(),
                              bulletData.weapon, int(bulletData.hitType), bulletData.hitID,
                              bulletData.offset.x, bulletData.offset.y, bulletData.offset.z);
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    bool onPlayerShotVehicle(IPlayer &player, IVehicle &target, const PlayerBulletData &bulletData) override
    {
        // public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
        auto result = callLua("OnPlayerWeaponShot",
                              player.getID(),
                              bulletData.weapon, int(bulletData.hitType), bulletData.hitID,
                              bulletData.offset.x, bulletData.offset.y, bulletData.offset.z);
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    bool onPlayerShotObject(IPlayer &player, IObject &target, const PlayerBulletData &bulletData) override
    {
        // public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
        auto result = callLua("OnPlayerWeaponShot",
                              player.getID(),
                              bulletData.weapon, int(bulletData.hitType), bulletData.hitID,
                              bulletData.offset.x, bulletData.offset.y, bulletData.offset.z);
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    bool onPlayerShotPlayerObject(IPlayer &player, IPlayerObject &target, const PlayerBulletData &bulletData) override
    {
        // public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
        auto result = callLua("OnPlayerWeaponShot",
                              player.getID(),
                              bulletData.weapon, int(bulletData.hitType), bulletData.hitID,
                              bulletData.offset.x, bulletData.offset.y, bulletData.offset.z);
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }
    void onPlayerScoreChange(IPlayer &player, int score) override
    {
        // TODO
    }
    void onPlayerNameChange(IPlayer &player, StringView oldName) override
    {
        // TODO
    }
    void onPlayerInteriorChange(IPlayer &player, unsigned newInterior, unsigned oldInterior) override
    {
        // public OnPlayerInteriorChange(playerid, newinteriorid, oldinteriorid)
        callLua("OnPlayerInteriorChange", player.getID(), newInterior, oldInterior);
    }
    void onPlayerStateChange(IPlayer &player, PlayerState newState, PlayerState oldState) override
    {
        // public OnPlayerStateChange(playerid, PLAYER_STATE:newstate, PLAYER_STATE:oldstate)
        callLua("OnPlayerStateChange", player.getID(), int(newState), int(oldState));
    }
    void onPlayerKeyStateChange(IPlayer &player, uint32_t newKeys, uint32_t oldKeys) override
    {
        // public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
        callLua("OnPlayerKeyStateChange", player.getID(), newKeys, oldKeys);
    }
    void onPlayerDeath(IPlayer &player, IPlayer *killer, int reason) override
    {
        // public OnPlayerDeath(playerid, killerid, WEAPON:reason)
        callLua("OnPlayerDeath", player.getID(), killer ? killer->getID() : INVALID_PLAYER_ID, reason);
    }
    void onPlayerTakeDamage(IPlayer &player, IPlayer *from, float amount, unsigned weapon, BodyPart part) override
    {
        // public OnPlayerTakeDamage(playerid, issuerid, Float:amount, WEAPON:weaponid, bodypart)
        callLua("OnPlayerTakeDamage", player.getID(), from ? from->getID() : INVALID_PLAYER_ID, amount, weapon, int(part));
    }
    void onPlayerGiveDamage(IPlayer &player, IPlayer &to, float amount, unsigned weapon, BodyPart part) override
    {
        // public OnPlayerGiveDamage(playerid, damagedid, Float:amount, WEAPON:weaponid, bodypart)
        callLua("OnPlayerGiveDamage", player.getID(), to.getID(), amount, weapon, int(part));
    }
    void onPlayerClickMap(IPlayer &player, Vector3 pos) override
    {
        // public OnPlayerClickMap(playerid, Float:fX, Float:fY, Float:fZ)
        callLua("OnPlayerClickMap", player.getID(), pos.x, pos.y, pos.z);
    }
    void onPlayerClickPlayer(IPlayer &player, IPlayer &clicked, PlayerClickSource source) override
    {
        // public OnPlayerClickPlayer(playerid, clickedplayerid, CLICK_SOURCE:source)
        callLua("OnPlayerClickPlayer", player.getID(), clicked.getID(), int(source));
    }
    void onClientCheckResponse(IPlayer &player, int actionType, int address, int results) override
    {
        // public OnClientCheckResponse(playerid, actionid, memaddr, retndata)
        callLua("OnClientCheckResponse", player.getID(), actionType, address, results);
    }
    bool onPlayerUpdate(IPlayer &player, TimePoint now) override
    {
        // public OnPlayerUpdate(playerid)
        auto result = callLua("OnPlayerUpdate", player.getID());
        if (!result.empty())
        {
            const auto &value = result[0];
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            else if (std::holds_alternative<int>(value))
            {
                int intResult = std::get<int>(value);
                return static_cast<bool>(intResult);
            }
        }
        return true;
    }

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

        core_->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerSpawnDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerChangeDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerDamageDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerUpdateDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerStreamDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerTextDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerShotDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerClickDispatcher().addEventHandler(this);
        core_->getPlayers().getPlayerCheckDispatcher().addEventHandler(this);

        L_ = (L_ == nullptr) ? luaL_newstate() : L_;
        if (L_ == nullptr)
        {
            core_->printLn("OMP LUA: Lua state for main script load error!");
        }
        luaL_openlibs(L_);

        mainscriptFile_ = scanMainscripts("./mainscripts");

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
