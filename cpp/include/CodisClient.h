/**
 * @file
 * @brief
 *
 */

#ifndef CODIS_CLIENT_H
#define CODIS_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include "Reply.h"
#include "Command.h"
#include "RedisClientPool.h"
#include <sys/time.h>

using namespace std;

namespace bfd
{
namespace codis
{

typedef map<string, string> KVMap;


class CodisClient;
/**
 * @brief mget3
 */
class MgetAsyncRequestContext
{
	// mget过程中，对每个server发起的请求的回调变量
public:
	vector<string> user_keylist_;
	void (*callback)(KVMap& kvs);
	redisAsyncContext** async_context;
	CodisClient *client;
};

class CodisClient
{
public:
	  CodisClient(const string& proxyIP, const int port, const string& businessID);
	  ~CodisClient();
	  void returnAsync(redisAsyncContext* async_context) { m_ConnPool->returnItemAsync(async_context);};

	  /**
	   * @brief key
	   */
	  bool exists(string key, int tt = 0);
	  int del(string key, int tt = 0);
	  int del(vector<string>& keys, int tt = 0);
	  string type(string key, int tt = 0);
	  bool expire(string key, int seconds, int tt = 0);

	  /**
	   * @brief string
	   */
	  bool set(string key, string value, int tt = 0);
	  bool setnx(string key, string value, int tt = 0);
	  bool setex(string key, string value, int seconds, int tt = 0);
	  string get(string key, int tt = 0);
	  string getset(string key, string value, int tt = 0);
	  int setbit(string key, int index, bool value, int tt = 0);
	  int getbit(string key, int index, int tt = 0);
	  int bitcount(string key, int tt = 0);

	  /**
	   * @brief 同步
	   */
	  vector<string> mget(vector<string>& keys, int tt = 0);

	  /**
	   * @brief 全异步, 不支持多线程调用.异步执行命令,等所有命令都返回结果后, kvs.finish=true
	   */
	  bool mget2(vector<string>& keys, void (*callback)(KVMap& kvs));

	  bool mset(map<string, string>& keyvalues, int tt = 0);
	  int incr(string key, int tt = 0);
	  int decr(string key, int tt = 0);
	  int incrby(string key, int incr, int tt = 0);
	  int decrby(string key, int incr, int tt = 0);
	  long append(string key, string value, int tt = 0);

	  /**
	   * @brief list
	   */
	  int lpush(string key, string value, int tt = 0);
	  int rpush(string key, string value, int tt = 0);
	  int lpush(string key, vector<string> values, int tt = 0);
	  int rpush(string key, vector<string> values, int tt = 0);
	  int llen(string key, int tt = 0);
	  vector<string> lrange(string key, int start, int end, int tt = 0);
	  bool ltrim(string key, int start, int end, int tt = 0);
	  bool lset(string key, int index, string value, int tt = 0);
	  bool lrem(string key, int count, string value, int tt = 0);
	  string lpop(string key, int tt = 0);
	  string rpop(string key, int tt = 0);

	  /**
	   * @brief set
	   */
	  bool sadd(string key, string member, int tt = 0);
	  int sadd(string key, vector<string> members, int tt = 0);
	  bool srem(string key, string member, int tt = 0);
	  string spop(string key, int tt = 0);
	  string srandmember(string key, int tt = 0);
	  int scard(string key, int tt = 0);
	  bool sismember(string key, string member, int tt = 0);
	  vector<string> smembers(string key, int tt = 0);

	  /**
	   * @brief sorted set
	   */
	  bool zadd(string key, int score, string member, int tt = 0);
	  bool zrem(string key, string member, int tt = 0);
	  int zincrby(string key, int incr, string member, int tt = 0);
	  int zrank(string key, string member, int tt = 0);
	  int zrevrank(string key, string member, int tt = 0);
	  vector<string> zrange(string key, int start, int end, string withscores="False", int tt = 0);
	  vector<string> zrevrange(string key, int start, int end, string withscores="False", int tt = 0);
	  vector<string> zrangebyscore(string key, string min, string max, string withscores="False", int tt = 0);
	  vector<string> zrevrangebyscore(string key, string min, string max, string withscores="False", int tt = 0);
	  int zcount(string key, int min, int max, int tt = 0);
	  int zcard(string key, int tt = 0);
	  int zscore(string key, string member, int tt = 0);
	  int zremrangebyrank(string key, int min, int max, int tt = 0);
	  int zremrangebyscore(string key, int min, int max, int tt = 0);

	  /**
	   * @brief hash
	   */
	  bool hset(string key, string field, string value, int tt = 0);
	  string hget(string key, string field, int tt = 0);
	  vector<string> hmget(string key, vector<string>& field, int tt = 0);
	  bool hmset(string key, vector<string>& fields, vector<string>& values, int tt = 0);
	  int hincrby(string key, string field, int incr, int tt = 0);
	  bool hexists(string key, string field, int tt = 0);
	  bool hdel(string key, string field, int tt = 0);
	  int hlen(string key, int tt = 0);
	  vector<string> hkeys(string key, int tt = 0);
	  vector<string> hvals(string key, int tt = 0);
	  bool hgetall(string key, vector<string>& fields, vector<string>& values, int tt = 0);

	  /**
	   * @brief command
	   */
	  Reply RedisCommand(const vector<string>& command, int tt = 0);
	  Reply RedisCommand(Command& command, int tt = 0);
	  vector<Reply> RedisCommands(vector<Command>& commands);
          class myexception: public exception
          {
              virtual const char* what() const throw()
              {
                  return "Your value is too bigger than 1M";
              }
          } myex;

private:
	  aeEventLoop *m_Loop;
	  RedisClientPool *m_ConnPool;
	  string m_BID;
      string proxy_IP;
      string proxy_Port;

private:
	  static void* AEThread(void *arg);
	  static void mget2Callback(redisAsyncContext *c, void *r, void *privdata);
};

}
}

#endif
