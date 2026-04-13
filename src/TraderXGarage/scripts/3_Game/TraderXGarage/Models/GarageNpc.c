// TraderXGarage - GarageNpc
// NPC spawn configuration for garage interaction points. All fields camelCase.

class GarageNpc
{
	string className;
	vector position;
	vector orientation;
	vector parkingPosition;
	vector parkingOrientation;
	ref TStringArray clothes;

	void GarageNpc(string type = "", vector pos = "0 0 0", vector orient = "0 0 0", vector parkPos = "0 0 0", vector parkOrient = "0 0 0")
	{
		className = type;
		position = pos;
		orientation = orient;
		parkingPosition = parkPos;
		parkingOrientation = parkOrient;
		clothes = new TStringArray();
	}

	void AddClothing(string item)
	{
		clothes.Insert(item);
	}
}
