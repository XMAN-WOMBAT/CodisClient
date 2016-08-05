#include "RoundRobinCodisPool.h"
#include "Log.h"
#include "json/json.h"
#include "ScopedLock.h"
#include "etcd.hpp"

#ifndef  ETCD_MODE
#define  ETCD_MODE
#endif // ETCD_MODE


using namespace bfd::codis;

RoundRobinCodisPool::RoundRobinCodisPool(const string& zookeeperAddr, const string& proxyPath, const string& businessID)
				:m_Zh(NULL), m_ZookeeperAddr(zookeeperAddr),
				 m_ProxyPath(proxyPath),m_Mutex(PTHREAD_MUTEX_INITIALIZER),
				 proxyIndex(-1),m_BusinessID(businessID)
{
    m_Zh = zookeeper_init(zookeeperAddr.c_str(), proxy_watcher, 10000, NULL, this, 0);
    int cnt = 0;
    while (zoo_state(m_Zh)!=ZOO_CONNECTED_STATE && cnt<10000)
    {
        usleep(30000);
        cnt++;
	}
    if (cnt == 10000)
    {
        LOG(ERROR, "connect zookeeper error! zookeeperAddr="+zookeeperAddr);
        exit(1);
    }
	Init(m_Zh, proxyPath);
#endif // ZOOKEEPER_MODE

#ifdef ETCD_MODE


#endif // ETCD_MODE
}


RoundRobinCodisPool::RoundRobinCodisPool(const reborn::hostArray& coordinateAddr, const string& proxyPath, const string& businessID)
                :m_Zh(NULL), m_ZookeeperAddr(),
				 m_ProxyPath(proxyPath),m_Mutex(PTHREAD_MUTEX_INITIALIZER),
				 proxyIndex(-1),m_BusinessID(businessID),m_hosts(coordinateAddr),
                etcd_cluster_watchdog (new etcd::Watch<example::RapidReply>(coordinateAddr)),
                 etcd_client(new etcd::Client<example::RapidReply>(coordinateAddr))
{
try{

    callBack func = std::bind(&RoundRobinCodisPool::WatchProxyCallback,this,std::placeholders::_1);

    //etcd::Watch<example::RapidReply> etcd_cluster_watchdog(m_hosts);
    //cout <<"WATCH cluster RUN"<<endl;

    //etcd_cluster_watchdog->Run_watch(m_ProxyPath, func);
    example::RapidReply reply = etcd_client->Get(proxyPath);
    WatchProxyCallback(reply);

}catch(const etcd::ReplyException& e)
    {
        cout <<"happen reply exception"<<endl;
        cout <<e.what()<<endl;
    }

}




RoundRobinCodisPool::~RoundRobinCodisPool()
{
	for (size_t i=0; i<m_Proxys.size(); i++)
	{
		if (m_Proxys[i] != NULL)
		{
			delete m_Proxys[i];
			m_Proxys[i] = NULL;
		}
	}
}


void RoundRobinCodisPool::WatchProxyCallback(const example::RapidReply& reply)
{
    cout <<"WatchCallback"<<endl;
    vector<pair<string, int> > proxyInfos;
    example::RapidReply::KvPairs  t_RecvData;
    example::RapidReply* t_reply = (example::RapidReply*)&reply;
    t_reply->GetAll(t_RecvData);
    for(auto itr : t_RecvData)
    {
    //cout <<"key:"<<itr.first<<"\nvalue:"<<itr.second<<endl;
    string node_data = itr.second;
    Json::Reader reader;
    Json::Value state;
    if (!reader.parse(node_data, state, false))
		  {
			  stringstream stream;
			  stream << "data format json is faild [data:" << node_data << "]\n";
			  LOG(ERROR, stream.str());
			  continue;
		  }
		  if (state.isMember("state") && state["state"].isString() && state["state"].asString() == string("online"))
		  {
			  if (state.isMember("addr") && state["addr"].isString())
			  {
				  string ipport = state["addr"].asString();
				  vector<string> proxyinfo = split(ipport, ':');
				  if (proxyinfo.size()>1)
				  {
					  int port = atoi(proxyinfo[1].c_str());
					  proxyInfos.push_back(make_pair(proxyinfo[0], port));
					  cout <<"Proxy ip :["<<proxyinfo[0]<<"]port:["<<port<<"]"<<endl;
                }
			  }
		  }
		  else
		  {
			  stringstream stream;
			  stream << "get proxy state faild [data:" << state.toStyledString() << "]\n";
			  LOG(ERROR, stream.str());
		  }
    }
    InitProxyConns(proxyInfos);

}
CodisClient* RoundRobinCodisPool::GetProxy()
{
	int index = -1;
	ScopedLock lock(m_Mutex);
	{
		index = ++proxyIndex;
		if (proxyIndex >= m_Proxys.size())
		{
			proxyIndex = 0;
			index = 0;
		}

		if (m_Proxys.size() == 0)
		{
			index = -1;
			proxyIndex = -1;
		}
	}

	if (index == -1)
	{
		return NULL;
	}
	else
	{
		return m_Proxys[index];
	}
}

void RoundRobinCodisPool::Init(zhandle_t *(&zh), const string& proxyPath)
{
	vector<pair<string, int> > proxyInfos;

	proxyInfos = GetProxyInfos(m_Zh, proxyPath);
	if (proxyInfos.size() == 0)
	{
		LOG(ERROR, "no proxy can be used!");
		exit(1);
	}

	InitProxyConns(proxyInfos);
}

