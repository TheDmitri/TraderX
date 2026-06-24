/**
 * TraderX cross-mod integration DTOs.
 * Vanilla-only fields so external mods serialize/inspect without a hard dep
 * on any TraderX module internal type.
 */
class TraderXGarageVehicleInfo
{
    string vehicleId;
    string className;
    string displayName;
    int    healthLevel;   // 0..4 (PRISTINE..RUINED)
}
