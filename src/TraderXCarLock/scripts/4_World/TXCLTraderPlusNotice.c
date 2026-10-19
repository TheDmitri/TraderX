#ifdef CarLock
// TraderPlus V1 is loaded alongside TraderX: both ship a "CarLock" item and lock the same vehicles.
// TraderX CarLock is compiled out (every 4_World file is wrapped in #ifndef CarLock) and TraderPlus
// keeps handling locks. Remove TraderPlus to switch to TraderX CarLock.
[CF_RegisterModule(TXCLTraderPlusNotice)]
class TXCLTraderPlusNotice : CF_ModuleWorld
{
    override void OnInit()
    {
        super.OnInit();
        GetTXCLLog().LogWarning("[TXCL] TraderPlus CarLock detected - TraderX CarLock is disabled, TraderPlus handles vehicle locks.");
    }
}
#endif
