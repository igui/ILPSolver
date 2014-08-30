template <class T> struct MinMaxAvg
{
public:
	T min, max, avg;
	
	static MinMaxAvg<T> forVector(std::vector<T> col)
	{
		MinMaxAvg<T> res;
		if(col.empty())
			throw std::range_error("Col is empty");
		res.min = res.max = col.at(0);
		res.avg = 0;
		for(unsigned int i = 0; i < col.size(); ++i)
		{
			res.min = std::min(res.min, col.at(i));
			res.max = std::max(res.max, col.at(i));
			res.avg += col.at(i);
		}

		res.avg /= col.size();

		return res;
	}
};

