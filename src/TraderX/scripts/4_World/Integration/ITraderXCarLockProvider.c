/**
 * Port: carlock provider. Server-side, in-process calls.
 * Vanilla types only.
 */
class ITraderXCarLockProvider
{
    // Server. Adds a carlock, owner = ownerUid, no password.
    bool AddCarLock(EntityAI vehicle, string ownerUid) { return false; }
    // Adds + 8-digit password (string of 8 digits). False if length/format invalid.
    bool AddCarLockWithPassword(EntityAI vehicle, string ownerUid, string password) { return false; }
    bool RemoveCarLock(EntityAI vehicle) { return false; }
    bool IsLocked(EntityAI vehicle) { return false; }
    bool HasCarLock(EntityAI vehicle) { return false; }
    bool IsOwner(EntityAI vehicle, string playerUid) { return false; }
    int  GetPassword(EntityAI vehicle) { return -1; }
    bool RestoreCarLock(EntityAI vehicle, string ownerUid, int password, bool isLocked) { return false; }
    bool SetOwner(EntityAI vehicle, string ownerUid) { return false; }
    string GetOwner(EntityAI vehicle) { return ""; }
    int  GetPasswordLength() { return 8; }
}
