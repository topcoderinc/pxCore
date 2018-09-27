/*

pxCore Copyright 2005-2018 John Robinson

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/


#include "testString.h"

#include "rtTest.h"
#include "rtString.h"

#include <string.h>

void testString()
{
  printf("testString\n");
  rtString s;
  rtString s2;
  const char sz1[] = "Sample String";
  const char sz2[] = "String";
  const char sz3[] = "S";
  const char sz4[] = "Iñtërnâtiônàližætiøn";

  RT_TEST(s == NULL);
  RT_TEST(s == "");
  RT_TEST(s != sz1);
  RT_TEST(s.length() == 0);
  RT_TEST(s.byteLength() == 0);
  RT_TEST(strcmp(s.cString(),"") == 0);
  RT_TEST(strcmp(s,"") == 0);
  RT_TEST(s.isEmpty());
  s = sz1;
  RT_TEST(!s.isEmpty());
  s.term();
  RT_TEST(s.isEmpty());
  s = sz4;
  RT_TEST(s.length() == 20);
  RT_TEST(s.byteLength() == 28);

  RT_TEST(s == s);

  s2 = sz4;
  RT_TEST(s == s2);
  RT_TEST(s2 == s);
  
  s = sz4;
  s2 = "";
  RT_TEST(s2 < s);
  RT_TEST(s > s2);
  RT_TEST(s2 <= s);
  RT_TEST(s >= s2);
  RT_TEST(s <= s);
  RT_TEST(s >= s);
  RT_TEST(s != s2);

  s2 = "A";
  RT_TEST(s2 < s);
  RT_TEST(s > s2);
  RT_TEST(s2 <= s);
  RT_TEST(s >= s2);
  RT_TEST(s <= s);
  RT_TEST(s >= s);
  RT_TEST(s != s2);

  s = sz1;
  RT_TEST(s != "");
  RT_TEST(s == sz1);
  RT_TEST(s.length() == 13);
  RT_TEST(s.find(0,sz2) == 7);
  RT_TEST(s.find(0,sz3) == 0);
  RT_TEST(s.find(1,sz3) == 7);
  RT_TEST(s.find(0,"Blah") == (size_t)-1);

  RT_TEST(s.find(0,'S') == 0);
  RT_TEST(s.find(1,'S') == 7);

  RT_TEST(s.substring(0) == sz1);
  RT_TEST(s.substring(0,1) == "S");
  RT_TEST(s.substring(7) == sz2);
  RT_TEST(s.substring(7,6) == sz2);
}
