#include "cjson.h"


int main(int argc, const char ** argv) {
  char text1[]="{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n},\"a\":1\n}";	
	char text2[]="[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	char text3[]="[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	char text4[]="{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}";
	char text5[]="[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]";

	char text6[] = "\"\\\"\\u1234abc\\u1234abc\\\"123\"";
	char text7[] = "false";
	char text8[] = "-10e-10";
	char text9[] = "{\"name\": \"asd\"}";
	// Cjson* res = cjson_parse("{\n\"name\":\"jack\"}");
	// Cjson* res1 = cjson_parse("true");
	
	// Cjson* res1 = cjson_parse(text1);
	// Cjson* res2 = cjson_parse(text2);
	// Cjson* res3 = cjson_parse(text3);
	// Cjson* res4 = cjson_parse(text4);
	// Cjson* res5 = cjson_parse(text5);
	// Cjson* res6 = cjson_parse(text6);
	// Cjson* res7 = cjson_parse(text7);
	// Cjson* res8 = cjson_parse(text8);
	// Cjson* res9 = cjson_parse(text9);
	Cjson* res10 = cjson_parse(text5);
	const char* out = print_json(res10);
	printf("%s\n", out);
	free((void*)out);

	deleteCjson(res10);
}