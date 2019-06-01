#include "cjson.h"

static const char* parse_value(const char* str, Cjson* out); //分析函数总入口
static const char* parse_object(const char* str, Cjson* out); //解析对象
static const char* parse_string(const char* str, Cjson* out); //解析字符串
static const char* parse_number(const char* str, Cjson* out); //分析数字
static const char* parse_array(const char* str, Cjson* out); //解析数组
static Cjson* assign_simple_type_node(Cjson* item, 
  nodetype_t nodeType, const char * cpString); //填充null，false，true节点
static void set_nodeType(Cjson* item, nodetype_t nodeType); //设置nodeType属性
static int compute_hex(const char**); //计算utf-16的值，计算前导代理和后尾代理
static const char* skip_space(const char* str); // 跳过空白格
static char* cjson_strcopy(const char* src); //复制字符串节点
static const char* print_simple_node(const Cjson* out); //输出简单节点
static const char* print_string(const Cjson* out); //输出string节点
static const char* print_unicode(const char* str, char* des); //输出unicode
static const char* print_number(const Cjson* out); //输出数字的json
static const char* print_array(const Cjson* out); //输出array的json
static const char* print_object(const Cjson* out); //输出object的json
#define print_null(out) print_simple_node(out)   //输出null节点
#define print_true(out) print_simple_node(out)//输出true节点
#define print_false(out) print_simple_node(out)//输出false节点

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

//填充null，false，true节点
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
  return out;
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
    case '[':
      set_nodeType(out, NodeType_ARRAY);
      ptr = parse_array(str, out);
      break;
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

