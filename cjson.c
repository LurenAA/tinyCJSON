#include "cjson.h"

static const char* parse_value(const char* str, Cjson** out); //分析函数总入口
static const char* parse_object(const char* str, Cjson* out); //解析对象
static const char* parse_string(const char* str, char* res, int len); //解析字符串
static void* (*cjson_malloc) (size_t size) = malloc;  //malloc
static void (*cjson_free) (void *ptr) = free;  //free

//更改allocator
void new_hook(NewHook *hook) {
  if(!hook) {
    cjson_malloc = malloc;
    cjson_free = free;
    return ;
  }
  hook->free_fn && (cjson_free = hook->free_fn);
  hook->malloc_fn && (cjson_malloc = hook->malloc_fn);
}

//创建新节点
Cjson* create_new_node(nodetype_t nodeType) {
  static const size_t Cjson_size = sizeof(Cjson);
  Cjson* item = (Cjson*)cjson_malloc(Cjson_size);
  if(!item) {
    printf("error in create_new_node method\n");
    exit(1);
  }  
  memset(item, 0, Cjson_size);
  if(nodeType) 
    item->nodeType = nodeType;
  item->isReference = false;
  return item;
}

//创建新节点并且赋值，适用于object，array，number以外的类型
Cjson* create_simple_type_node(nodetype_t nodeType, const char * cpString) {
  Cjson* item = create_new_node(nodeType);
  const size_t len = strlen(cpString);
  item->value.complex = (char*)cjson_malloc(len + 1);
  strncpy(item->value.complex, cpString, len);
  *(item->value.complex + len) = '\0';
  return item;
}

//解析函数入口
Cjson* cjson_parse(const char * str) {
  Cjson** out;
  const char* end = parse_value(str, out);
  if(*end == '\0') {
    return *out;
  }
  printf("error in cjson_parse method");
  exit(1);
}

static const char* parse_value(const char* str, Cjson** out) {
  const char *ptr = str;
  switch (*ptr)
  {
    case '{':
      ptr = parse_object(str, *out);
      break;
    case '[':
      *out = create_array_node();
      break;
    case 't':
      *out = create_true_node();
      break;
    case 'f':
      *out = create_false_node();
      break;
    case 'n':
      *out = create_null_node();
      break;
    case '\"':
      *out = create_string_node();
      break;
    default:
      *out = create_number_node();
      break;
  }
  return ptr;
} 

static const char* parse_object(const char* str, Cjson* out) {
  const char *ptr = str ;
  if(*ptr != '{') {
    printf("error in parse_object method\n");
    exit(1);
  }
  ptr++;
  out = create_object_node();

}

static const char* parse_string(const char* str, char * res, int len) {

}