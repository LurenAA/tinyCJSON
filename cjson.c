#include "cjson.h"

static const char* parse_value(const char* str, Cjson* out); //分析函数总入口
static const char* parse_object(const char* str, Cjson* out); //解析对象
static const char* parse_string(const char* str, Cjson* out); //解析字符串
static const char* parse_number(const char* str, Cjson* out); //分析数字
static Cjson* assign_simple_type_node(Cjson* item, 
  nodetype_t nodeType, const char * cpString); //填充null，false，true节点
static void set_nodeType(Cjson* item, nodetype_t nodeType); //设置nodeType属性
static int compute_hex(const char**); //计算utf-16的值

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
  item = assign_simple_type_node(item, nodeType, cpString);
  return item;
}

static Cjson* assign_simple_type_node(Cjson* item, nodetype_t nodeType, const char * cpString) {
  const size_t len = strlen(cpString);
  item->value.complex = (char*)cjson_malloc(len + 1);
  strncpy(item->value.complex, cpString, len);
  *(item->value.complex + len) = '\0';
  return item;
}

//解析函数入口
Cjson* cjson_parse(const char * str) {
  Cjson* out = create_new_node(NOTYPE);
  const char* end = parse_value(str, out);
  if(*end == '\0') {
    return out;
  }
  printf("error in cjson_parse method");
  exit(1);
}

//解析各种类型的值
static const char* parse_value(const char* str, Cjson* out) {
  const char *ptr = str;
  switch (*ptr)
  {
    case '{':
      set_nodeType(out, NodeType_OBJECT);
      ptr = parse_object(str, out);
      break;
    // case '[':
    //   *out = create_array_node();
    //   break;
    case 't':
    case 'T':
      set_nodeType(out, NodeType_TRUE);
      out = assign_simple_type_node(out, NodeType_TRUE, "true");
      ptr += 4;
      break;
    case 'f':
    case 'F':
      set_nodeType(out, NodeType_FALSE);
      out = assign_simple_type_node(out, NodeType_FALSE, "false");
      ptr += 5;
      break;
    case 'n':
    case 'N':
      set_nodeType(out, NodeType_NULL);
      out = assign_simple_type_node(out, NodeType_NULL, "null");
      ptr += 4;
      break;
    case '\"':
      set_nodeType(out, NodeType_STRING);
      ptr = parse_string(str, out);
      break;
    default:
      if(strchr("-+0123456789e", *ptr) == 0) {
        printf("undefined value ,error in parse_value method");
        exit(1);
      }
      set_nodeType(out, NodeType_NUMBER);
      ptr = parse_number(str, out);
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

}

static const char* parse_string(const char* str, Cjson* out) {
  const char* ptr = str;
  int len = 0;
  if(*ptr++ != '\"') {
    printf("error in parse_string method\n");
    exit(1);
  }
  while(*ptr != '\"') {
    if(*ptr == '\\') {
      ++ptr;
    }
    if(*ptr < 32) {
      continue;
    }
    if(*ptr == 'u' && *(ptr - 1) == '\\') {
      len--;
    }
    ++len;
    ++ptr;
  } //计算长度,unicode长度转为utf-8为1到4位，所以就按四位来算
  out->value.complex = (char*)cjson_malloc(len + 1); //加上\0
  char* out_ptr = out->value.complex;  
  ptr = str + 1;
  while(*ptr != '\"') {
    if(*ptr != '\\') {
      if(*ptr >= 32)
        *out_ptr++ = *ptr++; 
    } else {
      ++ptr;
      switch(*ptr) {
        case '"': 
          *out_ptr++ = '\"';
          break;
        case '\\':
          *out_ptr++ = '\\';
          break;
        case 'b': 
          *out_ptr++ = '\b';
          break;
        case 'f':
          *out_ptr++ = '\f';
          break;
        case 'n':
          *out_ptr++ = '\n';
          break;
        case 'r':
          *out_ptr++ = '\r';
          break;
        case 't':
          *out_ptr++ = '\t';
          break;
        case 'u': {
          ++ptr;
          unsigned int w1 = compute_hex(&ptr), 
            w2,
            len = 4;
          unsigned long int u = 0;
          
          if(w1 < 0xD800 || w1 > 0xDFFF) {
            u = w1;
          } else if(w1 >= 0xD800 && w1 <= 0xDBFF) {
            w2 = compute_hex(&ptr);
            if(w2 < 0xDC00 || w2 > 0xDFFF) {
              printf("error w2 in parse_string method");
              exit(1);
            }
            u = 0x10000 + (((w1 & 0x3ff) << 10) | (w2 & 0x3ff));
          } else {
            printf("error w1 in parse_string method");
            exit(1);
          }
          
          if(u >= 0 && u <= 0x00007F) {
            len = 1;
          } else if (u >= 0x000080 && u <= 0x0007FF) {
            len = 2;
          } else if (u >= 0x000800 && u <= 0x00D7FF || 
          u >= 0x00E000 && u <= 0x00FFFF) {
            len = 3;
          } else if (u >= 0x010000 && u <= 0x10FFFF) {
            len = 4;
          } else {
            printf("u is not in the BMP, error in parse_string method");
            exit(1);
          }

          switch (len)
          {
            case 1:
              *out_ptr++ = u & 0x7F;
              break;
            case 2:
            case 3:
            case 4:{
              int lenTmp = len - 1;
              out_ptr += len;
              while(lenTmp-- != 0) {
                *(--out_ptr) = u & 0x3f | 0x80;
                u = u >> 6;
              }
              *(--out_ptr) = u & 0x3f | ((len == 4) ? 0xf0 : len == 3 ? 0xe0 : 0xc0);  
              out_ptr += len;
              break;
            }
            default: 
              printf("len is wrong, error in parse_string method");
              exit(1);
          }
          break;
        }
        default: 
          *out_ptr++ = *ptr;
      }
      ++ptr;
    }
  }
  *ptr++;
  *out_ptr = '\0';
  return ptr;
}

static int compute_hex(const char** ptr) {
  int res = 0x00,
   lowChar;
  for(int i = 4; i > 0; i--) {
    if(**ptr >= '0' && **ptr <= '9') {
      res += **ptr - '0';
    } else{
      lowChar = tolower(**ptr);
      if(lowChar <= 'f' && lowChar >= 'a') {
        res += lowChar + 10 - 'a';
      } else {
        printf("utf-16 must be hex, error in compute_hex method");
        exit(1);
      }
    }
    if(i != 1){
      res *= 16;
      ++(*ptr);
    }
  }
  return res;
}

static void set_nodeType(Cjson* item, nodetype_t nodeType) {
  if(item && nodeType) {
    item->nodeType = nodeType;
  }
}

static const char* parse_number(const char* str, Cjson* out) {

}