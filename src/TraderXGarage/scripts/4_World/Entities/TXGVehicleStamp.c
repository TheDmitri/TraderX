// TraderXGarage - CF ModStorage Vehicle Stamp & Destruction/Deletion Hooks
// Implements docs/handoff/INSURANCE_SPEC.md §4.2 & §6.
// Persists policyId, ownerUid, stampState, noResaleUntil via CF ModStorage (never raw OnStoreSave).

modded class CarScript
{
    string m_TXG_PolicyId = "";
    string m_TXG_OwnerUid = "";
    int    m_TXG_StampState = 0;
    int    m_TXG_NoResaleUntil = 0;
    string m_TXG_SharedWith = "";   // comma-separated Steam64 ids (storage v2)
    bool   m_TXG_GarageRemoval = false;
    bool   m_TXG_KilledHandled = false;

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return;

        ctx.Write(m_TXG_PolicyId);
        ctx.Write(m_TXG_OwnerUid);
        ctx.Write(m_TXG_StampState);
        ctx.Write(m_TXG_NoResaleUntil);
        ctx.Write(m_TXG_SharedWith);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return true;

        string polId = "";
        string ownUid = "";
        int stState = 0;
        int noResale = 0;
        if (ctx.Read(polId) && ctx.Read(ownUid) && ctx.Read(stState) && ctx.Read(noResale))
        {
            m_TXG_PolicyId = polId;
            m_TXG_OwnerUid = ownUid;
            m_TXG_StampState = stState;
            m_TXG_NoResaleUntil = noResale;
        }
        string shared = "";
        if (ctx.GetVersion() >= 2 && ctx.Read(shared))
            m_TXG_SharedWith = shared;
        // Never fail the vehicle load because of the stamp.
        return true;
    }

    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);
        if (!GetGame().IsServer() || m_TXG_KilledHandled)
            return;
        m_TXG_KilledHandled = true;
        TXGInsuranceService ins = GetTXGInsuranceService();
        if (ins)
            ins.OnVehicleDestroyed(this);
    }

    override void EEDelete(EntityAI parent)
    {
        if (GetGame() && GetGame().IsServer())
        {
            if (m_TXG_StampState == TXGConstants.STAMP_INSURED && !m_TXG_GarageRemoval && !m_TXG_KilledHandled && !TXGVehicleStampHelper.s_ServerShuttingDown)
            {
                TXGInsuranceService ins = GetTXGInsuranceService();
                if (ins)
                    ins.OnVehicleDeletedExternally(this);
            }
        }
        super.EEDelete(parent);
    }
}

modded class BoatScript
{
    string m_TXG_PolicyId = "";
    string m_TXG_OwnerUid = "";
    int    m_TXG_StampState = 0;
    int    m_TXG_NoResaleUntil = 0;
    string m_TXG_SharedWith = "";   // comma-separated Steam64 ids (storage v2)
    bool   m_TXG_GarageRemoval = false;
    bool   m_TXG_KilledHandled = false;

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return;

        ctx.Write(m_TXG_PolicyId);
        ctx.Write(m_TXG_OwnerUid);
        ctx.Write(m_TXG_StampState);
        ctx.Write(m_TXG_NoResaleUntil);
        ctx.Write(m_TXG_SharedWith);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return true;

        string polId = "";
        string ownUid = "";
        int stState = 0;
        int noResale = 0;
        if (ctx.Read(polId) && ctx.Read(ownUid) && ctx.Read(stState) && ctx.Read(noResale))
        {
            m_TXG_PolicyId = polId;
            m_TXG_OwnerUid = ownUid;
            m_TXG_StampState = stState;
            m_TXG_NoResaleUntil = noResale;
        }
        string shared = "";
        if (ctx.GetVersion() >= 2 && ctx.Read(shared))
            m_TXG_SharedWith = shared;
        return true;
    }

    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);
        if (!GetGame().IsServer() || m_TXG_KilledHandled)
            return;
        m_TXG_KilledHandled = true;
        TXGInsuranceService ins = GetTXGInsuranceService();
        if (ins)
            ins.OnVehicleDestroyed(this);
    }

    override void EEDelete(EntityAI parent)
    {
        if (GetGame() && GetGame().IsServer())
        {
            if (m_TXG_StampState == TXGConstants.STAMP_INSURED && !m_TXG_GarageRemoval && !m_TXG_KilledHandled && !TXGVehicleStampHelper.s_ServerShuttingDown)
            {
                TXGInsuranceService ins = GetTXGInsuranceService();
                if (ins)
                    ins.OnVehicleDeletedExternally(this);
            }
        }
        super.EEDelete(parent);
    }
}

