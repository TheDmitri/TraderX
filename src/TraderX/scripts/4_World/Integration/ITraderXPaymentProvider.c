/**
 * Port: banking / payment provider.
 * "Interface" = base class with virtual stubs (Enforce has no reliable
 * cross-version `interface` keyword). A module subclasses and overrides.
 * Signatures use vanilla types only — no module-internal type leaks here.
 * Formalizes the contract already implemented by Banking's TXBPaymentProvider.
 */
class ITraderXPaymentProvider
{
    int  GetBalance(Man player) { return 0; }
    int  GetClientBalance() { return 0; }
    bool Deduct(Man player, int amount) { return false; }
    void Credit(Man player, int amount) { }
    bool DeductLabeled(Man player, int amount, string category, string counterpartyName) { return false; }
    void CreditLabeled(Man player, int amount, string category, string counterpartyName) { }
    bool IsAvailable(Man player) { return false; }
    string GetDisplayName() { return ""; }
    void RequestBalanceRefresh() { }
}
