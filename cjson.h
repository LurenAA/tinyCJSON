#ifndef __CJSON_H_
#define __CJSON_H_

#ifdef __cplusplus
  extern "C"
  {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

//标志节点类型
typedef enum NodeType {     
  NodeType_NUMBER = 10,
  NodeType_ARRAY,
  NodeType_OBJECT,
  NodeType_STRING,
  NodeType_TRUE,
  NodeType_FALSE,
  NodeType_NULL,
  NOTYPE = 0
} nodetype_t;

//记录数据
typedef union DataValue{
  int intNum;
  double doubleNum;
  char* complex;
} datavalue_t;

//json节点类型
typedef struct _cjson{
  struct _cjson *prev, *next;
  struct _cjson *child;

  nodetype_t nodeType; //节点类型 
  datavalue_t value;  //值
  bool isReference; //是否是引用
  char *keyName; //建值
} Cjson;

typedef struct {
  void* (*malloc_fn) (size_t size);
  void (*free_fn) (void *);
} NewHook;

extern void new_hook(NewHook *hook); //初始化allocator
extern Cjson* create_simple_type_node(nodetype_t nodeType, const char * cpString); //添加除了array和object，number外其他节点的数据
extern Cjson* create_new_node(nodetype_t nodeType); //创建节点
extern Cjson* cjson_parse(const char *); //解析json函数

//创建各种类型的节点
#define create_null_node() create_simple_type_node(NodeType_NULL, "null")   //创建null节点
#define create_true_node() create_simple_type_node(NodeType_TRUE, "true")   //创建true节点
#define create_false_node()  create_simple_type_node(NodeType_FALSE, "false")  //创建false节点
#define create_object_node()  create_new_node(NodeType_OBJECT) //创建object节点
#define create_array_node() create_new_node(NodeType_ARRAY) //创建array节点
#define create_string_node() create_new_node(NodeType_STRING) //创建string节点
#define create_number_node() create_new_node(NodeType_NUMBER) //number节点

#ifdef __cplusplus
  }
#endif
#endif