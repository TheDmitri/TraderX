// TraderXBanking - PlayerBase Extension
// Adds banking account reference and banking action to players

modded class PlayerBase
{
	ref TXBAccountData m_TXBBankAccount;

	TXBAccountData GetTXBBankAccount()
	{
		if (!m_TXBBankAccount && GetGame().IsServer())
		{
			m_TXBBankAccount = GetTXBBankingService().GetOrCreateAccount(this);
		}
		return m_TXBBankAccount;
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTXBOpenBank);
	}
}