//解析字符串
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
        case '/':
          *out_ptr++ = '/';
          break;
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
        case 'u': { //unicode
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

//计算前导代理和后尾代理
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

//设置nodeType
static void set_nodeType(Cjson* item, nodetype_t nodeType) {
  if(item && nodeType) {
    item->nodeType = nodeType;
  }
}

//解析数字
static const char* parse_number(const char* str, Cjson* out) {
  const char* ptr = str;
  char* end;
  double num;
  num = strtod(str, &end);
  if(num - 0 < DBL_EPSILON && end == str) {
    printf("str is not a number, error in parse_number function");
    exit(1);
  }
  ptr = end;
  if(fabs(num - (int)num) < DBL_EPSILON) {
    out->value.intNum = num;
    out->isInt = true;
  } else {
    out->value.doubleNum = num;
    out->isInt = false;
  }
  return ptr;
}

//解析对象
static const char* parse_object(const char* str, Cjson* out) {
  const char *ptr = str;
  Cjson* cur = out;
  bool firstContentKey = true; //标识对象的第一个属性
  if(*ptr++ != '{') {
    printf("error in parse_object method\n");
    exit(1);
  }
  if(*ptr == '}') {
    return ++ptr;
  }

  while(firstContentKey || *ptr == ',') {
    *ptr == ',' && ++ptr;
    Cjson *child = create_new_node(NOTYPE);
    if(firstContentKey) {
      cur->child = child;
      firstContentKey = false;
    } else {
      cur = add_next(cur, child);
    }
    
    ptr = skip_space(ptr);
    if(*ptr != '\"') {
      printf("object need name, error in parse_object");
      exit(1);
    }
    ptr = parse_value(ptr, child);
    if(child->nodeType != NodeType_STRING) {
      printf("keyNmae must be string type");
      exit(1);
    }
    child->keyName = child->value.complex;
    child->value.complex = NULL;
    ptr = skip_space(ptr);
    if(*ptr++ != ':') {
      printf("object need : after keyName, error in parse_object");
      exit(1);
    }
    ptr = skip_space(ptr);
    ptr = parse_value(ptr, child);
    ptr = skip_space(ptr);
    cur = child;
  }
  
  if(*ptr++ != '}') {
    printf("end object must be a }, error in parse_object");
    exit(1);
  }
  return ptr;
}

//跳过空白
static const char* skip_space(const char* str) {
  while(str && (*str < 32 || *str == ' ')) {
    str++;
  }
  return str;
}

//添加下一个节点
Cjson* add_next(Cjson* cur, Cjson* next) {
  if(!cur) {
    printf("can not add next node without current one, error in add_next function");
    exit(1);
  }
  if(cur->next) {
    next->next = cur->next;
    cur->prev = next;
  }
  cur->next = next;
  next->prev = cur;
  return cur;
}

//解析数组
static const char* parse_array(const char* str, Cjson* out) {
  const char* ptr = str;
  bool firstArrayItemFlag = true; //标志第一个数组对象，因为第一个开头不是，
  if(!out) {
    printf("out cant not be a NULL pointer, error in parse_array method");
    exit(1);
  }
  Cjson* cur = out;
  while(firstArrayItemFlag || *ptr == ',') {
    ++ptr;
    Cjson* newOne = create_new_node(NOTYPE);
    if(firstArrayItemFlag) {
      firstArrayItemFlag = false;
      cur->child = newOne;
    } else {
      cur = add_next(cur, newOne);
    }
    cur = newOne;
    ptr = skip_space(ptr);  
    ptr = parse_value(ptr, cur);
    ptr = skip_space(ptr);
  }

  if(*ptr++ != ']') {
    printf("array must end with a ], error in parse_array method \n");
    exit(1);
  }
  return ptr;
}

 //删除Cjson对象
Cjson* deleteCjson(Cjson* out) {
  if(out->child) {
    deleteCjson(out->child);
  }
  if(out->keyName) 
   cjson_free(out->keyName);
  if(out->nodeType != NodeType_NUMBER) {
    cjson_free(out->value.complex);
  }
  Cjson* next = out->next;
  cjson_free(out);
  if(next)
    deleteCjson(next);
}

//输出json节点总入口
const char* print_json(const Cjson* out) {
  const char* res = NULL;
  switch (out->nodeType)
  {
    case NodeType_NULL:
      res = print_null(out);
      break;
    case NodeType_FALSE:
      res = print_false(out);
      break;
    case  NodeType_TRUE:
      res = print_true(out);
      break;
    case NodeType_STRING:
      res = print_string(out);
      break;
    case NodeType_NUMBER:
      res = print_number(out);
      break;
    case NodeType_ARRAY:
      res = print_array(out);
      break;
    case NodeType_OBJECT:
      res = print_object(out);
      break;
    default:
      break;
  }
  return res;
}

//输出简单节点
static const char* print_simple_node(const Cjson* out) {
  static nodetype_t allowTypes[] = { NodeType_TRUE, NodeType_FALSE, NodeType_NULL};
  bool nodeTypeFlag = false;
  for(int i = 0; i < 3; i++) {
    if(out->nodeType == allowTypes[i]) {
      nodeTypeFlag = true;
    }
  }
  if(nodeTypeFlag == false) {
    printf("type error in print_simple_node method\n");
    exit(1);
  }
  char* res;
  res = cjson_strcopy(out->value.complex);
  return res;
}

//复制字符串
static char* cjson_strcopy(const char* src){
  if(!src) {
    printf("error in cjson strcopy, src need be a string\n");
    exit(1);
  }
  int len = strlen(src);
  char* res = (char*)malloc(len);
  res = strcpy(res, src);
  return res;
}

//输出string节点
static const char* print_string(const Cjson* out) {
  if(out->nodeType != NodeType_STRING) {
    printf("type error in print_String\n");
    exit(1);
  }
  const char* ptr = out->value.complex;
  int len = 0, curLen = 80; //curlen记录现在res长度
  char* res; 
  char* resStart = (char*)cjson_malloc(curLen);
  res = resStart;
  memset(res, 0, curLen);
  *res++ = '\"';
  len++;
  while(*ptr != '\0') {
    if(*ptr >= 32 && *ptr != '\"' && *ptr != '\\') {
      *res++ = *ptr++;
      len++;
    } else {
      len += 2;
      *res++ = '\\';
      switch(*ptr) {
        case '\"':
        case '/':
        case '\\':
          *res++ = *ptr;
          break;
        case '\b':
          *res++ = 'b';
          break;
        case '\f':
          *res++ = 'f';
          break;
        case '\n':
          *res++ = 'n';
          break;
        case '\r':
          *res++ = 'r';
          break;
        case '\t':
          *res++ = 't';
          break;
        default:
          *res++ = 'u';
          ptr = print_unicode(ptr, res);    //将utf-8转化为utf-16
          --ptr;
          while(*res >= 48 && *res <= 57 ||  //跳过unicode的几个字符
          *res >= 65 && *res <= 70 ||
          *res >= 97 && *res <= 102) {
            res++;
            len++;
          }
      }
      ++ptr;
    }
    if(len >= curLen) {
      while(curLen <= len)
        curLen *= 2;
      char* tmpRes = (char*)realloc(res, curLen);
      if(tmpRes = NULL) {
        cjson_free(resStart);
        printf("realloc error in print_string\n");
        exit(1);
      }
      resStart = tmpRes;
      res = resStart + curLen - 80;
    }
  }
  *res++ = '\"';
  *res++ = '\0';
  len += 2;
  char* tmpRes = (char*) realloc(resStart, len);
  if(tmpRes == NULL) {
    cjson_free(resStart);
    printf("realloc error in print_string\n");
    exit(1);
  }
  return tmpRes;
}

//输出unicode
static const char* print_unicode(const char* str, char* des) {
  const char* ptr = str;
  int size;
  unsigned int firstChar = (unsigned int) *ptr & 0xff, 
    unicode = 0;
  if(firstChar <= 0x7f && firstChar >= 0) {
    size = 1;
  } else if (firstChar >= 0xc0 && firstChar <= 0xdf) {
    size = 2;
  } else if (firstChar >= 0xe0 && firstChar <= 0xef) {
    size = 3;
  } else if (firstChar >= 0xf0 && firstChar <= 0xf7) {
    size =  4;
  } else {
    printf("invalid unicode in print_unicode\n");
    exit(1);
  }
  if(size == 1) {
    unicode = (unsigned int) *ptr++ & 0xff & 0xff;
  } else {
    if(size == 4) {
      unicode = (unsigned int) *ptr++ & 0xff & 0x1f;
    } else if (size == 3) {
      unicode = (unsigned int) *ptr++ & 0xff & 0x0f;
    } else {
      unicode = (unsigned int) *ptr++ & 0xff & 0x07;  
    }
    unicode <<= 6;
    
    int time = size - 1;
    while (time--) {
      unicode += (unsigned int) *ptr++ & 0xff & 0x3f;
      time && (unicode <<= 6);
    }
  }  
  //得到unicode，转化为utf-16
  int printInLen = sprintf(des, "%04x", unicode);
  return ptr;
}

//输出数字
static const char* print_number(const Cjson* out) {
  if(out->nodeType != NodeType_NUMBER) {
    printf("type error in print_number method\n");
    exit(1);
  }
  char* res = (char*) malloc(80);
  int len = 0;
  if(!res) {
    printf("malloc error in print_number method\n");
    exit(1);
  }
  if(out -> isInt) {
    len = sprintf(res, "%d", out->value.intNum);
  } else {
    len = sprintf(res, "%e", out->value.doubleNum);
  }
  char* tmpRes = (char*)realloc(res, len + 1);
  if(!tmpRes) {
    printf("realloc error in print_number function \n");
    free(tmpRes);
    exit(1);
  }
  res = tmpRes;
  return res;
}

//输出array 的json
static const char* print_array(const Cjson* out) {
  int curLen = 80, len = 0;
  char* res = (char*)cjson_malloc(curLen),
    *subRes,
    *resStart;
  resStart = res;
  Cjson* curCjson = out->child;
  if(!curCjson) {
    return NULL;
  }
  if(!res) {
    printf("error in print_array method \n");
    cjson_free(res);
    exit(1);
  }
  *res++ = '[';
  len++;
  while(curCjson && curCjson->nodeType && curCjson->nodeType != NOTYPE) {
    subRes = (char*)print_json(curCjson);
    int tmpLen = strlen(subRes);
    len += tmpLen + 1;
    if(len >= curLen) {
      while(curLen <= len)
        curLen *= 2;
      char* tmpc = (char*)realloc(resStart, curLen);
      if(!tmpc) {
        printf("Error in realloc in print_array function");
        exit(1);
      }
      resStart = tmpc;
      res = resStart + len - tmpLen - 2;
    }
    if(sprintf(res, "%s", subRes) != tmpLen ) {
      printf("error in sprintf in print_array method");
      cjson_free(res);
      cjson_free((char*)subRes);
      exit(1);
    }
    cjson_free(subRes);
    res += tmpLen;
    *res++ = ',';
    curCjson = curCjson->next;
  }
  *(--res) = ']';
  ++res;
  *res++ = '\0';
  len += 2;
  char* tmpOne = (char*)realloc(resStart, len);
  if(!tmpOne) {
    printf("error in sprintf in print_array method");
    cjson_free(resStart);
    exit(1);
  }
  return tmpOne;
}

//输出object的json
static const char* print_object(const Cjson* out) {
  if(out->nodeType != NodeType_OBJECT) {
    printf("error in print_object method/n");
    exit(1);
  }
  int curLen = 80, len = 0;
  char* res = (char*)cjson_malloc(curLen),
    *subRes,
    *resStart;
  resStart = res;
  Cjson* curCjson = out->child;
  if(!curCjson) {
    return NULL;
  }
  if(!res) {
    printf("error in print_array method \n");
    cjson_free(res);
    exit(1);
  }
  *res++ = '{';
  len++;
  while(curCjson && curCjson->nodeType
   && curCjson->nodeType != NOTYPE) {
     char *keyName, *value; //处理键
     int nameLen, valueLen;
     keyName = (char *)curCjson->keyName;
     if(!keyName) {
       printf("error in print_object, no keyName\n");
       exit(1);
     }
     nameLen = strlen(keyName);
     len += nameLen + 4; // 加上了逗号与冒号,引号
     if(len >= curLen) {
      while(curLen <= len)
        curLen *= 2;
      char* tmpc = (char*)realloc(resStart, curLen);
      if(!tmpc) {
        printf("Error in realloc in print_object function");
        exit(1);
      }
      resStart = tmpc;
      res = resStart + len - nameLen - 5;
    }
    *res++ = '\"';
    if(sprintf(res, "%s", keyName) != nameLen) {
      printf("error in sprintf in print_object\n");
      exit(1);
    }
    res += nameLen;
    *res++ = '\"';
    *res++ = ':';
    value = (char *)print_json(curCjson); //处理值
    valueLen = strlen(value);
    len += valueLen;
    if(len >= curLen) {
      while(curLen <= len)
        curLen *= 2;
      char* tmpc = (char*)realloc(resStart, curLen);
      if(!tmpc) {
        printf("Error in realloc in print_object function");
        exit(1);
      }
      resStart = tmpc;
      res = resStart + len - valueLen - 1;
    }
    if(sprintf(res, "%s", value) != valueLen) {
      printf("error in sprintf in print_object\n");
      exit(1);
    }
    res += valueLen;
    *res++ = ',';
    cjson_free(value);
    value = NULL;
    curCjson = curCjson->next;
  }
  *(--res) = '}';
  ++res;
  *res++ = '\0';
  len += 2;
  char* tmpOne = (char*)realloc(resStart, len);
  if(!tmpOne) {
    printf("error in sprintf in print_object method");
    cjson_free(resStart);
    exit(1);
  }
  return tmpOne;
}