/**
 * TraderXTheme — the ONLY hardcoded colors left in TraderX code.
 *
 * Everything visual that can be a style lives in the atlas (source:
 * datasets/gui/_fixtures/field-ledger.html → build-styles.sh). This class holds
 * only the colors that are applied at RUNTIME by state (a SetColor tint that no
 * static style can express): item-card warning tint, nav/slot active state,
 * tooltip availability, stock-low, preset marker.
 *
 * Re-skinning: a modder changes the atlas tokens for 99% of the UI; these few
 * runtime colors are the remaining 1%. Keep them aligned with the CBT palette
 * (gold #C4A35A / green #6B8E5A / slate / red #C45050).
 *
 * Replaces the old sprawling TraderXViewStyles (deleted in US-025); dead tokens
 * (UI_*, CATEGORY_*, TEXT_IDLE/GRAY) were dropped.
 */
class TraderXTheme
{
    // Item card content tint (multiplied onto the TXSurface/TXCard style):
    // IDLE is ~white so the style shows; WARNING is a light warm tint.
    static int CARD_IDLE    = 0xfffefefe;
    static int CARD_WARNING = 0xffe6b8a0;

    // Tooltip variant/attachment availability text.
    static int TEXT_OK   = 0xff6b8e5a;  // available (green)
    static int TEXT_WARN = 0xff754d57;  // unavailable

    // Nav tab label.
    static int NAV_ACTIVE   = 0xffc4a35a;  // gold
    static int NAV_INACTIVE = 0xff8a8f96;  // muted

    // Player slot icon — selected matches the nav tab gold/orange.
    static int SLOT_ACTIVE   = 0xffc4a35a;  // gold/orange (= NAV_ACTIVE)
    static int SLOT_INACTIVE = 0xff8b8f9a;  // muted

    // Item stock-low text.
    static int STOCK_LOW = 0xffc45050;  // CBT red

    // Preset default marker.
    static int PRESET_DEFAULT = 0xffc45050;
    static int PRESET_NORMAL  = 0xfffefefe;

    // Notification detail text (legacy dormant stack).
    static int NOTIF_SUCCESS = 0xff6b8e5a;
    static int NOTIF_ERROR   = 0xffc45050;
}
