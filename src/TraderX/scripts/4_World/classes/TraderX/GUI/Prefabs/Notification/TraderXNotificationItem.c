class TraderXNotificationItem
{
    int id;
    int notificationType;
    int creationTime;
    int duration;
    string notificationTitle;
    string content;
    bool hasDetails;
    string details;
    ref array<ref TraderXNotificationDetails> detailsList;
    
    void TraderXNotificationItem(int notificationType, int duration, string notificationTitle, string content, bool hasDetails = false, string details = "", array<ref TraderXNotificationDetails> detailsList = null)
    {
        this.id = GetGame().GetTime();
        this.notificationType = notificationType;
        this.duration = duration;
        this.notificationTitle = notificationTitle;
        this.content = content;
        this.details = details;
        this.hasDetails = hasDetails;
        this.detailsList = detailsList;
    }

    void AddDetail(bool isSuccess, string content, string status)
    {
        TraderXNotificationDetails detail = new TraderXNotificationDetails(isSuccess, content, status);
        detailsList.Insert(detail);
    }
}

class TraderXNotificationDetails
{
    bool success;
    string content;
    string status;

    void TraderXNotificationDetails(bool success, string content, string status)
    {
        this.success = success;
        this.content = content;
        this.status = status;
    }
}
