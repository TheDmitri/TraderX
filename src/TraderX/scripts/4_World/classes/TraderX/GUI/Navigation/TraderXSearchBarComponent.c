// Reusable search field. Embed once per page via data-component="tx-search-bar".
// Owns the input + debounce; emits the (debounced) keyword on a static ScriptInvoker
// so the host page reacts without duplicating the editbox/binding on every page.
class TraderXSearchBarComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-search-bar", TraderXSearchBarComponent);

	// Invoked with the current keyword after the debounce window.
	static ref ScriptInvoker Event_OnSearchChangedCallBack = new ScriptInvoker();

	string search_keyword;

	EditBoxWidget searchInput;
	ImageWidget searchHint;
	// "Item name" hint, hidden while there is text
	Widget searchPlaceholder;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-search-bar";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/SearchBarView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
	}

	override void OnDestroy()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(EmitSearch);
	}

	// Two-way bound editbox change -> debounce -> emit.
	void OnSearchChanged()
	{
		if (searchPlaceholder && searchInput) searchPlaceholder.Show(searchInput.GetText() == "");
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(EmitSearch);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(EmitSearch, 300, false);
	}

	void EmitSearch()
	{
		Event_OnSearchChangedCallBack.Invoke(search_keyword);
	}
}
