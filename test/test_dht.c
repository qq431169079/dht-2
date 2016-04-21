#include "../dht.c"
#include "tap.h"
#include <arpa/inet.h>

static void
test_init(){
  dht_t *dht = dht_new(9999);
  ok(dht != NULL, "dht allocated correctly");
  dht_close(dht);
}

static int ran = 0;
static int lkp = 0;
static int err = 0;
static uint8_t key[DHT_HASH_SIZE] = {0};
static uint8_t data[] = "hello";

static ssize_t
lookup(const uint8_t skey[DHT_HASH_SIZE], void **sdata) {
  ok(crypto_verify_32(skey, key) == 0, "key matches");
  *sdata = data;
  lkp++;
  return sizeof(data);
}

static int
store(const uint8_t skey[DHT_HASH_SIZE], const void *sdata, const size_t length){
  ok(length == sizeof(data), "length matches in store");
  ok(memcmp(sdata, data, length) == 0, "length matches in store");
  ok(crypto_verify_32(skey, key) == 0, "key matches in store");
  return sizeof(data);
}

static void
success(void *closure, uint8_t key[DHT_HASH_SIZE], uint8_t *data, size_t length) {
  (void) closure, (void) key, (void) data, (void) length;
  ok(true, "found a key");
  ran++;
}

static void
error(void *closure) {
  (void)closure;
  ok(false, "error finding something");
  err++;
}

static void
test_set_get() {
  dht_t *dht  = dht_new(9999);
  dht_t *dht2 = dht_new(10000);
  ok(dht != NULL, "dht allocated correctly");
  ok(dht2 != NULL, "multiple dhts per process");
  int ret = blake2(key, data, NULL, DHT_HASH_SIZE, sizeof(data), 0);
  ok(ret == 0, "hashed 'hello' correctly");

  struct sockaddr_storage addr = {0};
  socklen_t slen = sizeof(addr);
  ret = getsockname(dht2->socket,(struct sockaddr *)&addr, &slen);
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(((struct sockaddr_in *)&addr)->sin_addr), str, INET_ADDRSTRLEN);  
  ok(ret == 0, "parsed address correctly");
  dht_add_node(dht, dht2->id, &addr);
  dht_set_storage(dht2, store, lookup);
  ret = dht_set(dht, data, sizeof(data), success, error, NULL);
  if(ret == -1) {
    printf("%s\n", strerror(errno));
    fflush(stdout);
  }
  ok(ret >= 0, "sent a set request");
  ret = dht_run(dht2, 100);
  ok(ret == 0, "dht2 received a set request");
  ret = dht_run(dht, 100);
  ok(ret == 0, "dht received set response");  
  dht_get(dht, key, success, error, NULL);
  ret = dht_run(dht2, 100);
  ok(ret == 0, "dht2 received a get request");
  ret = dht_run(dht, 100);
  ok(ret == 0, "dht received get response");  

  dht_close(dht);
  dht_close(dht2);
  ok(ran == 1, "got a value");
  ok(lkp == 1, "stored the value");
  ok(err == 0, "couldn't find a value");
}

int
main(){
  start_test;
  test_init();
  test_set_get();
}
