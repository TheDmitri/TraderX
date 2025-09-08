/*
	Open source code taken from DayZ Editor
	It allows to load json file bigger than 64ko
	Thank you guys for that ;)
*/
class TraderXJsonLoader<Class T>
{
	protected static ref JsonSerializer m_Serializer = new JsonSerializer;

	static void StringToObject(string string_data, out T data)
	{
		string error;
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;

		if (!m_Serializer.ReadFromString(data, string_data, error)) {
			GetTraderXLogger().LogError(error);
		}
	}

	static string ObjectToString(T data)
	{
		string string_data;
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;

		if (!m_Serializer.WriteToString(data, true, string_data)) {
			GetTraderXLogger().LogError("TraderXJsonLoader::ObjectToString Could not stringify "+ data.ToString());
			return string.Empty;
		}

		return string_data;
	}

	static void SaveToFile(string path, T data)
	{
		FileHandle fh = OpenFile(path, FileMode.WRITE);

		if (!fh) {
			GetTraderXLogger().LogError("TraderXJsonLoader::SaveToFile File could not be created at "+ path);
			return;
		}

		string jsonData;
		bool success = m_Serializer.WriteToString(data, true, jsonData);

		if (success && jsonData != string.Empty) {
			FPrintln(fh, jsonData);
		}

		GetTraderXLogger().LogDebug("TraderXJsonLoader::SaveToFile Complete: " + path);
		CloseFile(fh);
	}

	static void LoadFromFile(string path, out T data, bool isLog = true)
	{
		if (!FileExist(path)) {
			GetTraderXLogger().LogError("TraderXJsonLoader::LoadFromFile File not found "+ path);
			return;
		}

		FileHandle fh = OpenFile(path, FileMode.READ);
		string jsonData;
		string error;

		if (!fh) {
			GetTraderXLogger().LogError("TraderXJsonLoader::LoadFromFile File could not be opened " + path);
			return;
		}

		string line;
		while (FGets(fh, line) > 0) {
			jsonData = jsonData + "\n" + line;
		}

		bool success = m_Serializer.ReadFromString(data, jsonData, error);

		if (error != string.Empty || !success) {
			GetTraderXLogger().LogError("TraderXJsonLoader::LoadFromFile ERROR Parsing "+ path + error);
			return;
		}

		if(isLog)
			GetTraderXLogger().LogDebug("TraderXJsonLoader::LoadFromFile Loaded file: " + path);

		CloseFile(fh);
	}
};
