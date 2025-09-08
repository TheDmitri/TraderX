class TraderXLicense
{
    UUID licenseId;
    string licenseName;
    string description;

    void TraderXLicense(string licenseName, string description)
    {
        this.licenseId = UUID.Generate();
        this.licenseName = licenseName;
        this.description = description;
    }
}
