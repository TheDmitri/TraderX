// TraderXGarage - GarageService
// Server-side service handling garage operations: NPC spawning, park-in, park-out, vehicle list.
// Registered via DayZular DZlrServiceRegistry.

class GarageService extends DayZularService
{
	private ref GarageSettings m_Settings;
	private ref array<Object> m_SpawnedNpcs;

	void GarageService()
	{
		m_SpawnedNpcs = new array<Object>();

		if (GetGame().IsServer())
		{
			m_Settings = GarageSettings.Load();
			GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
			GetDayZularEvents().OnPlayerJoined.Insert(OnPlayerJoined);
		}
	}

	ref GarageSettings GetSettings()
	{
		return m_Settings;
	}

	// ─── Lifecycle ───

	void OnMissionStart()
	{
		SpawnNpcs();
	}

	void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
	{
		SyncConfigToClient(identity);
	}

	// ─── NPC Management ───

	private void SpawnNpcs()
	{
		if (!m_Settings || !m_Settings.npcs)
			return;

		foreach (ref GarageNpc npcConfig : m_Settings.npcs)
		{
			if (npcConfig.className == "")
				continue;

			Object obj = GetGame().CreateObject(npcConfig.className, npcConfig.position, false, false, true);
			if (!obj)
				continue;

			obj.SetPosition(npcConfig.position);
			obj.SetOrientation(npcConfig.orientation);

			PlayerBase npcPlayer = PlayerBase.Cast(obj);
			if (npcPlayer)
				SetupNpcClothing(npcPlayer, npcConfig);

			m_SpawnedNpcs.Insert(obj);
		}
	}

	private void SetupNpcClothing(PlayerBase npc, ref GarageNpc config)
	{
		if (!config.clothes)
			return;

		foreach (string clothing : config.clothes)
		{
			npc.GetInventory().CreateAttachment(clothing);
		}
	}

	// ─── Config Sync ───

