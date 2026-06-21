// Grid/tiles variant of the catalog item card. Same component logic as the list card
// (CatalogItemCardComponent) — including the shared static click event, so the catalog
// page's existing listener handles both — only the layout differs (vertical card matching
// the purchase item card aspect instead of the horizontal list row).
class CatalogItemGridCardComponent extends CatalogItemCardComponent
{
	private static ref ComponentAutoRegister s_RegisterGrid = new ComponentAutoRegister("tx-catalog-item-card-grid", CatalogItemGridCardComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-catalog-item-card-grid";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/CatalogItemCardGridView.layout";
	}
}
