// Lightweight object wrapper for a player-slot nav button id.
// DayZular's ForEachBinding boxes each item as a Class and assigns it via
// reflection, so it only supports arrays of OBJECTS — an array<int> silently
// fails to bind (the int can't be boxed into a Class out-param), leaving the
// generated slot buttons with navBtnId == 0. Wrapping the id in a ref class
// makes the foreach work like every other TraderX list.
class TraderXNavSlotData
{
	int navBtnId;

	void TraderXNavSlotData(int tNavBtnId)
	{
		navBtnId = tNavBtnId;
	}
}
