/*
Don't judge me on this but I spent far too much time trying to copy a map
without loosing data from the original one. This method works, so I'm using it !
*/
class TraderXCopy<Class T>
{
	static void CopyMap(T dataIn, out T dataOut)
	{
		string data = TraderXJsonLoader<T>.ObjectToString(dataIn);
		TraderXJsonLoader<T>.StringToObject(data, dataOut);
	}
}
