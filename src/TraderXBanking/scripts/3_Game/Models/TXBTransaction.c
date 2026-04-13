// TraderXBanking - Transaction Model & Queue
// Represents a pending banking transaction and FIFO queue for sequential processing

class TXBTransaction
{
	PlayerBase player;
	int amount;
	int type;
	string destinationUID;

	void TXBTransaction(PlayerBase _player, int _amount, int _type, string _destUID = "")
	{
		player = _player;
		amount = _amount;
		type = _type;
		destinationUID = _destUID;
	}
}

class TXBTransactionQueue
{
	protected ref array<ref TXBTransaction> m_Queue = new array<ref TXBTransaction>();

	int Count()
	{
		return m_Queue.Count();
	}

	bool HasNext()
	{
		return m_Queue.Count() > 0;
	}

	void Enqueue(TXBTransaction transaction)
	{
		m_Queue.Insert(transaction);
	}

	TXBTransaction Peek()
	{
		if (m_Queue.Count() > 0)
			return m_Queue[0];
		return null;
	}

	void Dequeue()
	{
		if (m_Queue.Count() > 0)
			m_Queue.RemoveOrdered(0);
	}
}
