#include "util.h"

namespace reborn
{

host::host()
{

}
host::host(const std::string& ip, const std::string&port)
	:m_ip(ip), m_port(port)
{
	if (ip.empty() || port.empty())
	{
		return;
	}

	m_addr = ip + ":" + port;
}

host::~host()
{

}

std::string host::getIp()const
{
	return m_ip;
}

std::string host::getPort()const
{
	return m_port;
}

std::string host::getAddr()const
{
	return m_addr;
}

hostArray::hostArray()
{

}

hostArray::~hostArray()
{

}

bool hostArray::addHost(const host & h)
{
	for (auto hItr : m_Hosts)
	{
		if (hItr.getAddr().compare( h.getAddr()) == 0)
		{
			return false;
		}
	}
	m_Hosts.push_back(h);

	return true;
}

bool hostArray::addHost(const std::string& ip, const std::string& port)
{
	for (auto hItr : m_Hosts)
	{
		if (hItr.getIp().compare(ip) == 0 && hItr.getPort().compare(port) == 0)
		{
			return false;
		}
	}
	host t_host(ip,port);
	m_Hosts.push_back(t_host);
	return true;
}


bool hostArray::deleHost(const std::string& ip, const std::string &port)
{
	std::vector<host>::iterator hItr = m_Hosts.begin();
	for (; hItr != m_Hosts.end(); )
	{
		if (hItr->getIp().compare(ip) == 0 && hItr->getPort().compare(port) == 0)
		{
			m_Hosts.erase(hItr++);
			return true;
		}
		else
		{
			hItr++;
		}
	}

		return false;
}

bool hostArray::deleHost(const std::string&addr)
{
	std::vector<host>::iterator hItr = m_Hosts.begin();
	for (; hItr != m_Hosts.end();)
	{
		if (hItr->getAddr().compare(addr) == 0 )
		{
			m_Hosts.erase(hItr++);
			return true;
		}
		else
		{
			hItr++;
		}
	}

		return false;
}

bool hostArray::deleHost(const host& h)
{
	std::vector<host>::const_iterator hItr = m_Hosts.begin();
	for (; hItr != m_Hosts.end();)
	{
		if (hItr->getAddr().compare(h.getAddr()) == 0)
		{
			//m_Hosts.erase(hItr++);
			return true;
		}
		else
		{
			hItr++;
		}
	}

		return false;
}

bool hostArray::getHost(const std::string& ip, const std::string& port, host& h)const
{
	std::vector<host>::const_iterator hItr = m_Hosts.begin();
	for (; hItr != m_Hosts.end();)
	{
		if (hItr->getIp().compare(ip) == 0 && hItr->getPort().compare(port) == 0)
		{
			h = *hItr;
			return  true;
		}
		else
		{
			hItr++;
		}
	}


		return false;

}

bool hostArray::getHost(const std::string& addr, host& h)const
{
	std::vector<host>::const_iterator hItr = m_Hosts.begin();
	for (; hItr != m_Hosts.end();)
	{
		if (hItr->getAddr().compare(addr) == 0)
		{
			h = *hItr;
			return true;
		}
		else
		{
			hItr++;
		}
	}

		return false;

}

size_t hostArray::size()
{
	return m_Hosts.size();
}

host& hostArray::operator[](int index)
{
	return m_Hosts[index];
}

}
