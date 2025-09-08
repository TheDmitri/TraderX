class TraderXNpc
{
  int               npcId;
  string  					className;
	string            givenName;
	string            role;
	vector						position;
	vector						orientation;
  ref array<string> categoriesId;
  ref array<string> currenciesAccepted;
	ref array<ref TraderXLoadoutItem> loadouts;

  void TraderXNpc(int id, string className, string givenName, string role, vector position, vector orientation, array<ref TraderXLoadoutItem> loadouts, array<string> categoriesId, array<string> currenciesAccepted)
  {
    this.className = className;
    this.givenName = givenName;
    this.role = role;
    this.position = position;
    this.orientation = orientation;
    this.loadouts = loadouts;
    this.categoriesId = categoriesId;
    this.currenciesAccepted = currenciesAccepted;

    npcId = TraderXNpcId.Generate().Value();
  }

  // Getters
  int GetNpcId() { return npcId; }
  string GetClassName() { return className; }
  string GetGivenName() { return givenName; }
  string GetRole() { return role; }
  vector GetPosition() { return position; }
  vector GetOrientation() { return orientation; }
  array<string> GetCategories() { return categoriesId; }
  array<string> GetCurrenciesAccepted() { return currenciesAccepted; }
  array<ref TraderXLoadoutItem> GetLoadouts() { return loadouts; }

  // Setters
  void SetClassName(string name) { className = name; }
  void SetGivenName(string name) { givenName = name; }
  void SetRole(string newRole) { role = newRole; }
  void SetPosition(vector pos) { position = pos; }
  void SetOrientation(vector ori) { orientation = ori; }
  void SetCategories(array<string> categories) { categoriesId = categories; }
  void SetCurrenciesAccepted(array<string> currencies) { currenciesAccepted = currencies; }
  void SetLoadouts(array<ref TraderXLoadoutItem> items) { loadouts = items; }
};
