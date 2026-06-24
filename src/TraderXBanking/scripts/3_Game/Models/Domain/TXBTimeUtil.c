// TraderXBanking - Time utilities
// All timestamps are Unix epoch SECONDS (int). The old YYYYMMDDhhmmss packing (14 digits)
// overflowed int32 and produced garbage dates; epoch seconds fit int32 until 2038.

class TXBTimeUtil
{
	// Current UTC time as Unix epoch seconds.
	static int Now()
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		int s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mn, s);
		return DaysFromCivil(y, mo, d) * 86400 + h * 3600 + mn * 60 + s;
	}

	// Epoch at UTC midnight today.
	static int StartOfToday()
	{
		int y;
		int mo;
		int d;
		GetYearMonthDayUTC(y, mo, d);
		return DaysFromCivil(y, mo, d) * 86400;
	}

	// Epoch N days before now.
	static int DaysAgo(int days)
	{
		return Now() - days * 86400;
	}

	// "MM-DD HH:MM"
	static string FormatShort(int epoch)
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		Parts(epoch, y, mo, d, h, mn);
		return Pad2(mo) + "-" + Pad2(d) + " " + Pad2(h) + ":" + Pad2(mn);
	}

	// "YYYY-MM-DD HH:MM"
	static string FormatDateTime(int epoch)
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		Parts(epoch, y, mo, d, h, mn);
		return y.ToString() + "-" + Pad2(mo) + "-" + Pad2(d) + " " + Pad2(h) + ":" + Pad2(mn);
	}

	// "YYYY-MM-DD"
	static string FormatDate(int epoch)
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		Parts(epoch, y, mo, d, h, mn);
		return y.ToString() + "-" + Pad2(mo) + "-" + Pad2(d);
	}

	// Decompose epoch seconds into civil parts.
	static void Parts(int epoch, out int y, out int mo, out int d, out int h, out int mn)
	{
		int days = epoch / 86400;
		int secs = epoch - days * 86400;
		CivilFromDays(days, y, mo, d);
		h = secs / 3600;
		mn = (secs - h * 3600) / 60;
	}

	static string Pad2(int v)
	{
		if (v < 10) return "0" + v.ToString();
		return v.ToString();
	}

	// Howard Hinnant days_from_civil (proleptic Gregorian), ternary-free for Enforce.
	static int DaysFromCivil(int y, int m, int d)
	{
		if (m <= 2) y = y - 1;

		int era;
		if (y >= 0) era = y / 400;
		else era = (y - 399) / 400;

		int yoe = y - era * 400;

		int mp;
		if (m > 2) mp = m - 3;
		else mp = m + 9;

		int doy = (153 * mp + 2) / 5 + d - 1;
		int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		return era * 146097 + doe - 719468;
	}

	// Inverse: day-number (days since 1970-01-01) -> civil y/m/d.
	static void CivilFromDays(int z, out int y, out int m, out int d)
	{
		z = z + 719468;

		int era;
		if (z >= 0) era = z / 146097;
		else era = (z - 146096) / 146097;

		int doe = z - era * 146097;
		int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
		int yy = yoe + era * 400;
		int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
		int mp = (5 * doy + 2) / 153;

		d = doy - (153 * mp + 2) / 5 + 1;

		if (mp < 10) m = mp + 3;
		else m = mp - 9;

		if (m <= 2) yy = yy + 1;
		y = yy;
	}
}
