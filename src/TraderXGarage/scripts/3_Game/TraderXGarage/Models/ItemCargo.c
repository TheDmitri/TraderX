// TraderXGarage - ItemCargo
// Simplified cargo item snapshot for vehicle inventory. All fields camelCase.

class ItemCargo
{
	string itemName;
	int amount;
	float health;
	int extraData;

	void ItemCargo(string name = "", int qty = 1, float hp = 100, int extra = 0)
	{
		itemName = name;
		amount = qty;
		health = hp;
		extraData = extra;
	}
}
