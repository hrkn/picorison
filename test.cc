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
#include "picojson.h"
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
    is(picojson::value expr .serialize(), string(expected), "picojson::value" #expr)

  TEST( (true),  "!t");
  TEST( (false), "!f");
  TEST( (42.0),   "42");
  TEST( (string("hello")), "hello");
  TEST( ("hello"), "hello");
  TEST( ("hello", 4), "hell");

  {
    double a = 1;
    for (int i = 0; i < 1024; i++) {
      picojson::value vi(a);
      std::stringstream ss;
      ss << vi;
      picojson::value vo;
      ss >> vo;
      double b = vo.get<double>();
      if ((i < 53 && a != b) || fabs(a - b) / b > 1e-8) {
        printf("ng i=%d a=%.18e b=%.18e\n", i, a, b);
      }
      a *= 2;
    }
  }

#undef TEST

#define TEST(in, type, cmp) {				\
    picojson::value v;							\
    const char* s = in;							\
    string err = picojson::parse(v, s, s + strlen(s));			\
    _ok(err.empty(), in " no error");					\
    _ok(v.is<type>(), in " check type");					\
    is(v.get<type>(), static_cast<type>(cmp), in " correct output");			\
    is(*s, '\0', in " read to eof");					\
  }
  TEST("!f", bool, false);
  TEST("!t", bool, true);
  TEST("90.5", double, 90.5);
  TEST("1.7976931348623157e+308", double, std::numeric_limits<double>::max());
  TEST(R"('hello')", string, string("hello"));
  TEST(u8R"('aクリス')", string,
       string("a\xe3\x82\xaf\xe3\x83\xaa\xe3\x82\xb9"));
  TEST(u8R"('𠀋')", string, string("\xf0\xa0\x80\x8b"));
  TEST(R"('Amazing!!')", string, string("Amazing!"));
  TEST(R"('What!'s RISON?')", string, string("What's RISON?"));
#ifdef PICOJSON_USE_INT64
  TEST("0", int64_t, 0);
  TEST("-9223372036854775808", int64_t, std::numeric_limits<int64_t>::min());
  TEST("9223372036854775807", int64_t, std::numeric_limits<int64_t>::max());
#endif
#undef TEST

#define TEST(actual, reserialized_expected) {				\
    picojson::value v;							\
    const char* s = actual;							\
    string err = picojson::parse(v, s, s + strlen(s));			\
    is(v.serialize(), string(reserialized_expected), actual " reserialization");			\
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
#undef TEST

