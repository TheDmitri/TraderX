class TraderXGeneralSettings {
    string version = TRADERX_CURRENT_VERSION;
	string serverID;
    ref array<ref TraderXLicense> licenses;
    ref TraderXStates acceptedStates;
    ref array<ref TraderXNpc> traders;
    ref array<ref TraderXObject> traderObjects;

    void TraderXGeneralSettings() {
        licenses = new array<ref TraderXLicense>();
        traders = new array<ref TraderXNpc>();
        traderObjects = new array<ref TraderXObject>();
    }

	TraderXNpc GetNpcById(int id)
	{
		foreach(TraderXNpc npc : traders)
		{
			if(npc.npcId == id)
				return npc;
		}

		return null;
	}

	void DefaultTraderXGeneralSettings()
	{
		GenerateServerID(serverID);
    	licenses.Insert(new TraderXLicense("Car License", "Allows you to drive a car."));
		licenses.Insert(new TraderXLicense("Admin License", "Allows you to do anything."));

		acceptedStates = new TraderXStates(true, true, true);
		
		traderObjects.Insert(new TraderXObject("bldr_Misc_RoadBarrier",Vector(11537.2001953125,  59.0, 14726.099609375), Vector(55.000003814697269, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("bldr_Wall_Barricade1_10",Vector(11590.5498046875, 58.224754333496097, 14760.6728515625), Vector(144,	0,	0)));
		traderObjects.Insert(new TraderXObject("bldr_Wall_Barricade1_4",Vector(11615.189453125,   57.636619567871097, 14626.82421875), Vector(153,	0,	0)));
		traderObjects.Insert(new TraderXObject("bldr_Misc_Barricade",Vector(11613.2607421875,    57.99546432495117,   14622.95703125), Vector(54,	0,	0)));
		traderObjects.Insert(new TraderXObject("Land_Wreck_S1023_Blue",Vector(11596.6044921875,  58.728797912597659,  14630.3427734375), Vector(-135,0,		0)));
		traderObjects.Insert(new TraderXObject("Land_Wreck_Lada_Red",Vector(11598.5400390625,    58.41791915893555,   14628.796875), Vector(171.999535,0,		0)));
		traderObjects.Insert(new TraderXObject("bldr_Wall_Barricade1_4",Vector( 11593.5087890625, 57.81910705566406,  14632.2060546875), Vector(63,	0,	0)));
		traderObjects.Insert(new TraderXObject("bldr_Wall_Barricade1_10",Vector(11609.3408203125, 58.077457427978519, 14766.326171875), Vector(54,0,0)));
		traderObjects.Insert(new TraderXObject("bldr_Wall_Barricade1_4",Vector( 11593.5087890625, 57.81910705566406,  14632.2060546875), Vector(63,	0,	0)));
		traderObjects.Insert(new TraderXObject("bldr_hbarrier_4m",Vector(11664.1875, 57.7883186340332, 14659.564453125), Vector(-36.00001525878906, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("bldr_hbarrier_4m",Vector(11654.0380859375, 58.30573272705078, 14652.6845703125), Vector(-36.00001525878906, 0.0, 0.0)));
    	traderObjects.Insert(new TraderXObject("bldr_hbarrier_4m",Vector(11656.80078125, 58.16636657714844, 14653.1533203125), Vector(53.9, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("bldr_hbarrier_4m",Vector(11662.8837890625, 57.84144973754883,14657.0732421875), Vector(53.9, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_City_Stand_FastFood",Vector(11594.310546875, 59.78278350830078, 14673.0546875), Vector(-117.0, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_Misc_TrailRoof_Small",Vector(11598.7998046875, 59.28260040283203, 14669.2001953125), Vector(62, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_Misc_TrailRoof_Small",Vector(11600.9658203125, 59.28261184692383, 14665.07421875), Vector(62, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_Misc_Well_Pump_Yellow",Vector(11610.8466796875,58.61098861694336, 14674.2265625), Vector(126, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_Garage_Office",Vector(11627.701171875, 67.97467041015625, 14805.9326171875), Vector(0, 0.0, 0.0)));
		traderObjects.Insert(new TraderXObject("Land_Garage_Small",Vector( 11615.798828125, 59.178348541259769,14687.2724609375), Vector(50, 0.0, 0.0)));
		TraderXSettingsRepository.Save(this);
	}

	void GenerateServerID(out string sID)
	{
		int uuid[4];
		UUIDApi.Generate(uuid);
		sID = UUIDApi.FormatString(uuid);
	}
}
