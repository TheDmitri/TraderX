/**
 * TraderXUuid - Unique id generation (server IDs, license IDs, transaction IDs).
 * Replaces the DabsFramework UUID type/helper with vanilla UUIDApi.
 */
class TraderXUuid
{
    static string GenerateUUID()
    {
        int uid[4];
        UUIDApi.Generate(uid);
        return UUIDApi.FormatString(uid);
    }
}
