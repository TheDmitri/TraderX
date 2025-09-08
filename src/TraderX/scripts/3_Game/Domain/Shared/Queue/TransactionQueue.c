//FIFO Queue for handling transaction such as banking transaction or item transaction
class TransactionQueue<Class T>
{
    private ref array<ref T> queue;

    void TransactionQueue()
    {
      queue = new array<ref T>();
    }

    int Count()
    {
      return queue.Count();
    }

    bool IsMaxQueue(int max)
    {
      if(queue.Count() > max)
        return true;
      
      return false;
    }

    void EnQueue(T product)
    {
      queue.Insert(product);
    }

    void DeQueue()
    {
      for(int i = 1; i < queue.Count(); i++)
      {
        queue.Set(i - 1, queue[i]);
      }

      i--;

      queue.RemoveOrdered(i);
    }

    T Peek()
    {
      return queue[0];
    }

    bool HasNextQueue()
    {
      return queue.IsValidIndex(0);
    }
}
