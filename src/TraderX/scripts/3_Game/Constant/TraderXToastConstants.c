// TraderX HUD toast constants.
//
// Type keys + timings for the marketplace toast card (TraderXToastService).
// Ported from CourseBarterTrader's barter notification model — TraderX is a
// buy/sell trader (not p2p), so the type set is purchase / sale / info /
// warning / error. The keys double as imageset frame names in traderx_icons
// (datasets/icons/svg/<key>.svg), so they must stay lowercase and match the SVGs.

const string TXNOTIF_PURCHASE = "purchase";   // item bought (success)
const string TXNOTIF_SALE     = "sale";        // item sold (success)
const string TXNOTIF_INFO     = "info";        // generic info / bulk success
const string TXNOTIF_WARNING  = "warning";     // partial result (mixed success/fail)
const string TXNOTIF_ERROR    = "error";       // failed action
const string TXNOTIF_BEANS    = "beans";       // DayZ Beans Launcher welcome (beans can icon)

// Toast lifecycle timing (ms) — matches CBT's slide-in / hold / slide-out / gap.
const int TX_TOAST_HOLD_MS = 5000;
const int TX_TOAST_IN_MS   = 360;
const int TX_TOAST_OUT_MS  = 320;
const int TX_TOAST_GAP_MS  = 260;
