/**
 * TraderXProviderRegistry — singleton port registry for cross-mod integration.
 *
 * Each module registers its provider implementation at init (Banking already does).
 * External mods call TraderXProviderRegistry.GetInstance().GetXProvider() without
 * depending on the module PBO. Signatures stay vanilla-only at the interface layer.
 */
class TraderXProviderRegistry
{
    protected static ref TraderXProviderRegistry s_Instance;

    protected ref ITraderXPaymentProvider  m_Payment;
    protected ref ITraderXGarageProvider   m_Garage;
    protected ref ITraderXCarLockProvider  m_CarLock;
    protected ref ITraderXSafeZoneProvider m_SafeZone;

    static TraderXProviderRegistry GetInstance()
    {
        if (!s_Instance) s_Instance = new TraderXProviderRegistry();
        return s_Instance;
    }

    void RegisterBankingProvider(ITraderXPaymentProvider p)  { m_Payment = p; }
    void RegisterGarageProvider(ITraderXGarageProvider p)    { m_Garage = p; }
    void RegisterCarLockProvider(ITraderXCarLockProvider p)  { m_CarLock = p; }
    void RegisterSafeZoneProvider(ITraderXSafeZoneProvider p){ m_SafeZone = p; }

    ITraderXPaymentProvider  GetBankingProvider()  { return m_Payment; }
    ITraderXGarageProvider   GetGarageProvider()   { return m_Garage; }
    ITraderXCarLockProvider  GetCarLockProvider()  { return m_CarLock; }
    ITraderXSafeZoneProvider GetSafeZoneProvider() { return m_SafeZone; }

    bool HasBanking()  { return m_Payment  != null; }
    bool HasGarage()   { return m_Garage   != null; }
    bool HasCarLock()  { return m_CarLock  != null; }
    bool HasSafeZone() { return m_SafeZone != null; }
}