vector<pair<string, int> > RoundRobinCodisPool::GetProxyInfos(zhandle_t *(&zh), const string& proxyPath)
{
	  vector<pair<string, int> > proxys;

	  struct String_vector strings;
	  int rc0 = zoo_get_children(zh, proxyPath.c_str(),1, &strings);
	  if (rc0 != ZOK)
	  {
		  stringstream stream;
		  stream << "get children from %s faild!!!\n proxypath=" << proxyPath;
		  LOG(ERROR, stream.str());
		  exit(1);
	  }

	  char **pstr = strings.data;
	  ostringstream sentinel_addr_oss;
	  vector<string> proxyNameVec;
	  for (int i=0; i<strings.count; ++i, ++pstr)
	  {
		  Json::Reader reader;
		  Json::Value state;
		  stringstream proxyFullPath;
		  proxyFullPath << proxyPath << "/" << *pstr;
		  string node_data = ZkGet(zh, proxyFullPath.str());
		  if (!reader.parse(node_data, state, false))
		  {
			  stringstream stream;
			  stream << "data format json is faild [path: " << proxyFullPath << "][data:" << node_data << "]\n";
			  LOG(ERROR, stream.str());
			  continue;
		  }
		  if (state.isMember("state") && state["state"].isString() && state["state"].asString() == string("online"))
		  {
			  if (state.isMember("addr") && state["addr"].isString())
			  {
				  string ipport = state["addr"].asString();
				  vector<string> proxyinfo = split(ipport, ':');
				  if (proxyinfo.size()>1)
				  {
					  int port = atoi(proxyinfo[1].c_str());
					  proxys.push_back(make_pair(proxyinfo[0], port));
				  }
			  }
		  }
		  else
		  {
			  stringstream stream;
			  stream << "get proxy state faild [path: " << proxyFullPath << "][data:" << state.toStyledString() << "]\n";
			  LOG(ERROR, stream.str());
		  }
	  }

	  return proxys;
}

void RoundRobinCodisPool::InitProxyConns(vector<pair<string, int> >& proxyInfos)
{
	vector<CodisClient*> proxys;
	for (size_t i=0; i<proxyInfos.size(); i++)
	{
		CodisClient *proxy = new CodisClient(proxyInfos[i].first, proxyInfos[i].second, m_BusinessID);
		proxys.push_back(proxy);
	}

	ScopedLock lock(m_Mutex);
	{
		m_Proxys.swap(proxys);
		m_ProxyInfos = proxyInfos;
	}

	for (size_t i=0; i<proxys.size(); i++)
	{
		if (proxys[i] != NULL)
		{
			delete proxys[i];
			proxys[i] = NULL;
		}
	}
}

string RoundRobinCodisPool::ZkGet(zhandle_t *(&zh), const string &path, bool watch)
{
	char * buffer = NULL;
	Stat stat;
	int buf_len = 0;
	int rc;
	rc = zoo_get(zh, path.c_str(), 0, NULL, &buf_len, &stat);
	if (rc != ZOK)
	{
		LOG(ERROR, string("getting zk node stats failed:")+zerror(rc));
		return "";
	}
	buffer = (char*)malloc(sizeof(char) * (stat.dataLength+1));
	buf_len = stat.dataLength + 1;

	rc = zoo_get(zh, path.c_str(), watch, buffer, &buf_len, NULL);
	if (rc != ZOK)
	{
		LOG(ERROR, string("getting zk node stats failed:")+zerror(rc));
		if (buffer != NULL)
		{
			free(buffer);
			buffer = NULL;
		}
		return "";
	}
	string return_str = buffer;
	if (buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}
	return return_str;
}

void RoundRobinCodisPool::proxy_watcher(zhandle_t *zh, int type, int state, const char *path, void *context)
{
	RoundRobinCodisPool* ptr = reinterpret_cast<RoundRobinCodisPool*>(context);

	if ((type==ZOO_SESSION_EVENT) && (state==ZOO_CONNECTING_STATE))
	{
		zookeeper_close(ptr->m_Zh);
		ptr->m_Zh = zookeeper_init(ptr->m_ZookeeperAddr.c_str(), proxy_watcher, 10000, NULL, context, 0);
		if (ptr->m_Zh == NULL)
		{
			LOG(ERROR, "connect zookeeper error! zookeeperAddr="+ptr->m_ZookeeperAddr);
			exit(1);
		}
	}
	else if ((type==ZOO_SESSION_EVENT) && (state==ZOO_EXPIRED_SESSION_STATE))
	{
		zookeeper_close(ptr->m_Zh);
		ptr->m_Zh = zookeeper_init(ptr->m_ZookeeperAddr.c_str(), proxy_watcher, 10000, NULL, context, 0);
		if (ptr->m_Zh == NULL)
		{
			LOG(ERROR, "connect zookeeper error! zookeeperAddr="+ptr->m_ZookeeperAddr);
			exit(1);
		}
	}
	else if ((type==ZOO_SESSION_EVENT) && (state==ZOO_CONNECTED_STATE))
	{

	}
	else if ((state==ZOO_CONNECTED_STATE) && (type==ZOO_CHANGED_EVENT))
	{
		ptr->Init(ptr->m_Zh, ptr->m_ProxyPath);
	}
	else if ((state==ZOO_CONNECTED_STATE) && (type==ZOO_DELETED_EVENT))
	{
		ptr->Init(ptr->m_Zh, ptr->m_ProxyPath);
	}
	else if ((state==ZOO_CONNECTED_STATE) && (type==ZOO_CHILD_EVENT))
	{
		ptr->Init(ptr->m_Zh, ptr->m_ProxyPath);
	}
	else if ((state==ZOO_CONNECTED_STATE) && (type==ZOO_CREATED_EVENT))
	{
		ptr->Init(ptr->m_Zh, ptr->m_ProxyPath);
	}
	else
	{
		LOG(ERROR, "zookeeper connection state changed but not implemented");
	}

}
