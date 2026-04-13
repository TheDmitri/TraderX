/**
 * Central registry for external module providers.
 * External mods register their providers here at startup.
 * TraderX core queries this registry to determine what integrations are available.
 *
 * Usage by external mods:
 *   TraderXProviderRegistry.GetInstance().RegisterBankingProvider(new MyBankingProvider());
 *
 * Usage by TraderX core:
 *   if (TraderXProviderRegistry.GetInstance().HasBankingProvider())
 *       int balance = TraderXProviderRegistry.GetInstance().GetBankingProvider().GetBalance(player);
 */
class TraderXProviderRegistry
{
    private static ref TraderXProviderRegistry m_Instance;

    private ref ITraderXPaymentProvider m_BankingProvider;
    private ref ITraderXGarageProvider m_GarageProvider;
    private ref ITraderXCarLockProvider m_CarLockProvider;
    private ref ITraderXInsuranceProvider m_InsuranceProvider;
    private ref ITraderXReceiptProvider m_ReceiptProvider;

    static TraderXProviderRegistry GetInstance()
    {
        if (!m_Instance)
            m_Instance = new TraderXProviderRegistry();
        return m_Instance;
    }

    // --- Banking Provider ---

    void RegisterBankingProvider(ITraderXPaymentProvider provider)
    {
        m_BankingProvider = provider;
        Print("[TraderX] Banking provider registered: " + provider.GetDisplayName());
    }

    void UnregisterBankingProvider()
    {
        m_BankingProvider = null;
        Print("[TraderX] Banking provider unregistered");
    }

    bool HasBankingProvider()
    {
        return m_BankingProvider != null;
    }

    ITraderXPaymentProvider GetBankingProvider()
    {
        return m_BankingProvider;
    }

    // --- Garage Provider ---

    void RegisterGarageProvider(ITraderXGarageProvider provider)
    {
        m_GarageProvider = provider;
        Print("[TraderX] Garage provider registered");
    }

    void UnregisterGarageProvider()
    {
        m_GarageProvider = null;
        Print("[TraderX] Garage provider unregistered");
    }

    bool HasGarageProvider()
    {
        return m_GarageProvider != null;
    }

    ITraderXGarageProvider GetGarageProvider()
    {
        return m_GarageProvider;
    }

    // --- CarLock Provider ---

    void RegisterCarLockProvider(ITraderXCarLockProvider provider)
    {
        m_CarLockProvider = provider;
        Print("[TraderX] CarLock provider registered");
    }

    void UnregisterCarLockProvider()
    {
        m_CarLockProvider = null;
        Print("[TraderX] CarLock provider unregistered");
    }

    bool HasCarLockProvider()
    {
        return m_CarLockProvider != null;
    }

    ITraderXCarLockProvider GetCarLockProvider()
    {
        return m_CarLockProvider;
    }

    // --- Insurance Provider ---

    void RegisterInsuranceProvider(ITraderXInsuranceProvider provider)
    {
        m_InsuranceProvider = provider;
        Print("[TraderX] Insurance provider registered: " + provider.GetDisplayName());
    }

    void UnregisterInsuranceProvider()
    {
        m_InsuranceProvider = null;
        Print("[TraderX] Insurance provider unregistered");
    }

    bool HasInsuranceProvider()
    {
        return m_InsuranceProvider != null;
    }

    ITraderXInsuranceProvider GetInsuranceProvider()
    {
        return m_InsuranceProvider;
    }

    // --- Receipt Provider ---

    void RegisterReceiptProvider(ITraderXReceiptProvider provider)
    {
        m_ReceiptProvider = provider;
        Print("[TraderX] Receipt provider registered: " + provider.GetDisplayName());
    }

    void UnregisterReceiptProvider()
    {
        m_ReceiptProvider = null;
        Print("[TraderX] Receipt provider unregistered");
    }

    bool HasReceiptProvider()
    {
        return m_ReceiptProvider != null;
    }

    ITraderXReceiptProvider GetReceiptProvider()
    {
        return m_ReceiptProvider;
    }
}
