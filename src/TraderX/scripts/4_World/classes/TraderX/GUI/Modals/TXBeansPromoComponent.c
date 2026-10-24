// TraderX - DayZ Beans Launcher promo modal (shared).
// Auto-advancing carousel showcasing the launcher. Lives in TraderX core so any dependent mod
// (TraderX / TraderXBanking / TraderXGarage) can open it with one call:
//
//     TXBeansPromoComponent.Open();
//
// Opened OVER an already-open mod menu (cursor already active), so it doesn't manage player
// control itself. "GET THE LAUNCHER" opens the site via the Steam overlay.

class TXBeansPromoComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-beans-promo", TXBeansPromoComponent);

	protected static ref TXBeansPromoComponent s_Instance;

	// Carousel: slide0..slide(N-1) panels + dot0..dot(N-1) indicators in the layout.
	protected const int SLIDE_COUNT = 6;
	protected const int SLIDE_INTERVAL_MS = 3800;
	protected int m_SlideIndex = 0;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-beans-promo";
		m_Metadata.templatePath = "TraderX/datasets/gui/BeansPromoView.layout";
	}

	override void OnInit()
	{
		super.OnInit();
		m_SlideIndex = 0;
		ApplySlide();
		// Auto-advance to the next slide on a repeating timer.
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(AdvanceSlide, SLIDE_INTERVAL_MS, true);
	}

	override void OnDestroy()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(AdvanceSlide);
		super.OnDestroy();
	}

	// Open the promo overlay. Idempotent (no-op if already open).
	static void Open()
	{
		if (s_Instance)
			return;

		s_Instance = new TXBeansPromoComponent();
		s_Instance.InitComponent(null);

		Widget root = s_Instance.GetRoot();
		if (!root)
		{
			s_Instance = null;
			return;
		}

		root.Show(true);
		GetGame().GetUIManager().ShowUICursor(true);
	}

	void AdvanceSlide()
	{
		m_SlideIndex = (m_SlideIndex + 1) % SLIDE_COUNT;
		ApplySlide();
	}

	// Show the current slide, hide the rest; the active dot is ember, the others muted (Nasdara).
	protected void ApplySlide()
	{
		Widget root = GetRoot();
		if (!root)
			return;

		int activeCol = ARGB(255, 217, 118, 47);
		int idleCol = ARGB(255, 74, 64, 52);

		for (int i = 0; i < SLIDE_COUNT; i++)
		{
			Widget slide = root.FindAnyWidget("slide" + i.ToString());
			if (slide)
				slide.Show(i == m_SlideIndex);

			Widget dot = root.FindAnyWidget("dot" + i.ToString());
			if (dot)
			{
				if (i == m_SlideIndex)
					dot.SetColor(activeCol);
				else
					dot.SetColor(idleCol);
			}
		}
	}

	// Open the launcher download page through the Steam overlay, then close.
	void OnGetLauncher()
	{
		GetGame().OpenURL("steam://openurl/https://dayzbeanslauncher.com");
		Close();
	}

	void OnCloseClick()
	{
		Close();
	}

	// Stop the timer, hide now, free next frame — destroying mid-click (we're inside a button
	// handler) would free this component on the stack = native CTD. Defer one frame.
	protected void Close()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(AdvanceSlide);
		Widget root = GetRoot();
		if (root)
			root.Show(false);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DeferredDestroy, 1, false);
	}

	protected void DeferredDestroy()
	{
		Destroy();
		s_Instance = null;
	}
}
