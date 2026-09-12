// Helper for deciding whether a category's configured `icon` value is something the UI can
// actually load as an image. In configs the field is very often an emoji (📦/💚/⚙️/🛠️) or an
// empty string — neither is a texture and LoadImageFile fails on them (blank/orange square).
// Only a real texture path (.edds/.paa/.tga) should be loaded; everything else uses a default.
class TraderXCategoryIconUtils
{
	static bool IsIconFilePath(string icon)
	{
		if (icon == string.Empty)
			return false;

		return icon.Contains(".edds") || icon.Contains(".paa") || icon.Contains(".tga") || icon.Contains(".EDDS") || icon.Contains(".PAA") || icon.Contains(".TGA");
	}
}
