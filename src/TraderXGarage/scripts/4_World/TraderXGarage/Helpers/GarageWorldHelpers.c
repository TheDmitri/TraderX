// TraderXGarage - GarageWorldHelpers
// World-level utility methods that depend on 4_World types (PlayerBase, CarScript, EntityAI, CargoBase).

class GarageWorldHelpers
{
	// ─── Vehicle Detection ───

	static CarScript CheckForCarAtPosition(vector pos, float radius = 3.0)
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxyCargos = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(pos, radius, objects, proxyCargos);

		foreach (Object obj : objects)
		{
			CarScript car = CarScript.Cast(obj);
			if (car)
				return car;
		}

		return null;
	}

	static bool CarHasMember(CarScript car)
	{
		if (!car)
			return false;

		for (int i = 0; i < car.CrewSize(); i++)
		{
			if (car.CrewMember(i))
				return true;
		}

		return false;
	}

	// ─── Parking Availability ───

	static bool IsParkingAvailable(vector parkPos, vector parkOrient, TStringArray whitelistedObjects)
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxyCargos = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(parkPos, 3.0, objects, proxyCargos);

		foreach (Object obj : objects)
		{
			string objType = obj.GetType();

			if (objType == "")
				continue;

			bool isWhitelisted = false;
			foreach (string whitelisted : whitelistedObjects)
			{
				if (objType == whitelisted)
				{
					isWhitelisted = true;
					break;
				}
			}

			if (!isWhitelisted)
				return false;
		}

		return true;
	}

	// ─── Money Utilities ───

	static int GetPlayerMoney(PlayerBase player)
	{
		if (!player)
			return 0;

		int total = 0;
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		foreach (EntityAI item : items)
		{
			if (item.GetType() == "TraderPlus_Money_Dollar1")
				total += 1;
			else if (item.GetType() == "TraderPlus_Money_Dollar5")
				total += 5;
			else if (item.GetType() == "TraderPlus_Money_Dollar10")
				total += 10;
			else if (item.GetType() == "TraderPlus_Money_Dollar25")
				total += 25;
			else if (item.GetType() == "TraderPlus_Money_Dollar50")
				total += 50;
			else if (item.GetType() == "TraderPlus_Money_Dollar100")
				total += 100;
		}

		return total;
	}

	static bool CheckIfPlayerHasEnoughMoney(PlayerBase player, int amount, bool useBankAccount)
	{
		if (amount <= 0)
			return true;

		if (useBankAccount && TraderXProviderRegistry.GetInstance().HasBankingProvider())
		{
			ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
			if (bankProvider.IsAvailable(player))
				return bankProvider.GetBalance(player) >= amount;
			return false;
		}

		return GetPlayerMoney(player) >= amount;
	}

	static bool RemoveMoneyFromPlayer(PlayerBase player, int amount, bool useBankAccount)
	{
		if (amount <= 0)
			return true;

		if (useBankAccount && TraderXProviderRegistry.GetInstance().HasBankingProvider())
		{
			ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
			if (bankProvider.IsAvailable(player))
				return bankProvider.Deduct(player, amount);
			return false;
		}

		return RemoveMoneyFromInventory(player, amount);
	}

	private static bool RemoveMoneyFromInventory(PlayerBase player, int amount)
	{
		int remaining = amount;
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		foreach (EntityAI item : items)
		{
			if (remaining <= 0)
				break;

			int value = 0;
			string type = item.GetType();

			if (type == "TraderPlus_Money_Dollar100")
				value = 100;
			else if (type == "TraderPlus_Money_Dollar50")
				value = 50;
			else if (type == "TraderPlus_Money_Dollar25")
				value = 25;
			else if (type == "TraderPlus_Money_Dollar10")
				value = 10;
			else if (type == "TraderPlus_Money_Dollar5")
				value = 5;
			else if (type == "TraderPlus_Money_Dollar1")
				value = 1;

			if (value > 0 && value <= remaining)
			{
				remaining -= value;
				GetGame().ObjectDelete(item);
			}
		}

		return remaining <= 0;
	}

	// ─── Inventory Serialization ───

	static ref GarageItemData GetGarageItemData(EntityAI entity)
	{
		if (!entity)
			return null;

		ref GarageItemData data = new GarageItemData();
		data.item = entity.GetType();
		data.quantity = entity.GetQuantity();
		data.health = entity.GetHealth();
		data.children = new array<ref GarageItemData>();

		// Serialize attachments
		for (int i = 0; i < entity.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = entity.GetInventory().GetAttachmentFromIndex(i);
			if (attachment)
			{
				ref GarageItemData attachData = GetGarageItemData(attachment);
				if (attachData)
				{
					attachData.locationType = "ATTACHMENT";
					data.children.Insert(attachData);
				}
			}
		}

		// Serialize cargo items
		CargoBase cargo = entity.GetInventory().GetCargo();
		if (cargo)
		{
			for (int j = 0; j < cargo.GetItemCount(); j++)
			{
				EntityAI cargoItem = cargo.GetItem(j);
				if (cargoItem)
				{
					ref GarageItemData cargoData = GetGarageItemData(cargoItem);
					if (cargoData)
					{
						cargoData.locationType = "CARGO";
						cargoData.locationRow = cargoItem.GetInventory().GetRow();
						cargoData.locationColumn = cargoItem.GetInventory().GetCol();
						cargoData.locationFlipped = cargoItem.GetInventory().GetFlip();
						data.children.Insert(cargoData);
					}
				}
			}
		}

		return data;
	}

	static void RestoreCargo(EntityAI parent, ref GarageItemData itemData)
	{
		if (!parent || !itemData)
			return;

		if (!itemData.children)
			return;

		foreach (ref GarageItemData childData : itemData.children)
		{
			if (!childData || childData.item == "")
				continue;

			EntityAI createdItem = CreateItem(parent, childData);
			if (createdItem && childData.children && childData.children.Count() > 0)
				RestoreCargo(createdItem, childData);
		}
	}

	static EntityAI CreateItem(EntityAI parent, ref GarageItemData itemData)
	{
		if (!parent || !itemData)
			return null;

		EntityAI item;

		if (itemData.locationType == "ATTACHMENT")
		{
			item = parent.GetInventory().CreateAttachment(itemData.item);
		}
		else
		{
			item = parent.GetInventory().CreateInInventory(itemData.item);
		}

		if (item)
		{
			if (itemData.health > 0)
				item.SetHealth(itemData.health);

			if (itemData.quantity > 0)
				item.SetQuantity(itemData.quantity);
		}

		return item;
	}

	// ─── Vehicle Creation ───

	static CarScript CreateVehicle(string className, vector position, vector orientation, float engineHealth, float fuelAmount, int vehicleId)
	{
		Object obj = GetGame().CreateObject(className, position, false, false, true);
		CarScript car = CarScript.Cast(obj);

		if (!car)
		{
			if (obj)
				GetGame().ObjectDelete(obj);
			return null;
		}

		car.SetPosition(position);
		car.SetOrientation(orientation);

		if (engineHealth > 0)
			car.SetHealth("Engine", "", engineHealth);

		if (fuelAmount > 0)
			car.Fill(CarFluid.FUEL, fuelAmount);

		return car;
	}
}
