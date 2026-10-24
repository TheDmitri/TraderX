// TraderXGarage - Vehicle receipts (TraderPlus V1 compatible)
// Same class names as TraderPlus so receipts held by players survive the switch to TraderX:
//   NewReceipt - TraderPlus receipt: vehicle class, attachments "class,health", CarLock code and the
//                vehicle cargo in its own locked inventory. TraderX receipts reuse the class and only
//                carry a record id (TXGReceiptData); the vehicle state stays on the server.
//   Receipt    - older TraderPlus receipt: index of the vehicle in the TraderPlus part lists.
// TraderPlus defines these classes itself when it is loaded.
#ifndef TraderPlus
class ReceiptBase extends ItemBase
{
    int CarID = 0;
    int CarAttachments = 0;
    protected bool m_TXG_HasStorageBlock;

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXGDeployReceipt);
    }

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return;

        ctx.Write(CarID);
        ctx.Write(CarAttachments);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return true;

        m_TXG_HasStorageBlock = true;
        int carId;
        int carAttachments;
        if (ctx.Read(carId) && ctx.Read(carAttachments))
        {
            CarID = carId;
            CarAttachments = carAttachments;
        }
        return true;
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Saved by TraderPlus: its data follows CF's block
        if (!m_TXG_HasStorageBlock)
        {
            Param2<int, int> data = new Param2<int, int>(-1, -1);
            if (ctx.Read(data))
            {
                CarID = data.param1;
                CarAttachments = data.param2;
            }
        }
        return true;
    }
}

class Receipt extends ReceiptBase
{
}

class NewReceiptBase extends ItemBase
{
    // Receipt packed by TraderPlus (CarClassName is also the display class of TraderX receipts)
    string CarClassName = "";
    int CarUniqueId;
    int CollateralMoney;
    int CarPassword;
    ref array<string> Attachments = new array<string>;

    // Receipt packed by TraderX: TraderXGarage/Receipts/<id>.json
    string m_TXG_ReceiptId = "";
    protected bool m_TXG_HasStorageBlock;

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXGDeployReceipt);
    }

    bool IsTraderPlusReceipt()
    {
        return m_TXG_ReceiptId == "" && CarClassName != "";
    }

    override string GetDisplayName()
    {
        if (CarClassName == "")
            return super.GetDisplayName();

        string vehicleName = GetGame().ConfigGetTextOut("CfgVehicles " + CarClassName + " displayName");
        if (vehicleName == "")
            vehicleName = CarClassName;
        if (GetGame().IsDedicatedServer())
            return vehicleName;
        return TXGText.Get("#txg_receipt_named", vehicleName);
    }

    // The client only gets the vehicle class for the name: the rest stays on the server
    override void OnInventoryEnter(Man player)
    {
        super.OnInventoryEnter(player);

        if (GetGame().IsServer() && player && player.GetIdentity() && CarClassName != "")
            GetRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnReceiptClass", new Param1<string>(CarClassName), true, player.GetIdentity(), this);
    }

    // The cargo of a TraderPlus receipt belongs to the vehicle: nothing goes in or out (as TraderPlus)
    void LockCargo()
    {
        GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);
        SetSynchDirty();
    }

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return;

        ctx.Write(m_TXG_ReceiptId);
        ctx.Write(CarClassName);
        ctx.Write(CarPassword);
        ctx.Write(CarUniqueId);
        ctx.Write(CollateralMoney);
        ctx.Write(Attachments.Count());
        foreach (string attachment : Attachments)
        {
            ctx.Write(attachment);
        }
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage ctx = storage.Get(TXGConstants.STORAGE_MOD);
        if (!ctx)
            return true;

        m_TXG_HasStorageBlock = true;
        string receiptId;
        string className;
        int password;
        int uniqueId;
        int collateral;
        int count;
        if (!ctx.Read(receiptId) || !ctx.Read(className) || !ctx.Read(password) || !ctx.Read(uniqueId) || !ctx.Read(collateral) || !ctx.Read(count))
            return true;

        m_TXG_ReceiptId = receiptId;
        CarClassName = className;
        CarPassword = password;
        CarUniqueId = uniqueId;
        CollateralMoney = collateral;
        Attachments.Clear();
        for (int i = 0; i < count; i++)
        {
            string attachment;
            if (!ctx.Read(attachment))
                break;
            Attachments.Insert(attachment);
        }
        return true;
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Saved by TraderPlus: its data follows CF's block
        if (!m_TXG_HasStorageBlock)
        {
            Param5<int, int, int, string, array<string>> data = new Param5<int, int, int, string, array<string>>(0, 0, 0, "", new array<string>);
            if (ctx.Read(data))
            {
                CarPassword = data.param1;
                CarUniqueId = data.param2;
                CollateralMoney = data.param3;
                CarClassName = data.param4;
                if (data.param5)
                    Attachments = data.param5;
            }
        }

        if (CarClassName != "")
            LockCargo();
        return true;
    }
}

class NewReceipt extends NewReceiptBase
{
}

// TraderPlus packs vehicles with a wrench (its TraderPlusWrench is a Wrench too)
modded class Wrench
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXGPackVehicle);
    }
}
#endif
