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
    // Hold a reference to the main server core.
    ICore *core_ = nullptr;

public:
    // Visit https://open.mp/uid to generate a new unique ID.
    PROVIDE_UID(0x46EEFEA7E0B81CAE);

    // When this component is destroyed we need to tell any linked components this it is gone.
    ~OmpLua()
    {
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