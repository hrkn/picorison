/*
 * Copyright 2009-2010 Cybozu Labs, Inc.
 * Copyright 2011-2014 Kazuho Oku, Yasuhiro Matsumoto, Shigeo Mitsunari
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "picorison.h"
#include "picotest/picotest.h"

#ifdef _MSC_VER
    #pragma warning(disable : 4127) // conditional expression is constant
#endif

using namespace std;

#define is(x, y, name) _ok((x) == (y), name)

#include <algorithm>
#include <sstream>
#include <limits>

int main(void)
{
  // constructors
#define TEST(expr, expected) \
    is(picorison::value expr .serialize(), string(expected), "picorison::value" #expr)

  TEST( (true),  "!t");
  TEST( (false), "!f");
  TEST( (42.0),   "42");
  TEST( (string("hello")), "hello");
  TEST( ("hello"), "hello");
  TEST( ("hello", 4), "hell");

  {
    double a = 1;
    for (int i = 0; i < 1024; i++) {
      picorison::value vi(a);
      std::stringstream ss;
      ss << vi;
      picorison::value vo;
      ss >> vo;
      double b = vo.get<double>();
      if ((i < 53 && a != b) || fabs(a - b) / b > 1e-8) {
        printf("ng i=%d a=%.18e b=%.18e\n", i, a, b);
      }
      a *= 2;
    }
  }

#undef TEST

#define TEST(in, type, cmp) {        \
    picorison::value v;              \
    const char* s = in;              \
    string err = picorison::parse(v, s, s + strlen(s));      \
    _ok(err.empty(), in " no error");          \
    _ok(v.is<type>(), in " check type");          \
    is(v.get<type>(), static_cast<type>(cmp), in " correct output");      \
    is(*s, '\0', in " read to eof");          \
  }
  TEST("!f", bool, false);
  TEST("!t", bool, true);
  TEST("90.5", double, 90.5);
  TEST("1.7976931348623157e308", double, std::numeric_limits<double>::max());
  TEST("2.2250738585072014e-308", double, std::numeric_limits<double>::min());
  TEST(R"('hello')", string, string("hello"));
  TEST(u8R"('aクリス')", string,
       string("a\xe3\x82\xaf\xe3\x83\xaa\xe3\x82\xb9"));
  TEST(u8R"('𠀋')", string, string("\xf0\xa0\x80\x8b"));
  TEST(R"('Amazing!!')", string, string("Amazing!"));
  TEST(R"('What!'s RISON?')", string, string("What's RISON?"));
#ifdef PICORISON_USE_INT64
  TEST("0", int64_t, 0);
  TEST("-9223372036854775808", int64_t, std::numeric_limits<int64_t>::min());
  TEST("9223372036854775807", int64_t, std::numeric_limits<int64_t>::max());
#endif
#undef TEST

#define TEST(actual, reserialized_expected) {        \
    picorison::value v;              \
    const char* s = actual;              \
    string err = picorison::parse(v, s, s + strlen(s));      \
    is(v.serialize(), string(reserialized_expected), actual " reserialization");      \
  }
  TEST("!f", "!f");
  TEST("!t", "!t");
  TEST("'hello'", "hello");
  TEST("hell-o_1~2.3", "hell-o_1~2.3");  // parsed as id
  TEST("'he is hero'", "'he is hero'");
  TEST("'-123'", "'-123'");
  TEST("',32'", "',32'");
  TEST("'33-4'", "'33-4'");
  TEST("'abc33-4'", "abc33-4");
  TEST("'Amazing!!'", "'Amazing!!'");
  TEST("'What!'s RISON?'", "'What!'s RISON?'");
  TEST("72057594037927936", "72057594037927936");
#ifdef PICORISON_USE_INT64
  TEST("144115188075855872", "144115188075855872");
#else
  TEST("144115188075855872", "1.4411518807585587e17");
#endif
#undef TEST

#define TEST(type, expr) {                 \
    picorison::value v;                   \
    const char *s = expr;                 \
    string err = picorison::parse(v, s, s + strlen(s));           \
    _ok(err.empty(), "empty " #type " no error");           \
    _ok(v.is<picorison::type>(), "empty " #type " check type");         \
    _ok(v.get<picorison::type>().empty(), "check " #type " array size"); \
  }
  TEST(array, "!()");
  TEST(object, "()");
#undef TEST

  {
    picorison::value v;
    const char *s = R"(!(1,!t,'hello'))";
    string err = picorison::parse(v, s, s + strlen(s));
    _ok(err.empty(), "array no error");
    _ok(v.is<picorison::array>(), "array check type");
    is(v.get<picorison::array>().size(), size_t(3), "check array size");
    _ok(v.contains(0), "check contains array[0]");
    _ok(v.get(0).is<double>(), "check array[0] type");
    is(v.get(0).get<double>(), 1.0, "check array[0] value");
    _ok(v.contains(1), "check contains array[1]");
    _ok(v.get(1).is<bool>(), "check array[1] type");
    _ok(v.get(1).get<bool>(), "check array[1] value");
    _ok(v.contains(2), "check contains array[2]");
    _ok(v.get(2).is<string>(), "check array[2] type");
    is(v.get(2).get<string>(), string("hello"), "check array[2] value");
    _ok(!v.contains(3), "check not contains array[3]");
  }

  {
    picorison::value v;
    const char *s = R"(('a':!t))";
    string err = picorison::parse(v, s, s + strlen(s));
    _ok(err.empty(), "object no error");
    _ok(v.is<picorison::object>(), "object check type");
    is(v.get<picorison::object>().size(), size_t(1), "check object size");
    _ok(v.contains("a"), "check contains property");
    _ok(v.get("a").is<bool>(), "check bool property exists");
    is(v.get("a").get<bool>(), true, "check bool property value");
    is(v.serialize(), string(R"((a:!t))"), "serialize object");
    _ok(!v.contains("z"), "check not contains property");
  }

  {
    picorison::value v1;
    v1.set<picorison::object>(picorison::object());
    v1.get<picorison::object>()["-114"] = picorison::value("514");
    v1.get<picorison::object>()["364"].set<picorison::array>(picorison::array());
    v1.get<picorison::object>()["364"].get<picorison::array>().push_back(picorison::value(334.0));
    picorison::value &v2 = v1.get<picorison::object>()["1919"];
    v2.set<picorison::object>(picorison::object());
    v2.get<picorison::object>()["893"] = picorison::value(810.0);
    is(v1.serialize(), string(R"(('-114':'514','1919':('893':810),'364':!(334)))"), "modification succeed");
  }

#define TEST(rison, msg) do {        \
    picorison::value v;          \
    const char *s = rison;        \
    string err = picorison::parse(v, s, s + strlen(s));  \
    is(err, string("syntax error at line " msg), msg);  \
  } while (0)
  TEST("!Foa", "1 near: oa");
  TEST("(]", "1 near: ]");
  TEST("\n\bbell", "1 near: bell");
  TEST("'abc\nd'", "1 near: ");
  TEST("(123:456)", "1 near: :456)");  // Unquoted fully numeric key isn't allowed
  TEST("( 'a': !t )", "1 near:  'a': !t )");  // No whitespace is permitted except inside quoted strings
#undef TEST

  {
    picorison::value v1, v2;
    const char *s;
    string err;
    s = R"(('b':!t,n:(a:'b','C':d-,'-bbb':'a'),'a':!(1,2,'three'),'d':2))";
    err = picorison::parse(v1, s, s + strlen(s));
    s = R"(('d':2.0,b:!t,a:!(1,2,three),n:('-bbb':a,C:d-,a:b)))";
    err = picorison::parse(v2, s, s + strlen(s));
    _ok((v1 == v2), "check == operator in deep comparison");
  }

  {
    picorison::value v1, v2;
    const char *s;
    string err;
    s = R"(('b':!t,'a':!(1,2,'three'),'d':2))";
    err = picorison::parse(v1, s, s + strlen(s));
    s = R"(('d':2.0,'a':!(1,'three'),'b':!t))";
    err = picorison::parse(v2, s, s + strlen(s));
    _ok((v1 != v2), "check != operator for array in deep comparison");
  }

  {
    picorison::value v1, v2;
    const char *s;
    string err;
    s = R"(('b':!t,'a':!(1,2,'three'),'d':2))";
    err = picorison::parse(v1, s, s + strlen(s));
    s = R"(('d':2.0,'a':!(1,2,'three'),'b':false))";
    err = picorison::parse(v2, s, s + strlen(s));
    _ok((v1 != v2), "check != operator for object in deep comparison");
  }

  {
    picorison::value v1, v2;
    const char *s;
    string err;
    s = R"(('b':!t,'a':!(1,2,'three'),'d':2))";
    err = picorison::parse(v1, s, s + strlen(s));
    picorison::object& o = v1.get<picorison::object>();
    o.erase("b");
    picorison::array& a = o["a"].get<picorison::array>();
    picorison::array::iterator i;
    i = std::remove(a.begin(), a.end(), picorison::value(std::string("three")));
    a.erase(i, a.end());
    s = R"(('a':!(1,2),'d':2))";
    err = picorison::parse(v2, s, s + strlen(s));
    _ok((v1 == v2), "check erase()");
  }

  _ok(picorison::value(3.0).serialize() == "3",
     "integral number should be serialized as a integer");

  {
    const char* s = R"(('a':!(1,2),'d':2))";
    picorison::null_parse_context ctx;
    string err;
    picorison::_parse(ctx, s, s + strlen(s), &err);
    _ok(err.empty(), "null_parse_context");
  }

  {
    picorison::value v1, v2;
    v1 = picorison::value(true);
    swap(v1, v2);
    _ok(v1.is<picorison::null>(), "swap (null)");
    _ok(v2.get<bool>() == true, "swap (bool)");

    v1 = picorison::value("a");
    v2 = picorison::value(1.0);
    swap(v1, v2);
    _ok(v1.get<double>() == 1.0, "swap (dobule)");
    _ok(v2.get<string>() == "a", "swap (string)");

    v1 = picorison::value(picorison::object());
    v2 = picorison::value(picorison::array());
    swap(v1, v2);
    _ok(v1.is<picorison::array>(), "swap (array)");
    _ok(v2.is<picorison::object>(), "swap (object)");
  }

  {
    picorison::value v;
    const char *s = R"(('a':1,'b':!(2,('b1':'abc')),'c':(),'d':!()))";
    string err;
    err = picorison::parse(v, s, s + strlen(s));
    _ok(err.empty(), "parse test data for prettifying output");
    _ok(v.serialize() == R"((a:1,b:!(2,(b1:abc)),c:(),d:!()))", "non-prettifying output");
  }

  try {
    picorison::value v(std::numeric_limits<double>::quiet_NaN());
    _ok(false, "should not accept NaN");
  } catch (std::overflow_error& e) {
    _ok(true, "should not accept NaN");
  }

  try {
    picorison::value v(std::numeric_limits<double>::infinity());
    _ok(false, "should not accept infinity");
  } catch (std::overflow_error& e) {
    _ok(true, "should not accept infinity");
  }

  try {
    picorison::value v(123.);
    _ok(! v.is<bool>(), "is<wrong_type>() should return false");
    v.get<bool>();
    _ok(false, "get<wrong_type>() should raise an error");
  } catch (std::runtime_error& e) {
    _ok(true, "get<wrong_type>() should raise an error");
  }

#ifdef PICORISON_USE_INT64
  {
    picorison::value v1((int64_t)123);
    _ok(v1.is<int64_t>(), "is int64_t");
    _ok(v1.is<double>(), "is double as well");
    _ok(v1.serialize() == "123", "serialize the value");
    _ok(v1.get<int64_t>() == 123, "value is correct as int64_t");
    _ok(v1.get<double>(), "value is correct as double");

    _ok(! v1.is<int64_t>(), "is no more int64_type once get<double>() is called");
    _ok(v1.is<double>(), "and is still a double");

    const char *s = "-9223372036854775809";
    _ok(picorison::parse(v1, s, s + strlen(s)).empty(), "parse underflowing int64_t");
    _ok(! v1.is<int64_t>(), "underflowing int is not int64_t");
    _ok(v1.is<double>(), "underflowing int is double");
    _ok(v1.get<double>() + 9.22337203685478e+18 < 65536, "double value is somewhat correct");
  }
#endif

  {
    picorison::value v;
    std::string err = picorison::parse(v, R"(!(1,'abc'))");
    _ok(err.empty(), "simple API no error");
    _ok(v.is<picorison::array>(), "simple API return type is array");
    is(v.get<picorison::array>().size(), 2, "simple API array size");
    _ok(v.get<picorison::array>()[0].is<double>(), "simple API type #0");
    is(v.get<picorison::array>()[0].get<double>(), 1, "simple API value #0");
    _ok(v.get<picorison::array>()[1].is<std::string>(), "simple API type #1");
    is(v.get<picorison::array>()[1].get<std::string>(), "abc", "simple API value #1");
  }

  {
    picorison::value v1((double) 0);
    _ok(! v1.evaluate_as_boolean(), "((double) 0) is false");
    picorison::value v2((double) 1);
    _ok(v2.evaluate_as_boolean(), "((double) 1) is true");
#ifdef PICORISON_USE_INT64
    picorison::value v3((int64_t) 0);
    _ok(! v3.evaluate_as_boolean(), "((int64_t) 0) is false");
    picorison::value v4((int64_t) 1);
    _ok(v4.evaluate_as_boolean(), "((int64_t) 1) is true");
#endif
  }

  {
    picorison::value v;
    std::string err;
    const char *s = "123abc";
    const char *reststr = picorison::parse(v, s, s + strlen(s), &err);
    _ok(err.empty(), "should succeed");
    _ok(v.is<double>(), "is number");
    _ok(v.get<double>() == 123.0, "is 123");
    is(*reststr, 'a', "should point at the next char");
  }

  return done_testing();
}
