class TraderXObject
{
  	string className;
	vector position, orientation;

	void TraderXObject(string className, vector position, vector orientation)
	{
		this.className  = className;
		this.position   = position;
		this.orientation= orientation;
	}
}
