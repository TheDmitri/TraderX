// UI element related enums

enum EButtonIds
{
  NAVIGATION = 232,
  PURCHASE = 299,
  FAVORITE = 300,
  SELECTED = 301
}

enum ENotificationType
{
  INFO,
  WARNING,
  ERROR,
  SUCCESS
}

// TODO: Consider moving this to ViewStyles.c or consolidating all color definitions
class TradePlusStyle
{
  static int PRIMARY_COLOR;
  static int SECONDARY_COLOR;
  static int WARNING_COLOR = 0xD44020;
}
