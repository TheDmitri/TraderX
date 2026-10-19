#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// TraderXCarLock implementation of the core ITraderXCarLockProvider port.
// Thin façade over TXCLVehicleLockManager. Server-side, in-process. 8-digit passwords.
// Registered by TXCLModule when TraderX is present. Only compiled with TraderX (needs the port).
//
// Target use: a trader mod buys a vehicle -> AddCarLockWithPassword(veh, buyerUid, code) so the
// vehicle spawns locked under the buyer with an 8-digit code.

#ifdef TraderX
class TXCLLockProvider : ITraderXCarLockProvider
{
    override bool AddCarLock(EntityAI vehicle, string ownerUid)
    {
        if (!GetGame().IsServer())
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;

        manager.InitializeVehicleLockData(TXCLPlayerDb.SteamIdToOwnerId(ownerUid), TXCL_UNDEFINED);
        return true;
    }

    override bool AddCarLockWithPassword(EntityAI vehicle, string ownerUid, string password)
    {
        if (!GetGame().IsServer())
            return false;

        if (!IsValidPassword(password))
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;

        manager.InitializeVehicleLockData(TXCLPlayerDb.SteamIdToOwnerId(ownerUid), password.ToInt());
        return true;
    }

    override bool RemoveCarLock(EntityAI vehicle)
    {
        if (!GetGame().IsServer())
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager || !manager.HasVehicleLock())
            return false;

        manager.ResetVehicleLockData();
        return true;
    }

    override bool IsLocked(EntityAI vehicle)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;
        return manager.IsVehicleLocked();
    }

    override bool HasCarLock(EntityAI vehicle)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;
        return manager.HasVehicleLock();
    }

    override bool IsOwner(EntityAI vehicle, string playerUid)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager || !manager.HasVehicleLock())
            return true;
        int expectedOwner = TXCLPlayerDb.SteamIdToOwnerId(playerUid);
        if (expectedOwner == TXCL_UNDEFINED)
            return false;
        return manager.GetVehicleLockOwner() == expectedOwner;
    }

    override int GetPassword(EntityAI vehicle)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager || !manager.HasVehicleLock())
            return TXCL_UNDEFINED;
        return manager.GetVehicleLockPassword();
    }

    override bool RestoreCarLock(EntityAI vehicle, string ownerUid, int password, bool isLocked)
    {
        if (!GetGame().IsServer())
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;

        int ownerId = TXCLPlayerDb.SteamIdToOwnerId(ownerUid);
        if (ownerId == TXCL_UNDEFINED)
            return false;

        manager.RestoreVehicleLockData(ownerId, password, isLocked, CF_Date.Now().GetTimestamp());
        return true;
    }

    override bool SetOwner(EntityAI vehicle, string ownerUid)
    {
        if (!GetGame().IsServer())
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager)
            return false;

        manager.SetVehicleLockOwner(TXCLPlayerDb.SteamIdToOwnerId(ownerUid));
        return true;
    }

    override string GetOwner(EntityAI vehicle)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(vehicle);
        if (!manager || !manager.HasVehicleLock())
            return "";
        return manager.GetVehicleLockOwner().ToString();
    }

    override int GetPasswordLength()
    {
        return 8;
    }

    // 8 characters, all digits.
    bool IsValidPassword(string password)
    {
        if (password.Length() != 8)
            return false;

        for (int i = 0; i < password.Length(); i++)
        {
            switch (password[i])
            {
                case "0": continue;
                break;
                case "1": continue;
                break;
                case "2": continue;
                break;
                case "3": continue;
                break;
                case "4": continue;
                break;
                case "5": continue;
                break;
                case "6": continue;
                break;
                case "7": continue;
                break;
                case "8": continue;
                break;
                case "9": continue;
                break;
                default: return false;
                break;
            }
        }

        return true;
    }
}
#endif // TraderX
#endif
