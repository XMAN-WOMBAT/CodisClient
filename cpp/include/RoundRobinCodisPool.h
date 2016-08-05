/**
 * @file
 * @brief
 */

#ifndef CODIS_CLIENT_ROUNDROBINCODISPOOL_H
#define CODIS_CLIENT_ROUNDROBINCODISPOOL_H

#include "CodisClient.h"
#include <vector>
#include <zookeeper.h>
#include "etcd.hpp"
#include "util.h"
#include "rapid_reply.hpp"
#include <memory>
using namespace std;

namespace bfd
{
namespace codis
{

class RoundRobinCodisPool
{
public:
	RoundRobinCodisPool(const string& coordinateAddr, const string& proxyPath, const string& businessID);
	RoundRobinCodisPool(const reborn::hostArray& coordinateAddr, const string& proxyPath, const string& businessID);
	~RoundRobinCodisPool();

	CodisClient* GetProxy();
private:
	vector<CodisClient*> m_Proxys;
	int proxyIndex;
	vector<pair<string, int> > m_ProxyInfos;
	zhandle_t *m_Zh;
	string m_ZookeeperAddr;
	string m_ProxyPath;
	string m_BusinessID;
	pthread_mutex_t m_Mutex;
	reborn::hostArray m_hosts;
private:
	void Init(zhandle_t *(&zh), const string& proxyPath);
	static void proxy_watcher(zhandle_t *zh, int type, int state, const char *path, void *context);
	string ZkGet(zhandle_t *(&zh), const string &path, bool watch=true);
	vector<pair<string, int> > GetProxyInfos(zhandle_t *(&zh), const string& proxyPath);
	void InitProxyConns(vector<pair<string, int> >& proxyInfos);
	void WatchProxyCallback(const example::RapidReply& reply);

	typedef std::function<void(const example::RapidReply& reply)> callBack;
	std::unique_ptr<etcd::Watch<example::RapidReply>> etcd_cluster_watchdog;
    std::unique_ptr<etcd::Client<example::RapidReply>> etcd_client;
};

}
}
#endif
