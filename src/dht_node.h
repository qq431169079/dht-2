#ifndef DHT_NODE_H_
#define DHT_NODE_H_

typedef struct {
  char[160] id;
  time_t created_at;
  time_t last_heard;
} dht_node_t;

dht_node_t *
dht_node_new();

#endif