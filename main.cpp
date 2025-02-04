#include <vector>
#include <filesystem>
#include <string>
#include <optional>

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
                sideFiles_.push_back(entry.path().string());
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

    void onIncomingConnection(IPlayer &player, StringView ipAddress, unsigned short port) override {}
    void onPlayerConnect(IPlayer &player) override {}
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
            core_->printLn("Lua state for main script load error!");
        }
        luaL_openlibs(L_);

        scanSidescripts("./sidescripts");
        mainscriptFile_ = scanMainscripts("./mainscripts");

        for (const auto &file : sideFiles_)
        {
            lua_State *SC = luaL_newstate();
            if (SC == nullptr)
            {
                core_->printLn("Lua state for side script load error!");
            }
            luaL_openlibs(SC);

            if (luaL_dofile(SC, file.c_str()) != LUA_OK)
            {
                const char *errorMsg = lua_tostring(SC, -1);
                core_->printLn("%s", errorMsg ? errorMsg : "Unknown Lua error");
                lua_pop(SC, 1);
            }
            // sideScripts_.push_back(SC);
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