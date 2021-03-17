//
// Created by Mingkai Chen on 2017-04-10.
//

#include <string>
#include <iostream>

class foo {
public:
	void set (std::string s, bool left);
	void print (void);

private:
	int u;
	std::string left_;
	std::string right_;
};

void foo::set (std::string s, bool left)
{
	try
	{
		if (u) throw std::exception();
		else u = 0;
	}
	catch (std::exception)
	{
		std::cout << "weee\n";
	}

	if (left)
	{
		left_ = s;
	}
	else
	{
		right_ = s;
	}
	switch (s[0])
	{
		case 'a':
			u = -1;
		case 'b':
			u = 0;
		case 'c':
			u = 92;
		case 'd':
			u = 1;
		default:
			u = 2;
	}

	if (left)
	{
		u += 12;
	}
	else
	{
		u -= 12;
	}
}

void foo::print (void)
{
	std::cout << left_ << "|" << right_ << std::endl;
}