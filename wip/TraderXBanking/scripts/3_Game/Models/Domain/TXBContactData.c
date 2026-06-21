// TraderXBanking - Contacts Data Model
// Per-player address book of frequently-used transfer recipients.
// Populated on successful transfer (send + receive), stored client + server.

class TXBContact
{
	string uid;
	string name;
	int transferCount;
	int lastUtc;

	void TXBContact()
	{
		uid = "";
		name = "";
		transferCount = 0;
		lastUtc = 0;
	}
}

class TXBContactData
{
	string version;
	string ownerUID;
	ref array<ref TXBContact> contacts;

	void TXBContactData()
	{
		version = TXBConstants.CONTACTS_VERSION;
		ownerUID = "";
		contacts = new array<ref TXBContact>();
	}

	static string GetContactsFilePath(string uid)
	{
		return TXBConstants.CONTACTS_DIR + "Contacts_" + uid + ".json";
	}

	static ref TXBContactData Load(string uid)
	{
		if (!FileExist(TXBConstants.CONTACTS_DIR))
			MakeDirectory(TXBConstants.CONTACTS_DIR);

		string filePath = GetContactsFilePath(uid);
		ref TXBContactData data;

		if (FileExist(filePath))
		{
			data = new TXBContactData();
			JsonFileLoader<TXBContactData>.JsonLoadFile(filePath, data);
			if (data.version != TXBConstants.CONTACTS_VERSION)
			{
				data.version = TXBConstants.CONTACTS_VERSION;
			}
		}
		else
		{
			data = new TXBContactData();
			data.ownerUID = uid;
		}

		return data;
	}

	void Save()
	{
		if (!FileExist(TXBConstants.CONTACTS_DIR))
			MakeDirectory(TXBConstants.CONTACTS_DIR);

		string filePath = GetContactsFilePath(ownerUID);
		JsonFileLoader<TXBContactData>.JsonSaveFile(filePath, this);
	}

	void RecordTransfer(string otherUid, string otherName, int utcNow)
	{
		if (otherUid == "" || otherUid == ownerUID) return;

		foreach (TXBContact c : contacts)
		{
			if (c && c.uid == otherUid)
			{
				c.name = otherName;
				c.transferCount = c.transferCount + 1;
				c.lastUtc = utcNow;
				return;
			}
		}

		ref TXBContact fresh = new TXBContact();
		fresh.uid = otherUid;
		fresh.name = otherName;
		fresh.transferCount = 1;
		fresh.lastUtc = utcNow;
		contacts.Insert(fresh);
	}

	TXBContact FindByUid(string otherUid)
	{
		foreach (TXBContact c : contacts)
		{
			if (c && c.uid == otherUid)
				return c;
		}
		return null;
	}
}