#define TEST(type, expr) {					       \
    picojson::value v;						       \
    const char *s = expr;					       \
    string err = picojson::parse(v, s, s + strlen(s));		       \
    _ok(err.empty(), "empty " #type " no error");		       \
    _ok(v.is<picojson::type>(), "empty " #type " check type");	       \
    _ok(v.get<picojson::type>().empty(), "check " #type " array size"); \
  }
  TEST(array, "!()");
  TEST(object, "()");
#undef TEST

  {
    picojson::value v;
    const char *s = R"(!(1,!t,'hello'))";
    string err = picojson::parse(v, s, s + strlen(s));
    _ok(err.empty(), "array no error");
    _ok(v.is<picojson::array>(), "array check type");
    is(v.get<picojson::array>().size(), size_t(3), "check array size");
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
    picojson::value v;
    const char *s = R"(( 'a': !t ))";
    string err = picojson::parse(v, s, s + strlen(s));
    _ok(err.empty(), "object no error");
    _ok(v.is<picojson::object>(), "object check type");
    is(v.get<picojson::object>().size(), size_t(1), "check object size");
    _ok(v.contains("a"), "check contains property");
    _ok(v.get("a").is<bool>(), "check bool property exists");
    is(v.get("a").get<bool>(), true, "check bool property value");
    is(v.serialize(), string(R"((a:!t))"), "serialize object");
    _ok(!v.contains("z"), "check not contains property");
  }

  {
    picojson::value v1;
  	v1.set<picojson::object>(picojson::object());
    v1.get<picojson::object>()["-114"] = picojson::value("514");
  	v1.get<picojson::object>()["364"].set<picojson::array>(picojson::array());
  	v1.get<picojson::object>()["364"].get<picojson::array>().push_back(picojson::value(334.0));
  	picojson::value &v2 = v1.get<picojson::object>()["1919"];
  	v2.set<picojson::object>(picojson::object());
  	v2.get<picojson::object>()["893"] = picojson::value(810.0);
    is(v1.serialize(), string(R"(('-114':'514','1919':('893':810),'364':!(334)))"), "modification succeed");
  }

#define TEST(json, msg) do {				\
    picojson::value v;					\
    const char *s = json;				\
    string err = picojson::parse(v, s, s + strlen(s));	\
    is(err, string("syntax error at line " msg), msg);	\
  } while (0)
  TEST("!Foa", "1 near: oa");
  TEST("(]", "1 near: ]");
  TEST("\n\bbell", "2 near: bell");
  TEST("'abc\nd'", "1 near: ");
#undef TEST

  {
    picojson::value v1, v2;
    const char *s;
    string err;
    s = R"(( 'b': !t, n:(a:'b','C':d-,'-bbb':'a'), 'a': !(1,2,'three'), 'd': 2))";
    err = picojson::parse(v1, s, s + strlen(s));
    s = R"(( 'd': 2.0, b: !t, a: !(1,2,three), n:('-bbb':a,C:d-,a:b) ))";
    err = picojson::parse(v2, s, s + strlen(s));
    _ok((v1 == v2), "check == operator in deep comparison");
  }

  {
    picojson::value v1, v2;
    const char *s;
    string err;
    s = R"(( 'b': !t, 'a': !(1,2,'three'), 'd': 2 ))";
    err = picojson::parse(v1, s, s + strlen(s));
    s = R"(( 'd': 2.0, 'a': !(1,'three'), 'b': !t ))";
    err = picojson::parse(v2, s, s + strlen(s));
    _ok((v1 != v2), "check != operator for array in deep comparison");
  }

  {
    picojson::value v1, v2;
    const char *s;
    string err;
    s = R"(( 'b': !t, 'a': !(1,2,'three'), 'd': 2 ))";
    err = picojson::parse(v1, s, s + strlen(s));
    s = R"(( 'd': 2.0, 'a': !(1,2,'three'), 'b': false ))";
    err = picojson::parse(v2, s, s + strlen(s));
    _ok((v1 != v2), "check != operator for object in deep comparison");
  }

  {
    picojson::value v1, v2;
    const char *s;
    string err;
    s = R"(( 'b': !t, 'a': !(1,2,'three'), 'd': 2 ))";
    err = picojson::parse(v1, s, s + strlen(s));
    picojson::object& o = v1.get<picojson::object>();
    o.erase("b");
    picojson::array& a = o["a"].get<picojson::array>();
    picojson::array::iterator i;
    i = std::remove(a.begin(), a.end(), picojson::value(std::string("three")));
    a.erase(i, a.end());
    s = R"(( 'a': !(1,2), 'd': 2 ))";
    err = picojson::parse(v2, s, s + strlen(s));
    _ok((v1 == v2), "check erase()");
  }

  _ok(picojson::value(3.0).serialize() == "3",
     "integral number should be serialized as a integer");

  {
    const char* s = R"(( 'a': !(1,2), 'd': 2 ))";
    picojson::null_parse_context ctx;
    string err;
    picojson::_parse(ctx, s, s + strlen(s), &err);
    _ok(err.empty(), "null_parse_context");
  }

  {
    picojson::value v1, v2;
    v1 = picojson::value(true);
    swap(v1, v2);
    _ok(v1.is<picojson::null>(), "swap (null)");
    _ok(v2.get<bool>() == true, "swap (bool)");

    v1 = picojson::value("a");
    v2 = picojson::value(1.0);
    swap(v1, v2);
    _ok(v1.get<double>() == 1.0, "swap (dobule)");
    _ok(v2.get<string>() == "a", "swap (string)");

    v1 = picojson::value(picojson::object());
    v2 = picojson::value(picojson::array());
    swap(v1, v2);
    _ok(v1.is<picojson::array>(), "swap (array)");
    _ok(v2.is<picojson::object>(), "swap (object)");
  }

  {
    picojson::value v;
    const char *s = R"(( 'a': 1, 'b': !( 2, ( 'b1': 'abc' ) ), 'c': (), 'd': !() ))";
    string err;
    err = picojson::parse(v, s, s + strlen(s));
    _ok(err.empty(), "parse test data for prettifying output");
    _ok(v.serialize() == R"((a:1,b:!(2,(b1:abc)),c:(),d:!()))", "non-prettifying output");
  }

  try {
    picojson::value v(std::numeric_limits<double>::quiet_NaN());
    _ok(false, "should not accept NaN");
  } catch (std::overflow_error& e) {
    _ok(true, "should not accept NaN");
  }

  try {
    picojson::value v(std::numeric_limits<double>::infinity());
    _ok(false, "should not accept infinity");
  } catch (std::overflow_error& e) {
    _ok(true, "should not accept infinity");
  }

  try {
    picojson::value v(123.);
    _ok(! v.is<bool>(), "is<wrong_type>() should return false");
    v.get<bool>();
    _ok(false, "get<wrong_type>() should raise an error");
  } catch (std::runtime_error& e) {
    _ok(true, "get<wrong_type>() should raise an error");
  }

#ifdef PICOJSON_USE_INT64
  {
    picojson::value v1((int64_t)123);
    _ok(v1.is<int64_t>(), "is int64_t");
    _ok(v1.is<double>(), "is double as well");
    _ok(v1.serialize() == "123", "serialize the value");
    _ok(v1.get<int64_t>() == 123, "value is correct as int64_t");
    _ok(v1.get<double>(), "value is correct as double");

    _ok(! v1.is<int64_t>(), "is no more int64_type once get<double>() is called");
    _ok(v1.is<double>(), "and is still a double");

    const char *s = "-9223372036854775809";
    _ok(picojson::parse(v1, s, s + strlen(s)).empty(), "parse underflowing int64_t");
    _ok(! v1.is<int64_t>(), "underflowing int is not int64_t");
    _ok(v1.is<double>(), "underflowing int is double");
    _ok(v1.get<double>() + 9.22337203685478e+18 < 65536, "double value is somewhat correct");
  }
#endif

  {
    picojson::value v;
    std::string err = picojson::parse(v, R"(!( 1, 'abc' ))");
    _ok(err.empty(), "simple API no error");
    _ok(v.is<picojson::array>(), "simple API return type is array");
    is(v.get<picojson::array>().size(), 2, "simple API array size");
    _ok(v.get<picojson::array>()[0].is<double>(), "simple API type #0");
    is(v.get<picojson::array>()[0].get<double>(), 1, "simple API value #0");
    _ok(v.get<picojson::array>()[1].is<std::string>(), "simple API type #1");
    is(v.get<picojson::array>()[1].get<std::string>(), "abc", "simple API value #1");
  }

  {
    picojson::value v1((double) 0);
    _ok(! v1.evaluate_as_boolean(), "((double) 0) is false");
    picojson::value v2((double) 1);
    _ok(v2.evaluate_as_boolean(), "((double) 1) is true");
#ifdef PICOJSON_USE_INT64
    picojson::value v3((int64_t) 0);
    _ok(! v3.evaluate_as_boolean(), "((int64_t) 0) is false");
    picojson::value v4((int64_t) 1);
    _ok(v4.evaluate_as_boolean(), "((int64_t) 1) is true");
#endif
  }

  {
    picojson::value v;
    std::string err;
    const char *s = "123abc";
    const char *reststr = picojson::parse(v, s, s + strlen(s), &err);
    _ok(err.empty(), "should succeed");
    _ok(v.is<double>(), "is number");
    _ok(v.get<double>() == 123.0, "is 123");
    is(*reststr, 'a', "should point at the next char");
  }

  return done_testing();
}
