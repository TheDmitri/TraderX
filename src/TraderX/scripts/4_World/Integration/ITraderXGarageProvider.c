/**
 * Port: garage provider. Server-side, in-process calls.
 * Vanilla types + core DTO (TraderXGarageVehicleInfo) only.
 */
class ITraderXGarageProvider
{
    // Server. Snapshots state (health/fuel/cargo/attachments) then removes the entity from the world.
    bool AddVehicle(string playerUid, EntityAI vehicle) { return false; }
    // Creates a fresh vehicle of this type and stores it directly (e.g. bought-but-unspawned vehicle).
    bool AddVehicleByType(string playerUid, string className) { return false; }
    // Removes from garage without respawn (sold/destroyed).
    bool RemoveVehicle(string playerUid, string vehicleId) { return false; }
    // Respawns the stored vehicle at pos/orientation, restores state. Returns the entity (or null).
    EntityAI RetrieveVehicle(string playerUid, string vehicleId, vector pos, vector orientation) { return null; }
    int  GetStoredCount(string playerUid) { return 0; }
    int  GetMaxSlots() { return 0; }
    bool HasFreeSlot(string playerUid) { return false; }
    array<ref TraderXGarageVehicleInfo> ListVehicles(string playerUid) { return null; }
    bool IsResaleBlocked(EntityAI vehicle) { return false; }
    void OnVehicleSold(EntityAI vehicle, string sellerUid) { }
}
