// TraderXBanking - Item utilities
// Shared client+server helpers for item metadata that isn't on the entity directly.

class TXBItemUtil
{
	// Inventory grid footprint (itemSize[] = {w,h}) read from config. Falls back to 1x1.
	// Item configs live under CfgVehicles / CfgWeapons / CfgMagazines depending on type.
	static void GetFootprint(string className, out int w, out int h)
	{
		w = 1;
		h = 1;
		if (className == "")
			return;

		string base = "CfgVehicles " + className;
		if (!GetGame().ConfigIsExisting(base)) base = "CfgWeapons " + className;
		if (!GetGame().ConfigIsExisting(base)) base = "CfgMagazines " + className;
		if (!GetGame().ConfigIsExisting(base)) return;

		array<int> sz = new array<int>();
		GetGame().ConfigGetIntArray(base + " itemSize", sz);
		if (sz.Count() >= 2 && sz[0] > 0 && sz[1] > 0)
		{
			w = sz[0];
			h = sz[1];
		}
	}
}
