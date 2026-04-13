// TraderXGarage - GarageAppComponent
// Main garage UI component. Displays vehicle list, preview, costs, and park in/out buttons.
// Uses DayZular component system with data binding.

class GarageAppComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register =
		new ComponentAutoRegister("txg-garage-app-component", GarageAppComponent);

	// ─── Bound Properties (used in layout bindings) ───
	string costParkInText;
	string costParkOutText;
	string moneyText;
	EntityAI vehiclePreview;
	ref array<ref GarageVehicleViewData> _inputVehicles = new array<ref GarageVehicleViewData>();

	// ─── Internal State ───
	private int m_PlayerLowId;
	private vector m_NpcPosition;
	private vector m_ParkingPosition;
	private bool m_IsFlag;
	private int m_SelectedVehicleIndex = -1;
	private string m_SelectedVehicleName;
	private ref TStringArray m_VehicleNames;
	private int m_MoneyOnPlayer;
	private int m_ParkInFee;
	private int m_ParkOutFee;
	private ref CarScript m_CarInPark;
	private int m_LastThrottleTime;

	// ─── Widget References ───
	ImageWidget ParkingStatus;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txg-garage-app-component";
		m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GarageAppComponent.layout";
	}

	override void OnInit()
	{
		GarageVehicleCardComponent.Event_OnVehicleSelected = new ScriptInvoker();
		GarageVehicleCardComponent.Event_OnVehicleSelected.Insert(OnVehicleSelected);

		// Load settings from client cache
		GarageRpcService rpcService = GetGarageRpcService();
		if (rpcService)
		{
			ref GarageSettings settings = rpcService.GetClientSettings();
			if (settings)
			{
				m_ParkInFee = settings.parkInCost;
				m_ParkOutFee = settings.parkOutCost;
			}
		}

		costParkInText = "$" + m_ParkInFee.ToString();
		costParkOutText = "$" + m_ParkOutFee.ToString();

		UpdateMoney();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		CleanupVehiclePreview();

		if (GarageVehicleCardComponent.Event_OnVehicleSelected)
			GarageVehicleCardComponent.Event_OnVehicleSelected.Remove(OnVehicleSelected);
	}

	// ─── Data Setters (called before OnInit) ───

	void SetGarageData(int playerLowId, vector npcPos, bool isFlag)
	{
		m_PlayerLowId = playerLowId;
		m_NpcPosition = npcPos;
		m_IsFlag = isFlag;
	}

	void SetResponseData(TStringArray vehicleNames, vector parkingPos)
	{
		m_VehicleNames = vehicleNames;
		m_ParkingPosition = parkingPos;
		BuildVehicleList();
		UpdateParkingStatus();
		UpdateMoney();
		MarkForCheck();
	}

	// ─── UI Construction ───

	private void BuildVehicleList()
	{
		_inputVehicles.Clear();

		if (!m_VehicleNames)
			return;

		// Check which vehicle is at the parking spot
		CarScript carAtSpot = GarageWorldHelpers.CheckForCarAtPosition(m_ParkingPosition);
		m_CarInPark = carAtSpot;

		for (int i = 0; i < m_VehicleNames.Count(); i++)
		{
			string name = m_VehicleNames.Get(i);
			bool isAtSpot = (carAtSpot && carAtSpot.GetType() == name);

			ref GarageVehicleViewData viewData = new GarageVehicleViewData(name, isAtSpot, i);
			_inputVehicles.Insert(viewData);
		}

		// Auto-select first vehicle
		if (_inputVehicles.Count() > 0)
			OnVehicleSelected(_inputVehicles.Get(0));
	}

	private void UpdateParkingStatus()
	{
		if (!ParkingStatus)
			return;

		bool isAvailable = !m_CarInPark;
		if (isAvailable)
			ParkingStatus.SetColor(ARGB(255, 48, 191, 48));  // Green
		else
			ParkingStatus.SetColor(ARGB(255, 191, 48, 48));  // Red
	}

	private void UpdateMoney()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
			return;

		GarageRpcService rpcService = GetGarageRpcService();
		if (rpcService)
		{
			ref GarageSettings settings = rpcService.GetClientSettings();
			if (settings && settings.payWithBankAccount)
			{
				#ifdef TRADERPLUS_BANKING
				string playerId = player.GetIdentity().GetPlainId();
				m_MoneyOnPlayer = TraderPlusBankingData.GetBankBalance(playerId);
				#endif
			}
			else
			{
				m_MoneyOnPlayer = GarageWorldHelpers.GetPlayerMoney(player);
			}
		}

		moneyText = "$" + m_MoneyOnPlayer.ToString();
	}

	// ─── Vehicle Preview ───

	private void UpdateVehiclePreview(string className)
	{
		CleanupVehiclePreview();

		if (className == "")
			return;

		vehiclePreview = EntityAI.Cast(GetGame().CreateObjectEx(className, vector.Zero, ECE_LOCAL));
		MarkForCheck();
	}

	private void CleanupVehiclePreview()
	{
		if (vehiclePreview)
		{
			GetGame().ObjectDelete(vehiclePreview);
			vehiclePreview = null;
		}
	}

	// ─── Event Handlers ───

	void OnVehicleSelected(GarageVehicleViewData data)
	{
		if (!data)
			return;

		m_SelectedVehicleIndex = data.index;
		m_SelectedVehicleName = data.vehicleName;
		UpdateVehiclePreview(data.vehicleName);
	}

	void OnParkInClicked(ClickEventArgs args)
	{
		if (!CanSendRequest())
			return;

		GarageRpcService rpcService = GetGarageRpcService();
		ref GarageSettings settings;
		if (rpcService)
			settings = rpcService.GetClientSettings();

		if (!settings)
			return;

		// Validate money client-side
		if (m_MoneyOnPlayer < m_ParkInFee)
		{
			NotificationSystem.AddNotification(NotificationType.GENERIC, NotificationSystem.DEFAULT_TIME_DISPLAYED, "Garage", settings.msgNotEnoughMoney);
			return;
		}

		// Find car at parking position
		CarScript car = GarageWorldHelpers.CheckForCarAtPosition(m_ParkingPosition);
		if (!car)
			return;

		// Validate max vehicles client-side
		if (settings.maxVehiclesStored > 0 && _inputVehicles.Count() >= settings.maxVehiclesStored)
		{
			NotificationSystem.AddNotification(NotificationType.GENERIC, NotificationSystem.DEFAULT_TIME_DISPLAYED, "Garage", settings.msgMaxVehiclesReached);
			return;
		}

		OnRequestSent();
		Param3<int, CarScript, vector> params = new Param3<int, CarScript, vector>(m_PlayerLowId, car, m_NpcPosition);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_PARK_IN, params, true, null);
	}

	void OnParkOutClicked(ClickEventArgs args)
	{
		if (!CanSendRequest())
			return;

		if (m_SelectedVehicleIndex < 0 || m_SelectedVehicleName == "")
			return;

		GarageRpcService rpcService = GetGarageRpcService();
		ref GarageSettings settings;
		if (rpcService)
			settings = rpcService.GetClientSettings();

		if (!settings)
			return;

		// Validate money client-side
		if (m_MoneyOnPlayer < m_ParkOutFee)
		{
			NotificationSystem.AddNotification(NotificationType.GENERIC, NotificationSystem.DEFAULT_TIME_DISPLAYED, "Garage", settings.msgNotEnoughMoney);
			return;
		}

		OnRequestSent();
		Param4<int, int, string, vector> params = new Param4<int, int, string, vector>(m_PlayerLowId, m_SelectedVehicleIndex, m_SelectedVehicleName, m_NpcPosition);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_PARK_OUT, params, true, null);
	}

	void OnCloseClicked(ClickEventArgs args)
	{
		GarageUIService uiService = GetGarageUIService();
		if (uiService)
			uiService.CloseGarageView();
	}

	// ─── Transaction Throttle ───

	private bool CanSendRequest()
	{
		int now = GetGame().GetTime();
		return (now - m_LastThrottleTime) >= TXG_TRANSACTION_INTERVAL_MS;
	}

	private void OnRequestSent()
	{
		m_LastThrottleTime = GetGame().GetTime();
	}
}
