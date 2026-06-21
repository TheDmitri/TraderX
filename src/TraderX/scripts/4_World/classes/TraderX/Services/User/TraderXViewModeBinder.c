// Helper so any list-owning component reacts to view-mode changes without
// duplicating the show/hide logic. Owner declares two container widgets
// (list rows + grid tiles) and forwards to Apply().
class TraderXViewModeBinder
{
    // Show the container matching the current global view mode, hide the other.
    static void Apply(Widget listContainer, Widget gridContainer)
    {
        bool grid = TraderXViewModeService.GetInstance().IsGridView();
        if (listContainer) listContainer.Show(!grid);
        if (gridContainer) gridContainer.Show(grid);
    }
}
