// TraderX core hooks on motorbikes (DayZ 1.30): last driver (trader sale ownership check) and
// the same 30 s spawn protection as cars and boats. CF ModStorage for MotorbikeScript comes from
// CF itself (JM/CF/Scripts/4_World/CommunityFramework/Entities/MotorbikeScript.c, 1.30+).
modded class MotorbikeScript
{
	string m_TraderX_LastDriverID = "";

	void MotorbikeScript()
	{
		if (GetGame().IsClient())
			return;

		// Same spawn protection as cars and boats: no damage for the first 30 s.
		SetAllowDamage(false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DisableTXGodMode, 30000);
	}

	void DisableTXGodMode()
	{
		SetAllowDamage(true);
	}

	bool IsLastDriver(PlayerBase player)
	{
		if (!player || !player.GetIdentity())
			return false;
		return m_TraderX_LastDriverID == player.GetIdentity().GetPlainId();
	}

	override void OnEngineStart()
	{
		super.OnEngineStart();
		PlayerBase player = PlayerBase.Cast(CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER));
		if (player && player.GetIdentity())
			m_TraderX_LastDriverID = player.GetIdentity().GetPlainId();
	}
}