class TXGVehicleStampHelper
{
    // Set when the mission ends: the engine then deletes every entity, which must not be read as
    // "insured vehicle deleted by another mod" (that would void every policy at each restart).
    static bool s_ServerShuttingDown = false;

    static string GetSharedWithRaw(EntityAI vehicle)
    {
        CarScript car = CarScript.Cast(vehicle);
        if (car)
            return car.m_TXG_SharedWith;
        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
            return boat.m_TXG_SharedWith;
        return "";
    }

    static void GetSharedWith(EntityAI vehicle, array<string> outUids)
    {
        if (!outUids)
            return;
        string raw = GetSharedWithRaw(vehicle);
        if (raw == "")
            return;
        TStringArray parts = new TStringArray();
        raw.Split(",", parts);
        foreach (string uid : parts)
        {
            if (uid != "" && outUids.Find(uid) == -1)
                outUids.Insert(uid);
        }
    }

    static bool IsSharedWith(EntityAI vehicle, string uid)
    {
        array<string> uids = new array<string>();
        GetSharedWith(vehicle, uids);
        return uid != "" && uids.Find(uid) != -1;
    }

    static void SetSharedWith(EntityAI vehicle, array<string> uids)
    {
        string raw = "";
        if (uids)
        {
            foreach (string uid : uids)
            {
                if (uid == "")
                    continue;
                if (raw != "")
                    raw = raw + ",";
                raw = raw + uid;
            }
        }
        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            car.m_TXG_SharedWith = raw;
            return;
        }
        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
            boat.m_TXG_SharedWith = raw;
    }

    static void ApplyStamp(EntityAI vehicle, string policyId, string ownerUid, int stampState, int noResaleUntil)
    {
        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            car.m_TXG_PolicyId = policyId;
            car.m_TXG_OwnerUid = ownerUid;
            car.m_TXG_StampState = stampState;
            car.m_TXG_NoResaleUntil = noResaleUntil;
            return;
        }

        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
        {
            boat.m_TXG_PolicyId = policyId;
            boat.m_TXG_OwnerUid = ownerUid;
            boat.m_TXG_StampState = stampState;
            boat.m_TXG_NoResaleUntil = noResaleUntil;
            return;
        }
    }

    static bool ReadStamp(EntityAI vehicle, out string policyId, out string ownerUid, out int stampState, out int noResaleUntil)
    {
        policyId = "";
        ownerUid = "";
        stampState = TXGConstants.STAMP_NONE;
        noResaleUntil = 0;

        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            policyId = car.m_TXG_PolicyId;
            ownerUid = car.m_TXG_OwnerUid;
            stampState = car.m_TXG_StampState;
            noResaleUntil = car.m_TXG_NoResaleUntil;
            return true;
        }

        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
        {
            policyId = boat.m_TXG_PolicyId;
            ownerUid = boat.m_TXG_OwnerUid;
            stampState = boat.m_TXG_StampState;
            noResaleUntil = boat.m_TXG_NoResaleUntil;
            return true;
        }

        return false;
    }

    static void SetGarageRemoval(EntityAI vehicle, bool removing)
    {
        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            car.m_TXG_GarageRemoval = removing;
            return;
        }

        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
        {
            boat.m_TXG_GarageRemoval = removing;
            return;
        }
    }
}
