////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 - 2021, Göteborg Bit Factory.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// https://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
// cmake.h include header must come first

#include <Variant.h>
#include <test.h>

#include <iostream>

////////////////////////////////////////////////////////////////////////////////
int main(int, char**) {
  UnitTest t(76);

  Variant v0(true);
  Variant v1(42);
  Variant v2(3.14);
  Variant v3("foo");
  Variant v4(1234567890, Variant::type_date);
  Variant v5(1200, Variant::type_duration);

  // Truth table.
  Variant vFalse(false);
  Variant vTrue(true);
  t.is(vFalse.operator_xor(vFalse), false, "false xor false --> false");
  t.is(vFalse.operator_xor(vTrue), true, "false xor true --> false");
  t.is(vTrue.operator_xor(vFalse), true, "true xor false --> false");
  t.is(vTrue.operator_xor(vTrue), false, "true xor true --> false");

  Variant v00 = v0.operator_xor(v0);
  t.is(v00.type(), Variant::type_boolean, "true xor true --> boolean");
  t.is(v00.get_bool(), false, "true xor true --> false");

  Variant v01 = v0.operator_xor(v1);
  t.is(v01.type(), Variant::type_boolean, "true xor 42 --> boolean");
  t.is(v01.get_bool(), false, "true xor 42 --> false");

  Variant v02 = v0.operator_xor(v2);
  t.is(v02.type(), Variant::type_boolean, "true xor 3.14 --> boolean");
  t.is(v02.get_bool(), false, "true xor 3.14 --> false");

  Variant v03 = v0.operator_xor(v3);
  t.is(v03.type(), Variant::type_boolean, "true xor 'foo' --> boolean");
  t.is(v03.get_bool(), false, "true xor 'foo' --> false");

  Variant v04 = v0.operator_xor(v4);
  t.is(v04.type(), Variant::type_boolean, "true xor 1234567890 --> boolean");
  t.is(v04.get_bool(), false, "true xor 1234567890 --> false");

  Variant v05 = v0.operator_xor(v5);
  t.is(v05.type(), Variant::type_boolean, "true xor 1200 --> boolean");
  t.is(v05.get_bool(), false, "true xor 1200 --> false");

  Variant v10 = v1.operator_xor(v0);
  t.is(v10.type(), Variant::type_boolean, "42 xor true --> boolean");
  t.is(v10.get_bool(), false, "42 xor true --> false");

  Variant v11 = v1.operator_xor(v1);
  t.is(v11.type(), Variant::type_boolean, "42 xor 42 --> boolean");
  t.is(v11.get_bool(), false, "42 xor 42 --> false");

  Variant v12 = v1.operator_xor(v2);
  t.is(v12.type(), Variant::type_boolean, "42 xor 3.14 --> boolean");
  t.is(v12.get_bool(), false, "42 xor 3.14 --> false");

  Variant v13 = v1.operator_xor(v3);
  t.is(v13.type(), Variant::type_boolean, "42 xor 'foo' --> boolean");
  t.is(v13.get_bool(), false, "42 xor 'foo' --> false");

  Variant v14 = v1.operator_xor(v4);
  t.is(v04.type(), Variant::type_boolean, "42 xor 1234567890 --> boolean");
  t.is(v04.get_bool(), false, "42 xor 1234567890 --> false");

  Variant v15 = v1.operator_xor(v5);
  t.is(v15.type(), Variant::type_boolean, "42 xor 1200 --> boolean");
  t.is(v15.get_bool(), false, "42 xor 1200 --> false");

  Variant v20 = v2.operator_xor(v0);
  t.is(v20.type(), Variant::type_boolean, "3.14 xor true --> boolean");
  t.is(v20.get_bool(), false, "3.14 xor true --> false");

  Variant v21 = v2.operator_xor(v1);
  t.is(v21.type(), Variant::type_boolean, "3.14 xor 42 --> boolean");
  t.is(v21.get_bool(), false, "3.14 xor 42 --> false");

  Variant v22 = v2.operator_xor(v2);
  t.is(v22.type(), Variant::type_boolean, "3.14 xor 3.14 --> boolean");
  t.is(v22.get_bool(), false, "3.14 xor 3.14 --> false");

  Variant v23 = v2.operator_xor(v3);
  t.is(v23.type(), Variant::type_boolean, "3.14 xor 'foo' --> boolean");
  t.is(v23.get_bool(), false, "3.14 xor 'foo' --> false");

  Variant v24 = v2.operator_xor(v4);
  t.is(v24.type(), Variant::type_boolean, "3.14 xor 1234567890 --> boolean");
  t.is(v24.get_bool(), false, "3.14 xor 1234567890 --> false");

  Variant v25 = v2.operator_xor(v5);
  t.is(v25.type(), Variant::type_boolean, "3.14 xor 1200 --> boolean");
  t.is(v25.get_bool(), false, "3.14 xor 1200 --> false");

  Variant v30 = v3.operator_xor(v0);
  t.is(v30.type(), Variant::type_boolean, "'foo' xor true --> boolean");
  t.is(v30.get_bool(), false, "'foo' xor true --> false");

  Variant v31 = v3.operator_xor(v1);
  t.is(v31.type(), Variant::type_boolean, "'foo' xor 42 --> boolean");
  t.is(v31.get_bool(), false, "'foo' xor 42 --> false");

  Variant v32 = v3.operator_xor(v2);
  t.is(v32.type(), Variant::type_boolean, "'foo' xor 3.14 --> boolean");
  t.is(v32.get_bool(), false, "'foo' xor 3.14 --> false");

  Variant v33 = v3.operator_xor(v3);
  t.is(v33.type(), Variant::type_boolean, "'foo' xor 'foo' --> boolean");
  t.is(v33.get_bool(), false, "'foo' xor 'foo' --> false");

  Variant v34 = v3.operator_xor(v4);
  t.is(v34.type(), Variant::type_boolean, "'foo' xor 1234567890 --> boolean");
  t.is(v34.get_bool(), false, "'foo' xor 1234567890 --> false");

  Variant v35 = v3.operator_xor(v5);
  t.is(v35.type(), Variant::type_boolean, "'foo' xor 1200 --> boolean");
  t.is(v35.get_bool(), false, "'foo' xor 1200 --> false");

  Variant v40 = v4.operator_xor(v0);
  t.is(v40.type(), Variant::type_boolean, "1234567890 xor true --> boolean");
  t.is(v40.get_bool(), false, "1234567890 xor true --> false");

  Variant v41 = v4.operator_xor(v1);
  t.is(v41.type(), Variant::type_boolean, "1234567890 xor 42 --> boolean");
  t.is(v41.get_bool(), false, "1234567890 xor 42 --> false");

  Variant v42 = v4.operator_xor(v2);
  t.is(v42.type(), Variant::type_boolean, "1234567890 xor 3.14 --> boolean");
  t.is(v42.get_bool(), false, "1234567890 xor 3.14 --> false");

  Variant v43 = v4.operator_xor(v3);
  t.is(v43.type(), Variant::type_boolean, "1234567890 xor 'foo' --> boolean");
  t.is(v43.get_bool(), false, "1234567890 xor 'foo' --> false");

  Variant v44 = v4.operator_xor(v4);
  t.is(v44.type(), Variant::type_boolean, "1234567890 xor 1234567890 --> boolean");
  t.is(v44.get_bool(), false, "1234567890 xor 1234567890 --> false");

  Variant v45 = v4.operator_xor(v5);
  t.is(v45.type(), Variant::type_boolean, "1234567890 xor 1200 --> boolean");
  t.is(v45.get_bool(), false, "1234567890 xor 1200 --> false");

  Variant v50 = v5.operator_xor(v0);
  t.is(v50.type(), Variant::type_boolean, "1200 xor true --> boolean");
  t.is(v50.get_bool(), false, "1200 xor true --> false");

  Variant v51 = v5.operator_xor(v1);
  t.is(v51.type(), Variant::type_boolean, "1200 xor 42 --> boolean");
  t.is(v51.get_bool(), false, "1200 xor 42 --> false");

  Variant v52 = v5.operator_xor(v2);
  t.is(v52.type(), Variant::type_boolean, "1200 xor 3.14 --> boolean");
  t.is(v52.get_bool(), false, "1200 xor 3.14 --> false");

  Variant v53 = v5.operator_xor(v3);
  t.is(v53.type(), Variant::type_boolean, "1200 xor 'foo' --> boolean");
  t.is(v53.get_bool(), false, "1200 xor 'foo' --> false");

  Variant v54 = v5.operator_xor(v4);
  t.is(v04.type(), Variant::type_boolean, "1200 xor 1234567890 --> boolean");
  t.is(v04.get_bool(), false, "1200 xor 1234567890 --> false");

  Variant v55 = v5.operator_xor(v5);
  t.is(v55.type(), Variant::type_boolean, "1200 xor 1200 --> boolean");
  t.is(v55.get_bool(), false, "1200 xor 1200 --> false");

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
