const string Safe_Zone_Root_Server = "$profile:TraderX/";
// Self-contained mod folder (was core's TraderXConfig/ pre-relocation).
const string Safe_Zone_Config_Dir_Server = Safe_Zone_Root_Server + "TraderXSafeZone/";
const string Safe_Zone_Config_Filename = Safe_Zone_Config_Dir_Server + "TXSafeZoneSettings.json";
const string Safe_Zone_Log_Dir = Safe_Zone_Config_Dir_Server + "Logs/";
const string Safe_Zone_Log_File = Safe_Zone_Log_Dir + "TXSZ_%1.log";
// Legacy path (pre-relocation) — auto-migrated on load, then deleted.
const string Safe_Zone_Legacy_Config_Filename = Safe_Zone_Root_Server + "TraderXConfig/TraderXTXSafeZoneSettings.json";
