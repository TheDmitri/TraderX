#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Server-side password validation + cooldown bookkeeping for one attempt.
// Correct code (or admin) unlocks and remembers the player as authorized.

class TXCLAttemptService
{
    static ref TXCLAttemptService m_Instance;

    static TXCLAttemptService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new TXCLAttemptService();
        return m_Instance;
    }

    void EnterPasswordAttempt(TXCLVehicleLockManager manager, PlayerBase player, int password)
    {
        TXCLPlayerDb playerDb = player.GetTXCLPlayerDb();
        if (!playerDb)
            return;

        TXCLCooldownState cooldownState = playerDb.GetCooldownStateById(manager.GetLockId());
        if (cooldownState && cooldownState.HasCooldown())
        {
            GetTXCLLog().LogInfo("[TXCL] Attempt on cooldown: " + player.GetIdentity().GetName() + " lockId=" + manager.GetLockId() + " remaining=" + cooldownState.GetRemainingTime());
            cooldownState.PrepareForClient();
            GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttemptResponse", new Param1<TXCLEnterPasswordResponse>(TXCLEnterPasswordResponse.Create("You are on cooldown", false, cooldownState)), true, player.GetIdentity());
            return;
        }

        bool isPasswordCorrect = (password == manager.GetVehicleLockPassword()) || playerDb.IsAdmin();
        if (!isPasswordCorrect)
        {
            if (!cooldownState)
            {
                playerDb.AddCooldownState(TXCLCooldownState.Create(manager.GetLockId(), GetTXCLModule().GetSettings().initCooldownTime));
                cooldownState = playerDb.GetCooldownStateById(manager.GetLockId());
            }
            else
            {
                cooldownState.AddCooldown();
            }

            cooldownState.PrepareForClient();
            GetTXCLLog().LogInfo("[TXCL] Wrong code: " + player.GetIdentity().GetName() + " lockId=" + manager.GetLockId() + " cooldown=" + cooldownState.GetRemainingTime() + "s");
            GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttemptResponse", new Param1<TXCLEnterPasswordResponse>(TXCLEnterPasswordResponse.Create("Code incorrect", false, cooldownState)), true, player.GetIdentity());
            return;
        }

        if (!playerDb.HasUnlockedVehicle(manager.GetLockId()))
            playerDb.AddUnlockedVehicle(manager.GetLockId());

        manager.SetVehicleLock(false);
        GetTXCLLog().LogInfo("[TXCL] Correct code, unlocked: " + player.GetIdentity().GetName() + " lockId=" + manager.GetLockId());
        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttemptResponse", new Param1<TXCLEnterPasswordResponse>(TXCLEnterPasswordResponse.Create("Code valid", true, null)), true, player.GetIdentity());
        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetPlayerDbResponse", new Param1<TXCLPlayerDb>(player.GetTXCLPlayerDb()), true, player.GetIdentity());
    }
}
#endif