	void SyncConfigToClient(PlayerIdentity identity)
	{
		if (!m_Settings)
			return;

		Param1<ref GarageSettings> params = new Param1<ref GarageSettings>(m_Settings);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_SYNC_CONFIG, params, true, identity);
	}

	// ─── Garage Request (list vehicles) ───

	void HandleGarageRequest(PlayerBase player, int playerLowId, vector position, bool isFlag)
	{
		if (!player || !player.GetIdentity())
			return;

		string playerId = player.GetIdentity().GetPlainId();
		string playerName = player.GetIdentity().GetName();

		ref GarageData garageData = GarageData.Load(playerId, playerName);
		if (!garageData)
			return;

		// Update parking position from NPC config or flag
		if (!isFlag)
		{
			ref GarageNpc npcConfig = FindNpcByPosition(position);
			if (npcConfig)
			{
				garageData.parkingPosition = npcConfig.parkingPosition;
				garageData.parkingOrientation = npcConfig.parkingOrientation;
			}
		}

		// Clean up expired trade vehicles
		if (m_Settings.useGarageOnlyToTrade)
			CleanExpiredVehicles(garageData, playerLowId);

		garageData.Save(playerId);

		// Build response arrays
		ref TStringArray vehicleNames = new TStringArray();
		ref TIntArray vehicleHealths = new TIntArray();

		foreach (ref VehicleData veh : garageData.vehicles)
		{
			vehicleNames.Insert(veh.vehicleName);
			vehicleHealths.Insert(veh.engineHealth);
		}

		// Send response
		Param4<ref TStringArray, ref TIntArray, vector, int> responseParams = new Param4<ref TStringArray, ref TIntArray, vector, int>(
			vehicleNames,
			vehicleHealths,
			garageData.parkingPosition,
			0
		);

		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_RESPONSE, responseParams, true, player.GetIdentity());
	}

	// ─── Park In ───

	void HandleParkIn(PlayerBase player, int playerLowId, CarScript car, vector position)
	{
		if (!player || !player.GetIdentity() || !car)
			return;

		string playerId = player.GetIdentity().GetPlainId();
		string playerName = player.GetIdentity().GetName();

		ref GarageData garageData = GarageData.Load(playerId, playerName);
		if (!garageData)
			return;

		// Validate max storage
		if (garageData.GetVehicleCount() >= m_Settings.maxVehiclesStored)
		{
			SendNotification(player, m_Settings.msgMaxVehiclesReached);
			return;
		}

		// Validate no crew
		if (GarageWorldHelpers.CarHasMember(car))
		{
			SendNotification(player, m_Settings.msgCarHasMember);
			return;
		}

		// Validate money
		if (!GarageWorldHelpers.CheckIfPlayerHasEnoughMoney(player, m_Settings.parkInCost, m_Settings.payWithBankAccount))
		{
			SendNotification(player, m_Settings.msgNotEnoughMoney);
			return;
		}

		// Deduct money
		GarageWorldHelpers.RemoveMoneyFromPlayer(player, m_Settings.parkInCost, m_Settings.payWithBankAccount);

		// Capture vehicle state
		float engineHealth = car.GetHealth("Engine", "");
		float fuelAmount = car.GetFluidFraction(CarFluid.FUEL) * car.GetFluidCapacity(CarFluid.FUEL);
		int vehicleId = 0;
		int cargoId = 0;
		int timestamp = GetGame().GetTime();

		// Save cargo if enabled
		ref array<ref ItemCargo> itemsCargo = new array<ref ItemCargo>();
		if (m_Settings.saveVehicleCargo)
		{
			cargoId = timestamp;
			ref GarageItemData fullCargoData = GarageWorldHelpers.GetGarageItemData(car);
			if (fullCargoData)
				fullCargoData.Save(playerLowId, cargoId);
		}

		// Store vehicle
		garageData.AddVehicle(
			car.GetType(),
			cargoId,
			vehicleId,
			timestamp,
			m_Settings.saveVehicleHealth ? engineHealth : 0,
			m_Settings.saveVehicleFuel ? fuelAmount : 0,
			itemsCargo,
			0
		);

		// Save lock data if enabled and CarLock provider is available
		if (m_Settings.saveVehicleLock && TraderXProviderRegistry.GetInstance().HasCarLockProvider())
		{
			ITraderXCarLockProvider lockProvider = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
			array<int> serializedLock = lockProvider.SerializeLockData(car);
			if (serializedLock)
			{
				int lastIdx = garageData.vehicles.Count() - 1;
				if (lastIdx >= 0)
					garageData.vehicles[lastIdx].lockData = serializedLock;
			}
		}

		garageData.Save(playerId);

		// Delete the vehicle from the world
		GetGame().ObjectDelete(car);

		SendNotification(player, m_Settings.msgParkInSuccess);

		// Refresh the vehicle list
		HandleGarageRequest(player, playerLowId, position, false);
	}

	// ─── Park Out ───

	void HandleParkOut(PlayerBase player, int playerLowId, int vehicleIndex, string vehicleName, vector position)
	{
		if (!player || !player.GetIdentity())
			return;

		string playerId = player.GetIdentity().GetPlainId();
		string playerName = player.GetIdentity().GetName();

		ref GarageData garageData = GarageData.Load(playerId, playerName);
		if (!garageData)
			return;

		if (vehicleIndex < 0 || vehicleIndex >= garageData.vehicles.Count())
			return;

		// Validate parking spot is clear
		if (!GarageWorldHelpers.IsParkingAvailable(garageData.parkingPosition, garageData.parkingOrientation, m_Settings.whitelistedObjects))
		{
			SendNotification(player, m_Settings.msgParkingNotAvailable);
			return;
		}

		// Validate money
		if (!GarageWorldHelpers.CheckIfPlayerHasEnoughMoney(player, m_Settings.parkOutCost, m_Settings.payWithBankAccount))
		{
			SendNotification(player, m_Settings.msgNotEnoughMoney);
			return;
		}

		ref VehicleData vehData = garageData.vehicles.Get(vehicleIndex);
		if (!vehData || vehData.vehicleName != vehicleName)
			return;

		// Deduct money
		GarageWorldHelpers.RemoveMoneyFromPlayer(player, m_Settings.parkOutCost, m_Settings.payWithBankAccount);

		// Create the vehicle
		CarScript car = GarageWorldHelpers.CreateVehicle(
			vehData.vehicleName,
			garageData.parkingPosition,
			garageData.parkingOrientation,
			vehData.engineHealth,
			vehData.fuelAmount,
			vehData.vehicleId
		);

		if (!car)
		{
			SendNotification(player, m_Settings.msgParkOutFail);
			return;
		}

		// Restore cargo if saved
		if (m_Settings.saveVehicleCargo && vehData.vehicleCargoId != 0)
		{
			ref GarageItemData cargoData = GarageItemData.Load(playerLowId, vehData.vehicleCargoId);
			if (cargoData)
			{
				GarageWorldHelpers.RestoreCargo(car, cargoData);
				GarageItemData.DeleteCargo(playerLowId, vehData.vehicleCargoId);
			}
		}

		// Restore lock data if saved and CarLock provider is available
		if (m_Settings.saveVehicleLock && vehData.lockData && TraderXProviderRegistry.GetInstance().HasCarLockProvider())
		{
			ITraderXCarLockProvider lockProvider = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
			lockProvider.DeserializeLockData(car, vehData.lockData);
		}

		// Remove from garage
		garageData.RemoveVehicle(vehicleIndex);
		garageData.Save(playerId);

		SendNotification(player, m_Settings.msgParkOutSuccess);

		// Refresh the vehicle list
		HandleGarageRequest(player, playerLowId, position, false);
	}

	// ─── Helpers ───

	private ref GarageNpc FindNpcByPosition(vector position)
	{
		if (!m_Settings || !m_Settings.npcs)
			return null;

		foreach (ref GarageNpc npc : m_Settings.npcs)
		{
			if (vector.Distance(npc.position, position) < 5.0)
				return npc;
		}

		return null;
	}

	private void CleanExpiredVehicles(ref GarageData garageData, int playerLowId)
	{
		if (!garageData || !garageData.vehicles)
			return;

		int currentTime = GetGame().GetTime();
		int expirySeconds = m_Settings.savedVehicleInGarageForTradeInHours * 3600;

		for (int i = garageData.vehicles.Count() - 1; i >= 0; i--)
		{
			ref VehicleData veh = garageData.vehicles.Get(i);
			if (veh && veh.timestamp > 0 && (veh.timestamp + expirySeconds) < currentTime)
			{
				veh.DeleteCargoIfExists(playerLowId);
				garageData.vehicles.Remove(i);
			}
		}
	}

	private void SendNotification(PlayerBase player, string message)
	{
		if (!player || !player.GetIdentity())
			return;

		Param1<string> msgParam = new Param1<string>(message);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, "Notification", msgParam, true, player.GetIdentity());
	}
}
