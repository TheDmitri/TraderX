/**
 * Abstract car lock provider interface for TraderX.
 * External mods (e.g., TraderPlusCarLock) extend this class and register
 * via TraderXProviderRegistry to enable lock-aware vehicle trading.
 *
 * Used by:
 * - TraderXVehicleFactory: clear locks on purchased vehicles
 * - TraderXGarage: preserve/restore lock state when parking vehicles
 * - TraderXVehicleTransactionService: check lock state before selling
 */
class ITraderXCarLockProvider
{
    // --- Lock State Queries ---

    /**
     * Whether the vehicle has a lock attached (regardless of locked/unlocked state).
     */
    bool HasLock(EntityAI vehicle)
    {
        return false;
    }

    /**
     * Whether the vehicle is currently locked.
     */
    bool IsLocked(EntityAI vehicle)
    {
        return false;
    }

    /**
     * Get the lock owner's ID for the vehicle.
     * Returns -1 if no lock or no owner.
     */
    int GetLockOwner(EntityAI vehicle)
    {
        return -1;
    }

    /**
     * Get the lock password for the vehicle.
     * Returns -1 if no lock or no password set.
     */
    int GetLockPassword(EntityAI vehicle)
    {
        return -1;
    }

    // --- Lock Control (server-side only) ---

    /**
     * Lock the vehicle.
     */
    void Lock(EntityAI vehicle)
    {
    }

    /**
     * Unlock the vehicle.
     */
    void Unlock(EntityAI vehicle)
    {
    }

    /**
     * Attach a lock to a vehicle with owner and password.
     * Used when restoring a garaged vehicle's lock state.
     */
    void AttachLock(EntityAI vehicle, int ownerUID, int password)
    {
    }

    /**
     * Remove the lock from a vehicle entirely.
     * Used when a vehicle is purchased from the trader (clean state).
     */
    void RemoveLock(EntityAI vehicle)
    {
    }

    // --- Serialization (for garage/receipt) ---

    /**
     * Serialize the vehicle's lock data into a simple array for storage.
     * Format: [hasLock(0/1), isLocked(0/1), ownerUID, password]
     * Returns null if no lock data to save.
     */
    array<int> SerializeLockData(EntityAI vehicle)
    {
        return null;
    }

    /**
     * Restore lock data from a serialized array.
     * @param vehicle The vehicle to restore lock data to
     * @param data Array in format [hasLock, isLocked, ownerUID, password]
     */
    void DeserializeLockData(EntityAI vehicle, array<int> data)
    {
    }

    // --- Permission Checks ---

    /**
     * Whether the given player is a car lock admin (bypasses all locks).
     */
    bool IsAdmin(PlayerBase player)
    {
        return false;
    }
}
