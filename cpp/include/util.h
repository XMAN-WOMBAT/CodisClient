#ifndef ETCD_CLIENT_POOL
#define ETCD_CLIENT_POOL
#include <string>
#include <vector>
#define  MAXRETRY 3
namespace reborn
{

	class host{
	public:
		explicit host(const std::string& ip, const std::string&port);
        explicit host();
		~host();
		std::string getIp()const;
		std::string getPort()const;
		std::string getAddr()const;
	private:
		std::string m_ip;
		std::string m_port;
		std::string m_addr;
	};
	class hostArray
	{
	public:
		explicit hostArray();
		~hostArray();
		bool addHost(const host&);
		bool addHost(const std::string& ip, const std::string& port);

		bool deleHost(const std::string& ip, const std::string &port);
		bool deleHost(const std::string&addr);
		bool deleHost(const host& h);

		bool getHost(const std::string& ip, const std::string& port, host& h)const;
		bool getHost(const std::string& addr, host& h)const;

		size_t size();

		host&  operator[](int index);
	protected:
		std::vector<host>	m_Hosts;
	private:
		size_t m_CurrentIndex = 0;
	};
}

#endif
